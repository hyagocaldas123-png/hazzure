import re

path = r'c:\Codex2\gpt\HazzureInternal\src\Internal.cpp'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# Update HookedWndProc
old_wndproc_pattern = r'static LRESULT CALLBACK HookedWndProc\(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam\) \{.*?\n\}'
new_wndproc = '''static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_bImGuiInitialized) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        if (g_bShowMenu) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse) {
                switch (uMsg) {
                    case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                    case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                    case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                    case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL: case WM_MOUSEMOVE:
                        return true;
                }
            }
            if (io.WantCaptureKeyboard) {
                switch (uMsg) {
                    case WM_KEYDOWN: case WM_KEYUP:
                    case WM_SYSKEYDOWN: case WM_SYSKEYUP:
                    case WM_CHAR:
                        return true;
                }
            }
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}'''

text = re.sub(old_wndproc_pattern, new_wndproc, text, flags=re.DOTALL)

# Update hkPresent frame loop
old_present_block = '''    int menuKey = HazzureUI::g_cfg.menuKey > 0 ? HazzureUI::g_cfg.menuKey : VK_INSERT;
    if (GetAsyncKeyState(menuKey) & 1) {
        g_bShowMenu = !g_bShowMenu;
    }

    if (g_bImGuiInitialized && g_mainRenderTargetView) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_bShowMenu;

        HazzureUI::RenderFrame(shared, client, &g_bShowMenu, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }'''

new_present_block = '''    int menuKey = HazzureUI::g_cfg.menuKey > 0 ? HazzureUI::g_cfg.menuKey : VK_INSERT;
    if (GetAsyncKeyState(menuKey) & 1) {
        g_bShowMenu = !g_bShowMenu;
    }

    if (g_bImGuiInitialized && g_mainRenderTargetView) {
        ImGuiIO& io = ImGui::GetIO();

        if (g_bShowMenu) {
            io.MouseDrawCursor = true;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            ClipCursor(NULL);

            POINT mousePt;
            if (GetCursorPos(&mousePt) && g_hWindow) {
                ScreenToClient(g_hWindow, &mousePt);
                io.MousePos = ImVec2(static_cast<float>(mousePt.x), static_cast<float>(mousePt.y));
            }
        } else {
            io.MouseDrawCursor = false;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        HazzureUI::RenderFrame(shared, client, &g_bShowMenu, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }'''

assert old_present_block in text, "old_present_block not found"
text = text.replace(old_present_block, new_present_block)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)
print("Internal.cpp updated with mouse release and input capture fix!")
