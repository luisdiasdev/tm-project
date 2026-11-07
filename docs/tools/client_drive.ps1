<#
.SYNOPSIS
  Drive the 748 client from the command line: launch, click, type, capture,
  or walk through the whole login sequence.

.DESCRIPTION
  PrintWindow() returns a black image for this D3D9 window, so screenshots use
  CopyFromScreen over the window rect after bringing the window forward. Keep
  the game on top: anything overlapping it shows up in the capture.

  Coordinates are window-relative. With the window at (0,0) they match the
  captured image pixel for pixel; the client area starts ~26 px below the top
  of the window (title bar), so image Y = logical client Y + 26.

  Every action except "launch" and "kill" requires the client to be running.
  "click" and "type" act on the current screen; run them in sequence, e.g.:

      powershell -File docs\tools\client_drive.ps1 -Action launch
      powershell -File docs\tools\client_drive.ps1 -Action login -Out shots\charselect.png
      powershell -File docs\tools\client_drive.ps1 -Action click -X 70 -Y 233 -Out shots\create.png

  The click is held for -HoldMilliseconds because the client polls input per
  frame and a zero-length synthetic click can be missed.

.PARAMETER Action
  launch  - kill (with -Restart) and start the client, wait for its window
  capture - save a screenshot of the window to -Out
  click   - click at -X/-Y (window-relative), optional -Out screenshot after
  type    - SendKeys -Text to the focused window, optional -Out screenshot
  login   - server row -> channel row -> Connect -> admin/admin -> Login
  kill    - stop the client

.PARAMETER Exe
  Client executable. Defaults to ..\client_748\TMProject_d.exe relative to the
  repository root.

.PARAMETER WorkingDir
  Working directory for launch. Defaults to the folder containing -Exe.

.PARAMETER Out
  PNG path for capture. Parent folders are created.

.PARAMETER X, Y
  Window-relative click position for -Action click.

.PARAMETER Text
  SendKeys string for -Action type, e.g. "admin{TAB}admin" or "{ENTER}".

.PARAMETER WaitSeconds
  Extra settle time after launch (default 8) or after Connect during login.

.EXAMPLE
  powershell -File docs\tools\client_drive.ps1 -Action launch -Restart

.EXAMPLE
  powershell -File docs\tools\client_drive.ps1 -Action login -Out artifacts\selchar.png

.EXAMPLE
  powershell -File docs\tools\client_drive.ps1 -Action click -X 377 -Y 498 -Out artifacts\modal.png

.EXAMPLE
  powershell -File docs\tools\client_drive.ps1 -Action type -Text "{ENTER}" -Out artifacts\after-enter.png
#>
param(
  [Parameter(Mandatory = $true)]
  [ValidateSet('launch', 'capture', 'click', 'type', 'login', 'kill')]
  [string]$Action,

  [string]$Exe,
  [string]$WorkingDir,
  [string]$Out,

  [int]$X = -1,
  [int]$Y = -1,
  [string]$Text,

  [int]$WaitSeconds = 8,
  [int]$HoldMilliseconds = 180,
  [int]$SettleMilliseconds = 500,

  [switch]$Restart
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if (-not $Exe) {
  $Exe = Join-Path $repoRoot '..\client_748\TMProject_d.exe'
}
if (-not $WorkingDir) {
  $WorkingDir = Split-Path -Parent $Exe
}

$Exe = (Resolve-Path $Exe).Path
$WorkingDir = (Resolve-Path $WorkingDir).Path
$processName = [System.IO.Path]::GetFileNameWithoutExtension($Exe)

# 748 login screen, 800x600, window at (0,0): see docs/port-748/build-and-test.md.
# KEEP IN MIND Y coordinates are window-relative, not client-relative; the client area starts ~26 px below the top of the window.
# So always add 26 to the logical client Y coordinate you get from the client debug console.
# X coordinates are the same, but the offset is only 8 px for the borders.
$loginUi = @{
  ServerRow  = @(340, 330)   # "World SV" group row
  ChannelRow = @(470, 367)   # 4th entry, "World SV-..." (rows at ~327/340/353/367)
  Connect    = @(311, 498)
  Account    = @(430, 296)
  Login      = @(360, 351)
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WydClientDrive {
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int Left, Top, Right, Bottom; }

  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
}
"@

[WydClientDrive]::SetProcessDPIAware() | Out-Null

function Get-GameProcess {
  $proc = Get-Process -Name $processName -ErrorAction SilentlyContinue | Select-Object -First 1

  if (-not $proc) {
    throw "$processName is not running. Start it with -Action launch."
  }

  $proc.Refresh()

  if ($proc.MainWindowHandle -eq 0) {
    throw "$processName has no main window yet; wait a second and retry."
  }

  return $proc
}

function Set-GameForeground {
  $proc = Get-GameProcess
  [WydClientDrive]::ShowWindow($proc.MainWindowHandle, 9) | Out-Null  # SW_RESTORE
  [WydClientDrive]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
  Start-Sleep -Milliseconds 400

  $rect = New-Object WydClientDrive+RECT
  [WydClientDrive]::GetWindowRect($proc.MainWindowHandle, [ref]$rect) | Out-Null
  return $rect
}

function Invoke-ClickAt([int]$clickX, [int]$clickY) {
  $rect = Set-GameForeground
  [WydClientDrive]::SetCursorPos($rect.Left + $clickX, $rect.Top + $clickY) | Out-Null
  Start-Sleep -Milliseconds 250
  [WydClientDrive]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)   # left down
  Start-Sleep -Milliseconds $HoldMilliseconds
  [WydClientDrive]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)   # left up
  Start-Sleep -Milliseconds $SettleMilliseconds
}

