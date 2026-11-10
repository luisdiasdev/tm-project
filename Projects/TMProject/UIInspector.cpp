#include "pch.h"
#include "UIInspector.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx9.h"

// Forward-declare the ImGui Win32 message handler (the header keeps this in a #if 0 block
// to avoid dragging <windows.h> into every translation unit that includes it).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "SControl.h"
#include "SControlContainer.h"
#include "TMScene.h"
#include "TMGlobal.h"

#include <vector>
#include <cstdlib>

namespace UIInspector
{
    static bool s_bEnabled = false;
    static bool s_bInitialized = false;
    static bool s_bShowAll = false;
    static bool s_bFrozen = false;
    static float s_frozenMouseX = 0.0f;
    static float s_frozenMouseY = 0.0f;
    static std::vector<SControl*> s_frozenHits;

    static char s_highlightIDBuffer[32] = "";
    static unsigned int s_highlightedID = 0;
    static bool s_bHasHighlightedID = false;

    bool Initialize(HWND hWnd, IDirect3DDevice9* pd3dDevice)
    {
        if (s_bInitialized)
            return true;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplWin32_Init(hWnd))
            return false;
        if (!ImGui_ImplDX9_Init(pd3dDevice))
            return false;

        s_bInitialized = true;
        return true;
    }

    void Shutdown()
    {
        if (!s_bInitialized)
            return;

        s_bInitialized = false;
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void Toggle()
    {
        s_bEnabled = !s_bEnabled;
    }

    bool IsEnabled()
    {
        return s_bEnabled;
    }

    void ToggleShowAll()
    {
        s_bShowAll = !s_bShowAll;
    }

    bool IsShowAllEnabled()
    {
        return s_bShowAll;
    }

    void ToggleFreeze()
    {
        s_bFrozen = !s_bFrozen;
    }

    bool IsFrozen()
    {
        return s_bFrozen;
    }

    static const char* ControlTypeName(CONTROL_TYPE type)
    {
        switch (type)
        {
        case CONTROL_TYPE::CTRL_TYPE_NONE: return "None";
        case CONTROL_TYPE::CTRL_TYPE_CURSOR: return "Cursor";
        case CONTROL_TYPE::CTRL_TYPE_PANEL: return "Panel";
        case CONTROL_TYPE::CTRL_TYPE_BUTTON: return "Button";
        case CONTROL_TYPE::CTRL_TYPE_CHECKBOX: return "CheckBox";
        case CONTROL_TYPE::CTRL_TYPE_RADIOBUTTON: return "RadioButton";
        case CONTROL_TYPE::CTRL_TYPE_RADIOBUTTONSET: return "RadioButtonSet";
        case CONTROL_TYPE::CTRL_TYPE_LISTBOX: return "ListBox";
        case CONTROL_TYPE::CTRL_TYPE_LISTBOXITEM: return "ListBoxItem";
        case CONTROL_TYPE::CTRL_TYPE_MESSAGEBOX: return "MessageBox";
        case CONTROL_TYPE::CTRL_TYPE_MESSAGEPANEL: return "MessagePanel";
        case CONTROL_TYPE::CTRL_TYPE_PROGRESSBAR: return "ProgressBar";
        case CONTROL_TYPE::CTRL_TYPE_SCROLLBAR: return "ScrollBar";
        case CONTROL_TYPE::CTRL_TYPE_TEXT: return "Text";
        case CONTROL_TYPE::CTRL_TYPE_EDITABLETEXT: return "EditableText";
        case CONTROL_TYPE::CTRL_TYPE_DIALOG: return "Dialog";
        case CONTROL_TYPE::CTRL_TYPE_3DOBJ: return "3DObj";
        case CONTROL_TYPE::CTRL_TYPE_GRID: return "Grid";
        default: return "Unknown";
        }
    }

    static void CollectControlsAt(SControlContainer* pContainer, float mouseX, float mouseY, std::vector<SControl*>& outHits)
    {
        if (!pContainer || !pContainer->m_pControlRoot)
            return;

        SControl* pCurrent = pContainer->m_pControlRoot;
        SControl* pRoot = pContainer->m_pControlRoot;
        TMVector2 parentPos{};

        do
        {
            if (!pCurrent->m_cDeleted)
            {
                float absX = parentPos.x + pCurrent->m_nPosX;
                float absY = parentPos.y + pCurrent->m_nPosY;

                if (mouseX >= absX && mouseX < absX + pCurrent->m_nWidth &&
                    mouseY >= absY && mouseY < absY + pCurrent->m_nHeight)
                {
                    outHits.push_back(pCurrent);
                }

                if (pCurrent->m_pDown)
                {
                    parentPos.x += pCurrent->m_nPosX;
                    parentPos.y += pCurrent->m_nPosY;
                    pCurrent = static_cast<SControl*>(pCurrent->m_pDown);
                    continue;
                }
            }

            do
            {
                if (pCurrent->m_pNextLink != nullptr)
                {
                    pCurrent = static_cast<SControl*>(pCurrent->m_pNextLink);
                    break;
                }

                pCurrent = static_cast<SControl*>(pCurrent->m_pTop);
                if (pCurrent != nullptr)
                {
                    parentPos.x -= pCurrent->m_nPosX;
                    parentPos.y -= pCurrent->m_nPosY;
                }
            } while (pCurrent != pRoot && pCurrent != nullptr);
        } while (pCurrent != pRoot && pCurrent != nullptr);
    }

    struct ControlRect
    {
        float x, y, w, h;
        unsigned int id;
        CONTROL_TYPE type;
    };

    static void CollectControlRects(SControlContainer* pContainer, std::vector<ControlRect>& outRects)
    {
        if (!pContainer || !pContainer->m_pControlRoot)
            return;

        SControl* pCurrent = pContainer->m_pControlRoot;
        SControl* pRoot = pContainer->m_pControlRoot;
        TMVector2 parentPos{};

        do
        {
            if (!pCurrent->m_cDeleted)
            {
                ControlRect rc{};
                rc.x = parentPos.x + pCurrent->m_nPosX;
                rc.y = parentPos.y + pCurrent->m_nPosY;
                rc.w = pCurrent->m_nWidth;
                rc.h = pCurrent->m_nHeight;
                rc.id = pCurrent->m_dwControlID;
                rc.type = pCurrent->m_eCtrlType;
                outRects.push_back(rc);

                if (pCurrent->m_pDown)
                {
                    parentPos.x += pCurrent->m_nPosX;
                    parentPos.y += pCurrent->m_nPosY;
                    pCurrent = static_cast<SControl*>(pCurrent->m_pDown);
                    continue;
                }
            }

            do
            {
                if (pCurrent->m_pNextLink != nullptr)
                {
                    pCurrent = static_cast<SControl*>(pCurrent->m_pNextLink);
                    break;
                }

                pCurrent = static_cast<SControl*>(pCurrent->m_pTop);
                if (pCurrent != nullptr)
                {
                    parentPos.x -= pCurrent->m_nPosX;
                    parentPos.y -= pCurrent->m_nPosY;
                }
            } while (pCurrent != pRoot && pCurrent != nullptr);
        } while (pCurrent != pRoot && pCurrent != nullptr);
    }

    static bool FindControlRectByID(SControlContainer* pContainer, unsigned int dwID, ControlRect& outRect)
    {
        if (!pContainer || !pContainer->m_pControlRoot)
            return false;

        SControl* pCurrent = pContainer->m_pControlRoot;
        SControl* pRoot = pContainer->m_pControlRoot;
        TMVector2 parentPos{};

        do
        {
            if (!pCurrent->m_cDeleted)
            {
                if (pCurrent->m_dwControlID == dwID)
                {
                    outRect.x = parentPos.x + pCurrent->m_nPosX;
                    outRect.y = parentPos.y + pCurrent->m_nPosY;
                    outRect.w = pCurrent->m_nWidth;
                    outRect.h = pCurrent->m_nHeight;
                    outRect.id = pCurrent->m_dwControlID;
                    outRect.type = pCurrent->m_eCtrlType;
                    return true;
                }

                if (pCurrent->m_pDown)
                {
                    parentPos.x += pCurrent->m_nPosX;
                    parentPos.y += pCurrent->m_nPosY;
                    pCurrent = static_cast<SControl*>(pCurrent->m_pDown);
                    continue;
                }
            }

            do
            {
                if (pCurrent->m_pNextLink != nullptr)
                {
                    pCurrent = static_cast<SControl*>(pCurrent->m_pNextLink);
                    break;
                }

                pCurrent = static_cast<SControl*>(pCurrent->m_pTop);
                if (pCurrent != nullptr)
                {
                    parentPos.x -= pCurrent->m_nPosX;
                    parentPos.y -= pCurrent->m_nPosY;
                }
            } while (pCurrent != pRoot && pCurrent != nullptr);
        } while (pCurrent != pRoot && pCurrent != nullptr);

        return false;
    }

    void NewFrame()
    {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    static void DrawControlList(const std::vector<SControl*>& hits)
    {
        if (hits.empty())
        {
            ImGui::TextDisabled("No control under cursor");
            return;
        }

        ImGui::Separator();
        ImGui::Text("Controls under cursor: %zu", hits.size());

        // Display from topmost (last hit) down to root.
        for (int i = static_cast<int>(hits.size()) - 1; i >= 0; --i)
        {
            SControl* pControl = hits[i];
            if (pControl == nullptr || pControl->m_cDeleted)
            {
                ImGui::TextDisabled("[deleted]");
                continue;
            }

            ImGui::PushID(static_cast<int>(pControl->m_dwUniqueID));

            if (ImGui::TreeNode("Control", "%u (0x%X) - %s",
                pControl->m_dwControlID, pControl->m_dwControlID, ControlTypeName(pControl->m_eCtrlType)))
            {
                ImGui::Text("Unique ID: %u", pControl->m_dwUniqueID);
                ImGui::Text("Rect: %.1f, %.1f, %.1f x %.1f",
                    pControl->m_nPosX, pControl->m_nPosY, pControl->m_nWidth, pControl->m_nHeight);
                ImGui::Text("Visible: %d  Enabled: %d  Focused: %d  Over: %d",
                    pControl->IsVisible(), pControl->m_bEnable, pControl->IsFocused(), pControl->IsOver());

                if (pControl->m_pTop)
                {
                    SControl* pParentControl = static_cast<SControl*>(pControl->m_pTop);
                    ImGui::Text("Parent: %u (%s)",
                        pParentControl->m_dwControlID, ControlTypeName(pParentControl->m_eCtrlType));
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    void Render()
    {
        if (!s_bInitialized)
            return;

        NewFrame();

        if (!s_bEnabled)
        {
            ImGui::EndFrame();
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        float liveMouseX = io.MousePos.x;
        float liveMouseY = io.MousePos.y;

        // Update the frozen snapshot only when we are not frozen.
        if (!s_bFrozen)
        {
            s_frozenMouseX = liveMouseX;
            s_frozenMouseY = liveMouseY;
            s_frozenHits.clear();
            if (g_pCurrentScene && g_pCurrentScene->m_pControlContainer)
                CollectControlsAt(g_pCurrentScene->m_pControlContainer, liveMouseX, liveMouseY, s_frozenHits);
        }

        float displayMouseX = s_frozenMouseX;
        float displayMouseY = s_frozenMouseY;

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("UI Inspector (F10 = toggle, F11 = show all, F12 = freeze)", &s_bEnabled))
        {
            if (s_bFrozen)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "FROZEN - press F12 to unfreeze");
            }
            ImGui::Text("Mouse: %.0f, %.0f", displayMouseX, displayMouseY);

            ImGui::Separator();
            ImGui::Text("Highlight by ID");
            bool bEnterPressed = ImGui::InputText("##HighlightID", s_highlightIDBuffer, sizeof(s_highlightIDBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue, nullptr, nullptr);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Decimal or 0xHEX");
            ImGui::SameLine();
            if (ImGui::Button("Highlight") || bEnterPressed)
            {
                char* pEnd = nullptr;
                unsigned long value = strtoul(s_highlightIDBuffer, &pEnd, 0);
                if (pEnd != s_highlightIDBuffer && value != 0)
                {
                    s_highlightedID = static_cast<unsigned int>(value);
                    s_bHasHighlightedID = true;
                }
                else
                {
                    s_bHasHighlightedID = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
            {
                s_highlightedID = 0;
                s_bHasHighlightedID = false;
                s_highlightIDBuffer[0] = '\0';
            }
            if (s_bHasHighlightedID)
            {
                ImGui::Text("Highlighting ID: %u (0x%X)", s_highlightedID, s_highlightedID);
            }

            if (g_pCurrentScene && g_pCurrentScene->m_pControlContainer)
            {
                DrawControlList(s_frozenHits);

                if (s_bShowAll)
                {
                    ImGui::Separator();
                    ImGui::Text("Show all controls enabled (F11)");
                }
            }
            else
            {
                ImGui::TextDisabled("No active scene/container");
            }
        }
        ImGui::End();

        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        if (s_bShowAll && g_pCurrentScene && g_pCurrentScene->m_pControlContainer)
        {
            std::vector<ControlRect> rects;
            CollectControlRects(g_pCurrentScene->m_pControlContainer, rects);

            for (const auto& rc : rects)
            {
                if (rc.w <= 0.0f || rc.h <= 0.0f)
                    continue;

                ImU32 col = IM_COL32(0, 255, 0, 160);
                drawList->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), col);

                char szLabel[32];
                sprintf_s(szLabel, "%u", rc.id);
                drawList->AddText(ImVec2(rc.x + 2.0f, rc.y + 2.0f), IM_COL32(255, 255, 255, 255), szLabel);
            }
        }

        if (s_bHasHighlightedID && g_pCurrentScene && g_pCurrentScene->m_pControlContainer)
        {
            ControlRect rc{};
            if (FindControlRectByID(g_pCurrentScene->m_pControlContainer, s_highlightedID, rc) &&
                rc.w > 0.0f && rc.h > 0.0f)
            {
                ImU32 col = IM_COL32(0, 255, 0, 255);
                drawList->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), col, 0.0f, 0, 2.0f);

                char szLabel[64];
                sprintf_s(szLabel, "%u (0x%X)", rc.id, rc.id);
                drawList->AddText(ImVec2(rc.x + 2.0f, rc.y + 2.0f), IM_COL32(255, 255, 255, 255), szLabel);
            }
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    bool HandleInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!s_bInitialized)
            return false;
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) != 0;
    }

    void OnDeviceLost()
    {
        if (s_bInitialized)
            ImGui_ImplDX9_InvalidateDeviceObjects();
    }

    void OnDeviceReset(IDirect3DDevice9* pd3dDevice)
    {
        if (s_bInitialized)
            ImGui_ImplDX9_CreateDeviceObjects();
    }

    void LogMissingControl(unsigned int dwID)
    {
        (void)dwID;
        // Implemented in SControlContainer.cpp to keep logging centralized.
    }
}
