#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include "minhook/MinHook.h"
#include "hooks/present.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "features/esp.h"
#include "features/config.h"
#include "utils/texture_loader.h"
#include <vector>
#include <string>
#include <wincodec.h>
#pragma comment(lib, "WindowsCodecs.lib")
#include "features/config.h"

extern bool bInitialized;
extern bool bShowMenu;

// Simple windowed overlay for testing
HWND g_hOverlayWnd = nullptr;
ID3D11Device* g_pOverlayDevice = nullptr;
ID3D11DeviceContext* g_pOverlayContext = nullptr;
IDXGISwapChain* g_pOverlaySwapChain = nullptr;
ID3D11RenderTargetView* g_pOverlayRenderTarget = nullptr;
bool g_bUseWindowedOverlay = false; // Set to true to test windowed overlay
ID3D11ShaderResourceView* g_pPlayerModelTexture = nullptr; // Player model image texture

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pOverlayDevice != nullptr && wParam != SIZE_MINIMIZED) {
            if (g_pOverlayRenderTarget) {
                g_pOverlayRenderTarget->Release();
                g_pOverlayRenderTarget = nullptr;
            }
            g_pOverlaySwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* pBackBuffer;
            g_pOverlaySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            g_pOverlayDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pOverlayRenderTarget);
            pBackBuffer->Release();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateWindowedOverlay() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, OverlayWndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, "CS2Overlay", nullptr };
    RegisterClassEx(&wc);
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Create window with proper flags for transparency and click-through
    HWND hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        wc.lpszClassName, 
        "CS2 Overlay", 
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    if (!hWnd) return false;
    
    // Set window to be transparent and click-through
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // Make window always on top
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    // Extended window style for click-through
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);
    
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pOverlaySwapChain, &g_pOverlayDevice, &featureLevel, &g_pOverlayContext) != S_OK) {
        DestroyWindow(hWnd);
        return false;
    }
    
    ID3D11Texture2D* pBackBuffer;
    g_pOverlaySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pOverlayDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pOverlayRenderTarget);
    pBackBuffer->Release();
    
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    
    // Style like external project
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 18.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(4.0f, 4.0f);
    style.WindowBorderSize = 0.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.094f, 0.102f, 0.125f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.102f, 0.114f, 0.133f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.137f, 0.149f, 0.169f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.137f, 0.149f, 0.169f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.765f, 1.0f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.765f, 1.0f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.945f, 0.945f, 0.945f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.765f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.29f, 0.62f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.29f, 0.62f, 1.0f, 1.0f);
    
    // CS2 Settings style - darker, more muted colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pOverlayDevice, g_pOverlayContext);
    
    // Load player model image - try to find it in project directory
    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(nullptr), modulePath, MAX_PATH);
    std::string exeDir = std::string(modulePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash + 1);
    }
    
    // Try different possible image file names
    std::vector<std::string> possiblePaths = {
        exeDir + "playermodel.png",
        exeDir + "playermodel.jpg",
        exeDir + "playermodel.jpeg",
        exeDir + "player_model.png",
        exeDir + "player_model.jpg",
        "C:\\Users\\mas\\Pictures\\Internal\\playermodel.png",
        "C:\\Users\\mas\\Pictures\\Internal\\playermodel.jpg",
        "C:\\Users\\mas\\Pictures\\Internal\\playermodel.jpeg",
        "C:\\Users\\mas\\Pictures\\Internal\\player_model.png",
        "C:\\Users\\mas\\Pictures\\Internal\\player_model.jpg"
    };
    
    for (const auto& path : possiblePaths) {
        g_pPlayerModelTexture = TextureLoader::LoadTextureFromFile(g_pOverlayDevice, g_pOverlayContext, path);
        if (g_pPlayerModelTexture) {
            break;
        }
    }
    
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    
    g_hOverlayWnd = hWnd;
    return true;
}