function Save-Capture([string]$path) {
  if (-not $path) {
    return
  }

  $rect = Set-GameForeground
  $width = $rect.Right - $rect.Left
  $height = $rect.Bottom - $rect.Top

  $full = [System.IO.Path]::GetFullPath($path)
  $dir = Split-Path -Parent $full
  if ($dir -and -not (Test-Path $dir)) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
  }

  $bitmap = New-Object System.Drawing.Bitmap $width, $height
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size $width, $height))
  $graphics.Dispose()
  $bitmap.Save($full, [System.Drawing.Imaging.ImageFormat]::Png)
  $bitmap.Dispose()

  Write-Output "saved $full (window $($rect.Left),$($rect.Top) ${width}x${height})"
}

function Send-Keys([string]$keys) {
  Set-GameForeground | Out-Null
  [System.Windows.Forms.SendKeys]::SendWait($keys)
  Start-Sleep -Milliseconds 500
}

switch ($Action) {
  'kill' {
    Get-Process -Name $processName -ErrorAction SilentlyContinue | Stop-Process -Force
    Write-Output "stopped $processName"
  }

  'launch' {
    if ($Restart) {
      Get-Process -Name $processName -ErrorAction SilentlyContinue | Stop-Process -Force
      Start-Sleep -Milliseconds 800
    }
    elseif (Get-Process -Name $processName -ErrorAction SilentlyContinue) {
      throw "$processName is already running. Use -Restart to replace it."
    }

    $proc = Start-Process -FilePath $Exe -WorkingDirectory $WorkingDir -PassThru

    for ($i = 0; $i -lt 60; $i++) {
      Start-Sleep -Milliseconds 500
      $proc.Refresh()
      if ($proc.MainWindowHandle -ne 0) {
        break
      }
      if ($proc.HasExited) {
        throw "$processName exited during startup."
      }
    }

    if ($proc.MainWindowHandle -eq 0) {
      throw "$processName did not create a window in 30 s."
    }

    Start-Sleep -Seconds $WaitSeconds
    Save-Capture $Out
    Write-Output "started $processName (pid $($proc.Id))"
  }

  'capture' {
    if (-not $Out) {
      throw "-Out is required for -Action capture."
    }
    $null = Get-GameProcess
    Save-Capture $Out
  }

  'click' {
    if ($X -lt 0 -or $Y -lt 0) {
      throw "-X and -Y are required for -Action click."
    }
    Invoke-ClickAt $X $Y
    Write-Output "clicked $X,$Y"
    Save-Capture $Out
  }

  'type' {
    if (-not $Text) {
      throw "-Text is required for -Action type."
    }
    Send-Keys $Text
    Write-Output "typed $Text"
    Save-Capture $Out
  }

  'login' {
    $null = Get-GameProcess

    Invoke-ClickAt $loginUi.ServerRow[0] $loginUi.ServerRow[1]
    Invoke-ClickAt $loginUi.ChannelRow[0] $loginUi.ChannelRow[1]
    Invoke-ClickAt $loginUi.Connect[0] $loginUi.Connect[1]

    Start-Sleep -Seconds $WaitSeconds

    Invoke-ClickAt $loginUi.Account[0] $loginUi.Account[1]
    Send-Keys 'admin{TAB}admin'
    Invoke-ClickAt $loginUi.Login[0] $loginUi.Login[1]

    Start-Sleep -Seconds 4
    Save-Capture $Out

    Write-Output "login sequence sent; if the login panel is still up, click Login again:"
    Write-Output "  -Action click -X $($loginUi.Login[0]) -Y $($loginUi.Login[1])"
  }
}
