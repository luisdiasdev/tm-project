<#
.SYNOPSIS
  Launch the WYD client, optionally click a point, and capture its window.

.DESCRIPTION
  PrintWindow returns a black image for the D3D9 client, so this captures the
  window rect from the screen after bringing the window to the foreground.
  That means another window overlapping the game will appear in the capture;
  keep the game on top.

  Coordinates are screen coordinates relative to the window rect. With the
  window at (0,0) this is also the captured image coordinate. The client area
  starts below the title bar (~26 px), so logical client Y + 26 = image Y.

.EXAMPLE
  powershell -File docs\tools\capture_and_click.ps1 `
    -Exe ..\client_748\TMProject_d.exe `
    -WorkingDir ..\client_748 `
    -ClickX 377 -ClickY 498 `
    -Out modal.png
#>
param(
  [Parameter(Mandatory = $true)][string]$Exe,
  [Parameter(Mandatory = $true)][string]$WorkingDir,
  [Parameter(Mandatory = $true)][string]$Out,
  [int]$ClickX = -1,
  [int]$ClickY = -1,
  [int]$Clicks = 1,
  [int]$WaitSeconds = 10,
  [int]$HoldMilliseconds = 180,
  [int]$SettleMilliseconds = 700
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WydCapture {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
  public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
[WydCapture]::SetProcessDPIAware() | Out-Null

$proc = Start-Process -FilePath $Exe -WorkingDirectory $WorkingDir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
  Start-Sleep -Milliseconds 500
  $proc.Refresh()
  if ($proc.MainWindowHandle -ne 0) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) {
  Write-Error "no game window (exited=$($proc.HasExited))"
  if (-not $proc.HasExited) { $proc.Kill() }
  exit 1
}

Start-Sleep -Seconds $WaitSeconds
[WydCapture]::ShowWindow($hwnd, 9) | Out-Null
[WydCapture]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Seconds 1

$rect = New-Object WydCapture+RECT
[WydCapture]::GetWindowRect($hwnd, [ref]$rect) | Out-Null

if ($ClickX -ge 0 -and $ClickY -ge 0) {
  for ($i = 0; $i -lt $Clicks; $i++) {
    [WydCapture]::SetCursorPos($rect.Left + $ClickX, $rect.Top + $ClickY) | Out-Null
    Start-Sleep -Milliseconds 300
    [WydCapture]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)  # left down
    Start-Sleep -Milliseconds $HoldMilliseconds
    [WydCapture]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)  # left up
    Start-Sleep -Milliseconds $SettleMilliseconds
  }
}

$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
$bmp = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bmp)
$graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0,
  (New-Object System.Drawing.Size $width, $height))
$graphics.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

"window=$($rect.Left),$($rect.Top) ${width}x${height} saved=$Out"
if (-not $proc.HasExited) { $proc.Kill() }