DWORD WINAPI OverlayRenderThread(LPVOID) {
    if (!CreateWindowedOverlay()) {
        return 1;
    }
    
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Check for INSERT key to toggle menu
        static bool insertPressed = false;
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!insertPressed) {
                bShowMenu = !bShowMenu;
                insertPressed = true;
            }
        } else {
            insertPressed = false;
        }
        
        // Run features (they handle their own enabled checks)
        ESP::Render();
        
        // Menu with Sidebar Design
        static int currentTab = 0; // 0: Visuals, 1: Colors, 2: Config, 3: Misc
        
        if (bShowMenu) {
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            ImGui::Begin("Undetected Only ESP", &bShowMenu, ImGuiWindowFlags_NoCollapse);
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            
            // Title bar with Toggle Menu button
            float titleBarHeight = 40.0f;
            ImU32 titleBarColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + titleBarHeight), titleBarColor);
            
            // Title
            ImGui::SetCursorPos(ImVec2(20.0f, 10.0f));
            ImVec4 titleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
            ImGui::SetWindowFontScale(1.3f);
            ImGui::Text("Undetected Only ESP");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
            
            // Toggle Menu button (top right)
            ImGui::SetCursorPos(ImVec2(windowSize.x - 130.0f, 8.0f));
            if (ImGui::Button("Toggle Menu", ImVec2(110, 25))) {
                bShowMenu = false;
            }
            
            // Sidebar background (below title bar)
            ImGui::SetCursorPosY(titleBarHeight + 5.0f);
            drawList->AddRectFilled(ImVec2(windowPos.x, windowPos.y + titleBarHeight), 
                ImVec2(windowPos.x + 180.0f, windowPos.y + windowSize.y), 
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.094f, 0.102f, 0.106f, 1.0f)));
            
            ImGui::SetCursorPosX(18.0f);
            ImGui::Spacing();
            
            // Sidebar buttons
            auto RenderSidebarButton = [&](const char* label, int index) {
                bool isSelected = currentTab == index;
                ImVec4 buttonColor = isSelected ? ImVec4(0.137f, 0.149f, 0.169f, 1.0f) : ImVec4(0, 0, 0, 0);
                ImVec4 textColor = isSelected ? ImVec4(0.0f, 0.765f, 1.0f, 1.0f) : ImVec4(0.69f, 0.722f, 0.757f, 1.0f);
                
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.137f, 0.149f, 0.169f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.137f, 0.149f, 0.169f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                
                ImGui::SetCursorPosX(8.0f);
                if (ImGui::Button(label, ImVec2(164, 40))) {
                    currentTab = index;
                }
                
                ImGui::PopStyleColor(4);
                ImGui::Spacing();
            };
            
            RenderSidebarButton("Visuals", 0);
            RenderSidebarButton("Colors", 1);
            RenderSidebarButton("Config", 2);
            RenderSidebarButton("Misc", 3);
            
            // Main content area - split into left (settings) and right (preview)
            ImGui::SetCursorPos(ImVec2(200, titleBarHeight + 15));
            
            // Calculate available width
            float contentWidth = windowSize.x - 210;
            float previewWidth = ESP::enabled ? 320.0f : 0.0f;
            float settingsWidth = contentWidth - previewWidth - (ESP::enabled ? 10.0f : 0.0f);
            
            // Left side: Settings
            ImGui::BeginChild("Settings", ImVec2(settingsWidth, windowSize.y - titleBarHeight - 25), false);
            
            if (currentTab == 0) {
                // Visuals Tab
                ImGui::Text("ESP Settings");
                ImGui::Spacing();
                
            ImGui::Checkbox("ESP", &ESP::enabled);
            if (ESP::enabled) {
                ImGui::Checkbox("Box ESP", &ESP::boxEnabled);
                    if (ESP::boxEnabled) {
                        ImGui::Indent();
                        ImGui::Checkbox("Cornered Box", &ESP::corneredBoxEnabled);
                        ImGui::Checkbox("Box Fill", &ESP::boxFillEnabled);
                        ImGui::Unindent();
                    }
                    ImGui::Checkbox("Glow ESP", &ESP::glowEnabled);
                    ImGui::Checkbox("Chams ESP", &ESP::chamsEnabled);
                    ImGui::Checkbox("Team ESP", &ESP::teamEspEnabled);
                ImGui::Checkbox("Health Bar", &ESP::healthBarEnabled);
                ImGui::Checkbox("Name ESP", &ESP::nameEnabled);
                    ImGui::Checkbox("Distance ESP", &ESP::distanceEnabled);
                    ImGui::Checkbox("Weapon ESP", &ESP::weaponEnabled);
                    ImGui::Checkbox("Head Circle", &ESP::headCircleEnabled);
                    ImGui::Checkbox("Lines ESP", &ESP::linesEnabled);
                    if (ESP::linesEnabled) {
                        ImGui::Indent();
                        const char* linesOrigins[] = { "Top", "Bottom", "Crosshair" };
                        ImGui::Combo("Origin", &ESP::linesOrigin, linesOrigins, 3);
                        ImGui::Unindent();
                    }
                    ImGui::Checkbox("View ESP", &ESP::viewEspEnabled);
                }
            }
            else if (currentTab == 1) {
                // Colors Tab
                ImGui::Text("ESP Colors");
                ImGui::Spacing();
                
                ImGui::Text("Enemies");
                ImGui::ColorEdit4("Enemy Box", (float*)&ESP::enemyBoxColor);
                ImGui::ColorEdit4("Enemy Glow", (float*)&ESP::enemyGlowColor);
                ImGui::ColorEdit4("Enemy Chams", (float*)&ESP::enemyChamsColor);
                ImGui::Separator();
                ImGui::Text("Teammates");
                ImGui::ColorEdit4("Team Box", (float*)&ESP::teamBoxColor);
                ImGui::ColorEdit4("Team Glow", (float*)&ESP::teamGlowColor);
                ImGui::ColorEdit4("Team Chams", (float*)&ESP::teamChamsColor);
            ImGui::Separator();
                ImGui::ColorEdit4("Lines", (float*)&ESP::linesColor);
                ImGui::ColorEdit4("Name", (float*)&ESP::nameColor);
            }
            else if (currentTab == 2) {
                // Config Tab
                ImGui::Text("Configuration");
                ImGui::Spacing();
                
                // Get available configs
                static std::vector<std::string> availableConfigs;
                static int selectedConfigIndex = -1;
                static bool refreshConfigs = true;
                
                if (refreshConfigs) {
                    availableConfigs = Config::GetAvailableConfigs();
                    refreshConfigs = false;
                }
                
                // Config selection
                ImGui::Text("Available Configs:");
                if (availableConfigs.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No configs found");
                } else {
                    // Create array of const char* for Combo
                    static std::vector<const char*> configNames;
                    configNames.clear();
                    for (const auto& config : availableConfigs) {
                        configNames.push_back(config.c_str());
                    }
                    
                    if (selectedConfigIndex >= (int)availableConfigs.size()) {
                        selectedConfigIndex = -1;
                    }
                    
                    if (ImGui::Combo("##ConfigList", &selectedConfigIndex, configNames.data(), (int)configNames.size())) {
                        // Config selected
                    }
                }
                
                ImGui::Spacing();
                
                // New config name input
                static char newConfigName[64] = "";
                ImGui::Text("New Config Name:");
                ImGui::InputText("##NewConfigName", newConfigName, sizeof(newConfigName));
                ImGui::Spacing();
                
                // Buttons
                if (ImGui::Button("Save Config", ImVec2(150, 30))) {
                    std::string filename = std::string(newConfigName);
                    if (filename.empty()) {
                        if (selectedConfigIndex >= 0 && selectedConfigIndex < (int)availableConfigs.size()) {
                            filename = availableConfigs[selectedConfigIndex];
                        } else {
                            filename = "config";
                        }
                    }
                    if (Config::Save(filename)) {
                        refreshConfigs = true;
                        ImGui::OpenPopup("Config Saved");
                    } else {
                        ImGui::OpenPopup("Config Error");
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Load Config", ImVec2(150, 30))) {
                    std::string filename;
                    if (selectedConfigIndex >= 0 && selectedConfigIndex < (int)availableConfigs.size()) {
                        filename = availableConfigs[selectedConfigIndex];
                    } else if (strlen(newConfigName) > 0) {
                        filename = std::string(newConfigName);
                    } else {
                        filename = "config";
                    }
                    if (Config::Load(filename)) {
                        ImGui::OpenPopup("Config Loaded");
                    } else {
                        ImGui::OpenPopup("Config Load Error");
                    }
                }
                
                ImGui::Spacing();
                
                if (selectedConfigIndex >= 0 && selectedConfigIndex < (int)availableConfigs.size()) {
                    if (ImGui::Button("Delete Config", ImVec2(150, 30))) {
                        ImGui::OpenPopup("Delete Config?");
                    }
                    
                    if (ImGui::BeginPopupModal("Delete Config?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Are you sure you want to delete:");
                        ImGui::Text("%s", availableConfigs[selectedConfigIndex].c_str());
                        ImGui::Spacing();
                        if (ImGui::Button("Yes", ImVec2(60, 0))) {
                            if (Config::DeleteConfig(availableConfigs[selectedConfigIndex])) {
                                refreshConfigs = true;
                                selectedConfigIndex = -1;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("No", ImVec2(60, 0))) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
                
                ImGui::Spacing();
                if (ImGui::Button("Refresh List", ImVec2(150, 30))) {
                    refreshConfigs = true;
                }
                
                // Popups
                if (ImGui::BeginPopupModal("Config Saved", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Configuration saved!");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                if (ImGui::BeginPopupModal("Config Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Failed to save configuration!");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                if (ImGui::BeginPopupModal("Config Loaded", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Configuration loaded!");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                if (ImGui::BeginPopupModal("Config Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Failed to load configuration!");
                    ImGui::Text("File not found or invalid.");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            else if (currentTab == 3) {
                // Misc Tab
                ImGui::Text("Miscellaneous");
                ImGui::Spacing();
                
                if (ImGui::Button("Unload DLL", ImVec2(150, 30))) {
                    // Use proper DLL unload method
                    HMODULE hModule = GetModuleHandleA("CS2Internal.dll");
                    if (hModule) {
                        CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
                            Sleep(100);
                            HMODULE hMod = (HMODULE)lpParam;
                            FreeLibraryAndExitThread(hMod, 0);
                            return 0;
                        }, hModule, 0, nullptr);
                    }
                }
            }
            
            ImGui::EndChild(); // End Settings child
            
            // Right side: ESP Preview (always visible if ESP enabled, in all tabs)
            if (ESP::enabled) {
                ImGui::SameLine();
                ImGui::BeginChild("Preview", ImVec2(previewWidth, windowSize.y - titleBarHeight - 25), false);
                
                ImGui::Text("ESP Preview");
                ImGui::Spacing();
                
                ImVec2 previewPos = ImGui::GetCursorScreenPos();
                ImVec2 previewSize = ImVec2(previewWidth - 20, 400);
                
                // Preview area background
                ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                drawList->AddRectFilled(previewPos, ImVec2(previewPos.x + previewSize.x, previewPos.y + previewSize.y), bgColor);
                drawList->AddRect(previewPos, ImVec2(previewPos.x + previewSize.x, previewPos.y + previewSize.y), 
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f)), 0.0f, 0, 1.0f);
                
                // Center of preview area
                ImVec2 center = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y * 0.5f);
                
                // Draw player model image if available, otherwise use wireframe
                if (g_pPlayerModelTexture) {
                    // Calculate image dimensions to fit nicely in preview
                    // Standard player model aspect ratio (approximately 0.4 width to height)
                    float boxHeight = 200.0f; // Larger size for better visibility
                    float boxWidth = boxHeight * 0.4f; // Standard player width ratio
                    
                    // Scale to fit preview area if needed
                    float maxHeight = previewSize.y * 0.7f;
                    float maxWidth = previewSize.x * 0.6f;
                    if (boxHeight > maxHeight) {
                        boxHeight = maxHeight;
                        boxWidth = boxHeight * 0.4f;
                    }
                    if (boxWidth > maxWidth) {
                        boxWidth = maxWidth;
                        boxHeight = boxWidth / 0.4f;
                    }
                    
                    ImVec2 boxMin = ImVec2(center.x - boxWidth * 0.5f, center.y - boxHeight * 0.5f);
                    ImVec2 boxMax = ImVec2(center.x + boxWidth * 0.5f, center.y + boxHeight * 0.5f);
                    ImVec2 boxHead = ImVec2(center.x, boxMin.y);
                    
                    // Draw the player model image - full image, no cropping (since background is removed)
                    drawList->AddImage((void*)g_pPlayerModelTexture, boxMin, boxMax, 
                        ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, 255));
                    
                    // Now draw ESP elements scaled to match the image exactly
                    // Get colors
                    ImVec4 colBox = ESP::enemyBoxColor;
                    ImU32 colBoxU32 = ImGui::ColorConvertFloat4ToU32(colBox);
                    
                    // Draw Box ESP (on top of player model image)
                    if (ESP::boxEnabled) {
                        if (ESP::boxFillEnabled) {
                            ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colBox.x, colBox.y, colBox.z, colBox.w * 0.3f));
                            drawList->AddRectFilled(boxMin, boxMax, fillColor);
                        }
                        
                        if (ESP::corneredBoxEnabled) {
                            float cornerLength = fminf(boxWidth, boxHeight) * 0.3f;
                            cornerLength = fmaxf(cornerLength, 6.0f);
                            
                            // Top-left corner
                            drawList->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x + cornerLength, boxMin.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x, boxMin.y + cornerLength), colBoxU32, 2.0f);
                            
                            // Top-right corner
                            drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x - cornerLength, boxMin.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + cornerLength), colBoxU32, 2.0f);
                            
                            // Bottom-left corner
                            drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + cornerLength, boxMax.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x, boxMax.y - cornerLength), colBoxU32, 2.0f);
                            
                            // Bottom-right corner
                            drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x - cornerLength, boxMax.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x, boxMax.y - cornerLength), colBoxU32, 2.0f);
                        } else {
                            drawList->AddRect(boxMin, boxMax, colBoxU32, 0.0f, 0, 2.0f);
                        }
                    }
                    
                    // Draw Health Bar
                    if (ESP::healthBarEnabled) {
                        float barWidth = 4.0f;
                        float healthPercent = 0.75f; // 75% health for preview
                        float barHeight = boxHeight * healthPercent;
                        float barX = boxMin.x - 10.0f;
                        float barY = boxMax.y - barHeight;
                        
                        ImU32 healthColor = IM_COL32(0, 255, 0, 255);
                        if (healthPercent < 0.5f) healthColor = IM_COL32(255, 255, 0, 255);
                        if (healthPercent < 0.25f) healthColor = IM_COL32(255, 0, 0, 255);
                        
                        drawList->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barWidth, boxMax.y), healthColor);
                        drawList->AddRect(ImVec2(barX, barY), ImVec2(barX + barWidth, boxMax.y), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
                    }
                    
                    // Draw Name
                    if (ESP::nameEnabled) {
                        ImVec2 namePos = ImVec2(boxHead.x, boxHead.y - 20.0f);
                        const char* previewName = "Preview Player [100 HP]";
                        ImVec2 textSize = ImGui::CalcTextSize(previewName);
                        namePos.x -= textSize.x * 0.5f;
                        
                        // Background
                        drawList->AddRectFilled(
                            ImVec2(namePos.x - 2, namePos.y - 1),
                            ImVec2(namePos.x + textSize.x + 2, namePos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(namePos, nameCol, previewName);
                    }
                    
                    // Draw Distance
                    if (ESP::distanceEnabled) {
                        ImVec2 distPos = ImVec2(boxHead.x, boxHead.y - (ESP::nameEnabled ? 40.0f : 20.0f));
                        const char* previewDist = "25m";
                        ImVec2 textSize = ImGui::CalcTextSize(previewDist);
                        distPos.x -= textSize.x * 0.5f;
                        
                        drawList->AddRectFilled(
                            ImVec2(distPos.x - 2, distPos.y - 1),
                            ImVec2(distPos.x + textSize.x + 2, distPos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(distPos, nameCol, previewDist);
                    }
                    
                    // Draw Weapon
                    if (ESP::weaponEnabled) {
                        ImVec2 weaponPos = ImVec2(boxHead.x, boxMax.y + 2.0f);
                        const char* previewWeapon = "AK-47";
                        ImVec2 textSize = ImGui::CalcTextSize(previewWeapon);
                        weaponPos.x -= textSize.x * 0.5f;
                        
                        drawList->AddRectFilled(
                            ImVec2(weaponPos.x - 2, weaponPos.y - 1),
                            ImVec2(weaponPos.x + textSize.x + 2, weaponPos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(weaponPos, nameCol, previewWeapon);
                    }
                    
                    // Draw Head Circle
                    if (ESP::headCircleEnabled) {
                        float headRadius = 8.0f;
                        drawList->AddCircle(boxHead, headRadius, colBoxU32, 32, 2.0f);
                    }
                    
                    // Draw Lines ESP
                    if (ESP::linesEnabled) {
                        ImVec2 originPoint;
                        switch (ESP::linesOrigin) {
                            case 0: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y); break;
                            case 1: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y); break;
                            case 2: default: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y * 0.5f); break;
                        }
                        ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(ESP::linesColor);
                        drawList->AddLine(originPoint, center, lineCol, 1.5f);
                    }
                } else {
                    // Fallback: Use wireframe if no image
                    float boxHeight = 120.0f;
                    float boxWidth = boxHeight * 0.4f;
                    ImVec2 boxMin = ImVec2(center.x - boxWidth, center.y - boxHeight * 0.5f);
                    ImVec2 boxMax = ImVec2(center.x + boxWidth, center.y + boxHeight * 0.5f);
                    ImVec2 boxHead = ImVec2(center.x, boxMin.y);
                    
                    // Draw wireframe model (same as before)
                    ImU32 wireframeColor = IM_COL32(0, 200, 255, 255);
                    ImU32 wireframeColorLight = IM_COL32(100, 220, 255, 180);
                    
                    float headRadius = boxWidth * 0.25f;
                    ImVec2 headCenter = ImVec2(center.x, boxMin.y + headRadius + 5.0f);
                    drawList->AddCircle(headCenter, headRadius, wireframeColor, 32, 2.0f);
                    drawList->AddLine(ImVec2(headCenter.x - headRadius * 0.5f, headCenter.y), 
                        ImVec2(headCenter.x + headRadius * 0.5f, headCenter.y), wireframeColorLight, 1.0f);
                    drawList->AddLine(ImVec2(headCenter.x, headCenter.y - headRadius * 0.5f), 
                        ImVec2(headCenter.x, headCenter.y + headRadius * 0.5f), wireframeColorLight, 1.0f);
                    
                    float neckLength = boxHeight * 0.08f;
                    ImVec2 neckTop = ImVec2(headCenter.x, headCenter.y + headRadius);
                    ImVec2 neckBottom = ImVec2(center.x, neckTop.y + neckLength);
                    drawList->AddLine(neckTop, neckBottom, wireframeColor, 2.0f);
                    
                    float torsoWidth = boxWidth * 0.5f;
                    float torsoHeight = boxHeight * 0.35f;
                    ImVec2 torsoMin = ImVec2(center.x - torsoWidth * 0.5f, neckBottom.y);
                    ImVec2 torsoMax = ImVec2(center.x + torsoWidth * 0.5f, neckBottom.y + torsoHeight);
                    drawList->AddRect(torsoMin, torsoMax, wireframeColor, 0.0f, 0, 2.0f);
                    
                    // Get colors for ESP elements
                    ImVec4 colBox = ESP::enemyBoxColor;
                    ImU32 colBoxU32 = ImGui::ColorConvertFloat4ToU32(colBox);
                    
                    // Draw Box ESP
                    if (ESP::boxEnabled) {
                        if (ESP::boxFillEnabled) {
                            ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colBox.x, colBox.y, colBox.z, colBox.w * 0.3f));
                            drawList->AddRectFilled(boxMin, boxMax, fillColor);
                        }
                        
                        if (ESP::corneredBoxEnabled) {
                            float cornerLength = fminf(boxWidth, boxHeight) * 0.3f;
                            cornerLength = fmaxf(cornerLength, 6.0f);
                            
                            drawList->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x + cornerLength, boxMin.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x, boxMin.y + cornerLength), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x - cornerLength, boxMin.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + cornerLength), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + cornerLength, boxMax.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x, boxMax.y - cornerLength), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x - cornerLength, boxMax.y), colBoxU32, 2.0f);
                            drawList->AddLine(ImVec2(boxMax.x, boxMax.y), ImVec2(boxMax.x, boxMax.y - cornerLength), colBoxU32, 2.0f);
                        } else {
                            drawList->AddRect(boxMin, boxMax, colBoxU32, 0.0f, 0, 2.0f);
                        }
                    }
                    
                    // Draw Health Bar
                    if (ESP::healthBarEnabled) {
                        float barWidth = 4.0f;
                        float healthPercent = 0.75f;
                        float barHeight = boxHeight * healthPercent;
                        float barX = boxMin.x - 10.0f;
                        float barY = boxMax.y - barHeight;
                        
                        ImU32 healthColor = IM_COL32(0, 255, 0, 255);
                        if (healthPercent < 0.5f) healthColor = IM_COL32(255, 255, 0, 255);
                        if (healthPercent < 0.25f) healthColor = IM_COL32(255, 0, 0, 255);
                        
                        drawList->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barWidth, boxMax.y), healthColor);
                        drawList->AddRect(ImVec2(barX, barY), ImVec2(barX + barWidth, boxMax.y), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
                    }
                    
                    // Draw Name
                    if (ESP::nameEnabled) {
                        ImVec2 namePos = ImVec2(boxHead.x, boxHead.y - 20.0f);
                        const char* previewName = "Preview Player [100 HP]";
                        ImVec2 textSize = ImGui::CalcTextSize(previewName);
                        namePos.x -= textSize.x * 0.5f;
                        
                        drawList->AddRectFilled(
                            ImVec2(namePos.x - 2, namePos.y - 1),
                            ImVec2(namePos.x + textSize.x + 2, namePos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(namePos, nameCol, previewName);
                    }
                    
                    // Draw Distance
                    if (ESP::distanceEnabled) {
                        ImVec2 distPos = ImVec2(boxHead.x, boxHead.y - (ESP::nameEnabled ? 40.0f : 20.0f));
                        const char* previewDist = "25m";
                        ImVec2 textSize = ImGui::CalcTextSize(previewDist);
                        distPos.x -= textSize.x * 0.5f;
                        
                        drawList->AddRectFilled(
                            ImVec2(distPos.x - 2, distPos.y - 1),
                            ImVec2(distPos.x + textSize.x + 2, distPos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(distPos, nameCol, previewDist);
                    }
                    
                    // Draw Weapon
                    if (ESP::weaponEnabled) {
                        ImVec2 weaponPos = ImVec2(boxHead.x, boxMax.y + 2.0f);
                        const char* previewWeapon = "AK-47";
                        ImVec2 textSize = ImGui::CalcTextSize(previewWeapon);
                        weaponPos.x -= textSize.x * 0.5f;
                        
                        drawList->AddRectFilled(
                            ImVec2(weaponPos.x - 2, weaponPos.y - 1),
                            ImVec2(weaponPos.x + textSize.x + 2, weaponPos.y + textSize.y + 1),
                            IM_COL32(0, 0, 0, 180)
                        );
                        
                        ImU32 nameCol = ImGui::ColorConvertFloat4ToU32(ESP::nameColor);
                        drawList->AddText(weaponPos, nameCol, previewWeapon);
                    }
                    
                    // Draw Head Circle
                    if (ESP::headCircleEnabled) {
                        float headRadius = 8.0f;
                        drawList->AddCircle(boxHead, headRadius, colBoxU32, 32, 2.0f);
            }
                    
                    // Draw Lines ESP
                    if (ESP::linesEnabled) {
                        ImVec2 originPoint;
                        switch (ESP::linesOrigin) {
                            case 0: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y); break;
                            case 1: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y); break;
                            case 2: default: originPoint = ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y * 0.5f); break;
                        }
                        ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(ESP::linesColor);
                        drawList->AddLine(originPoint, center, lineCol, 1.5f);
                    }
                }
                
                // Reserve space for preview
                ImGui::Dummy(previewSize);
                
                ImGui::EndChild();
            }
            
            ImGui::End();
        }
        
        ImGui::Render();
        // Clear with black (will be transparent due to LWA_COLORKEY)
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pOverlayContext->OMSetRenderTargets(1, &g_pOverlayRenderTarget, nullptr);
        g_pOverlayContext->ClearRenderTargetView(g_pOverlayRenderTarget, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        g_pOverlaySwapChain->Present(1, 0);
        
        // Update click-through: transparent when menu closed, clickable when menu open
        LONG_PTR exStyle = GetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE);
        if (bShowMenu) {
            // Menu open - allow clicks
            SetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        } else {
            // Menu closed - click-through
            SetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
        }
        
        // Update window to be click-through when menu is not open
        if (!bShowMenu) {
            LONG_PTR exStyle = GetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE);
            SetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
        } else {
            LONG_PTR exStyle = GetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE);
            SetWindowLongPtr(g_hOverlayWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        }
    }
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (g_pPlayerModelTexture) TextureLoader::ReleaseTexture(g_pPlayerModelTexture);
    if (g_pOverlayRenderTarget) g_pOverlayRenderTarget->Release();
    if (g_pOverlaySwapChain) g_pOverlaySwapChain->Release();
    if (g_pOverlayContext) g_pOverlayContext->Release();
    if (g_pOverlayDevice) g_pOverlayDevice->Release();
    
    return 0;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Wait for game to fully initialize
    Sleep(2000);

    // TEST MODE: Use windowed overlay to verify DLL is working
    // This creates a separate window overlay - if you see it, DLL is loaded and working
    // Set to false to use hooked overlay instead
    bool useWindowedOverlay = true; // Change to false to use hooked overlay
    
    if (useWindowedOverlay) {
        // Create windowed overlay thread
        HANDLE hOverlayThread = CreateThread(nullptr, 0, OverlayRenderThread, nullptr, 0, nullptr);
        if (hOverlayThread) {
            CloseHandle(hOverlayThread);
        }
        
        // Keep thread alive
        while (true) {
            Sleep(100);
            if (GetAsyncKeyState(VK_END) & 0x8000) {
                if (g_hOverlayWnd) {
                    PostMessage(g_hOverlayWnd, WM_QUIT, 0, 0);
                }
                break;
            }
        }
        return 0;
    }

    // HOOKED OVERLAY MODE: Hook into game's Present function
    // Initialize MinHook
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        return 1;
    }

    // Try to hook Present - retry multiple times
    bool hooked = false;
    for (int i = 0; i < 10; i++) {
        if (Present::Hook()) {
            hooked = true;
            break;
        }
        Sleep(300); // Wait longer between retries
    }
    
    if (!hooked) {
        MH_Uninitialize();
        return 1;
    }
    
    // Main loop - the Present hook will handle rendering
    while (true) {
        Sleep(100);
        
        // Check for unload key (END key)
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            break;
        }
    }

    // Cleanup
    Present::Shutdown();
    MH_Uninitialize();

    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // Create main thread
        HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}

