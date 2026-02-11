#include "present.h"
#include "../sdk/structs.h"
#include "../sdk/memory.h"
#include "../sdk/entity.h"
#include "../features/esp.h"
#include "../features/aimbot.h"
#include "../features/triggerbot.h"
#include <d3d11.h>
#include <dxgi.h>
#include "minhook/MinHook.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* D3D11CreateDeviceAndSwapChainFn)(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext
);

static PresentFn oPresent = nullptr;
static D3D11CreateDeviceAndSwapChainFn oD3D11CreateDeviceAndSwapChain = nullptr;
static ID3D11Device* pDevice = nullptr;
static ID3D11DeviceContext* pContext = nullptr;
static ID3D11RenderTargetView* mainRenderTargetView = nullptr;
static IDXGISwapChain* pSwapChain = nullptr;
static HWND hWnd = nullptr;
bool bInitialized = false; // Make it accessible from dllmain
bool bShowMenu = false; // Make it accessible from dllmain
static WNDPROC oWndProc = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (bShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }
    
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        bShowMenu = !bShowMenu;
        return 0;
    }
    
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!bInitialized && pSwapChain) {
        ::pSwapChain = pSwapChain;
        
        // Get device from swap chain
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        if (!pDevice) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        // Get device context
        pDevice->GetImmediateContext(&pContext);
        if (!pContext) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        // Get swap chain description
        DXGI_SWAP_CHAIN_DESC sd = {};
        if (FAILED(pSwapChain->GetDesc(&sd))) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        hWnd = sd.OutputWindow;
        if (!hWnd) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }

        // Get back buffer
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer))) {
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        // Create render target view
        if (FAILED(pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView))) {
            pBackBuffer->Release();
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        pBackBuffer->Release();

        // Initialize ImGui
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;
        io.ConfigWindowsResizeFromEdges = false;

        // Initialize ImGui Win32
        if (!ImGui_ImplWin32_Init(hWnd)) {
            ImGui::DestroyContext();
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }
        
        // Initialize ImGui D3D11
        if (!ImGui_ImplDX11_Init(pDevice, pContext)) {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return oPresent ? oPresent(pSwapChain, SyncInterval, Flags) : S_OK;
        }

        // Hook window procedure
        oWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

        bInitialized = true;
    }

    if (bInitialized && pDevice && pContext && mainRenderTargetView) {
        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Always show a test overlay to verify hook is working
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
        ImGui::Begin("CS2 Hook Test", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Hook Active!");
        ImGui::Text("Press INSERT for menu");
        ImGui::End();

        // Run features
        ESP::Render();
        Aimbot::Update();
        Triggerbot::Update();

        // Render menu
        if (bShowMenu) {
            ImGui::Begin("CS2 Internal Cheat", &bShowMenu, ImGuiWindowFlags_AlwaysAutoResize);
            
            ImGui::Checkbox("ESP", &ESP::enabled);
            if (ESP::enabled) {
                ImGui::Checkbox("Box ESP", &ESP::boxEnabled);
                ImGui::Checkbox("Health Bar", &ESP::healthBarEnabled);
                ImGui::Checkbox("Name ESP", &ESP::nameEnabled);
            }

            ImGui::Separator();
            ImGui::Checkbox("Aimbot", &Aimbot::enabled);
            if (Aimbot::enabled) {
                ImGui::SliderFloat("FOV", &Aimbot::fov, 1.0f, 180.0f);
                ImGui::SliderFloat("Smooth", &Aimbot::smooth, 1.0f, 20.0f);
                ImGui::Text("Aiming at: Middle of top of ESP box");
            }

            ImGui::Separator();
            ImGui::Checkbox("Triggerbot", &Triggerbot::enabled);
            if (Triggerbot::enabled) {
                ImGui::SliderInt("Delay (ms)", &Triggerbot::delay, 0, 100);
            }

            ImGui::End();
        }

        // Render ImGui
        ImGui::Render();
        pContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    // Call original Present
    if (oPresent) {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }
    return S_OK;
}

HRESULT __stdcall hkD3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext
) {
    HRESULT result = oD3D11CreateDeviceAndSwapChain(
        pAdapter, DriverType, Software, Flags, pFeatureLevels,
        FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain,
        ppDevice, pFeatureLevel, ppImmediateContext
    );

    if (SUCCEEDED(result) && ppSwapChain && *ppSwapChain) {
        // Hook the swap chain's Present function
        void** pVTable = *(void***)*ppSwapChain;
        if (pVTable && !bInitialized) {
            // Present is at index 8 in IDXGISwapChain vtable
            DWORD oldProtect;
            if (VirtualProtect(&pVTable[8], sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                oPresent = (PresentFn)pVTable[8];
                pVTable[8] = (void*)hkPresent;
                VirtualProtect(&pVTable[8], sizeof(void*), oldProtect, &oldProtect);
            }
        }
    }

    return result;
}

bool Present::Hook() {
    // Try both methods - hook existing swap chain first, then hook creation as fallback
    bool hookedExisting = Present::FindAndHookSwapChain();
    
    // Always also hook D3D11CreateDeviceAndSwapChain as backup
    HMODULE hD3D11 = GetModuleHandleA("d3d11.dll");
    if (!hD3D11) {
        hD3D11 = LoadLibraryA("d3d11.dll");
        if (!hD3D11) return hookedExisting; // Return true if we hooked existing
    }

    // Hook D3D11CreateDeviceAndSwapChain
    oD3D11CreateDeviceAndSwapChain = (D3D11CreateDeviceAndSwapChainFn)GetProcAddress(
        hD3D11, "D3D11CreateDeviceAndSwapChain"
    );
    
    if (oD3D11CreateDeviceAndSwapChain) {
        void* pTarget = (void*)oD3D11CreateDeviceAndSwapChain;
        if (MH_CreateHook(
            pTarget,
            (void*)hkD3D11CreateDeviceAndSwapChain,
            (void**)&oD3D11CreateDeviceAndSwapChain
        ) == MH_OK) {
            MH_EnableHook(pTarget); // Don't fail if this doesn't work
        }
    }
    
    // Return true if we hooked existing swap chain, or if we at least hooked creation
    return hookedExisting || (oD3D11CreateDeviceAndSwapChain != nullptr);
}

bool Present::FindAndHookSwapChain() {
    // Simplified approach: Get Present address directly from dxgi.dll
    // This is more reliable than creating a temporary swap chain
    
    HMODULE hDXGI = GetModuleHandleA("dxgi.dll");
    if (!hDXGI) {
        hDXGI = LoadLibraryA("dxgi.dll");
        if (!hDXGI) return false;
    }
    
    // Try to get Present from IDXGISwapChain vtable
    // We'll create a minimal swap chain just to get the vtable address
    HMODULE hD3D11 = GetModuleHandleA("d3d11.dll");
    if (!hD3D11) {
        hD3D11 = LoadLibraryA("d3d11.dll");
        if (!hD3D11) return false;
    }
    
    typedef HRESULT(WINAPI* D3D11CreateDeviceFn)(
        IDXGIAdapter*,
        D3D_DRIVER_TYPE,
        HMODULE,
        UINT,
        const D3D_FEATURE_LEVEL*,
        UINT,
        UINT,
        ID3D11Device**,
        D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**
    );
    
    D3D11CreateDeviceFn pD3D11CreateDevice = (D3D11CreateDeviceFn)GetProcAddress(hD3D11, "D3D11CreateDevice");
    if (!pD3D11CreateDevice) return false;
    
    typedef HRESULT(WINAPI* CreateDXGIFactoryFn)(REFIID, void**);
    CreateDXGIFactoryFn pCreateDXGIFactory = (CreateDXGIFactoryFn)GetProcAddress(hDXGI, "CreateDXGIFactory");
    if (!pCreateDXGIFactory) return false;
    
    // Create temporary device and swap chain to get vtable
    ID3D11Device* pTempDevice = nullptr;
    ID3D11DeviceContext* pTempContext = nullptr;
    IDXGISwapChain* pTempSwapChain = nullptr;
    
    IDXGIFactory* pFactory = nullptr;
    if (FAILED(pCreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory))) {
        return false;
    }
    
    IDXGIAdapter* pAdapter = nullptr;
    if (FAILED(pFactory->EnumAdapters(0, &pAdapter))) {
        pFactory->Release();
        return false;
    }
    
    D3D_FEATURE_LEVEL featureLevel;
    if (FAILED(pD3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &pTempDevice, &featureLevel, &pTempContext))) {
        pAdapter->Release();
        pFactory->Release();
        return false;
    }
    
    // Get IDXGIDevice to create swap chain
    IDXGIDevice* pDXGIDevice = nullptr;
    if (FAILED(pTempDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice))) {
        pTempDevice->Release();
        pTempContext->Release();
        pAdapter->Release();
        pFactory->Release();
        return false;
    }
    
    IDXGIAdapter* pDeviceAdapter = nullptr;
    if (FAILED(pDXGIDevice->GetAdapter(&pDeviceAdapter))) {
        pDXGIDevice->Release();
        pTempDevice->Release();
        pTempContext->Release();
        pAdapter->Release();
        pFactory->Release();
        return false;
    }
    
    IDXGIFactory* pDeviceFactory = nullptr;
    if (FAILED(pDeviceAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pDeviceFactory))) {
        pDeviceAdapter->Release();
        pDXGIDevice->Release();
        pTempDevice->Release();
        pTempContext->Release();
        pAdapter->Release();
        pFactory->Release();
        return false;
    }
    
    // Create temporary swap chain to get vtable
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetDesktopWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    
    if (FAILED(pDeviceFactory->CreateSwapChain(pTempDevice, &sd, &pTempSwapChain))) {
        pDeviceFactory->Release();
        pDeviceAdapter->Release();
        pDXGIDevice->Release();
        pTempDevice->Release();
        pTempContext->Release();
        pAdapter->Release();
        pFactory->Release();
        return false;
    }
    
    // Get vtable address - Present is at index 8
    void** pVTable = *(void***)pTempSwapChain;
    void* pPresentAddr = pVTable[8];
    
    // Clean up temporary objects
    pTempSwapChain->Release();
    pDeviceFactory->Release();
    pDeviceAdapter->Release();
    pDXGIDevice->Release();
    pTempDevice->Release();
    pTempContext->Release();
    pAdapter->Release();
    pFactory->Release();
    
    // Hook Present directly - this catches ALL Present calls from any swap chain
    MH_STATUS status = MH_CreateHook(pPresentAddr, (void*)hkPresent, (void**)&oPresent);
    if (status != MH_OK) {
        return false;
    }
    
    status = MH_EnableHook(pPresentAddr);
    if (status != MH_OK) {
        MH_RemoveHook(pPresentAddr);
        return false;
    }
    
    return true;
}

void Present::Shutdown() {
    if (bInitialized) {
        // Restore window procedure
        if (hWnd && oWndProc) {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        if (mainRenderTargetView) {
            mainRenderTargetView->Release();
            mainRenderTargetView = nullptr;
        }
        
        if (pContext) {
            pContext->Release();
            pContext = nullptr;
        }
        
        if (pDevice) {
            pDevice->Release();
            pDevice = nullptr;
        }
        
        bInitialized = false;
    }
    
    // Restore swap chain vtable
    if (pSwapChain && oPresent) {
        void** pVTable = *(void***)pSwapChain;
        if (pVTable) {
            DWORD oldProtect;
            VirtualProtect(&pVTable[8], sizeof(void*), PAGE_READWRITE, &oldProtect);
            pVTable[8] = (void*)oPresent;
            VirtualProtect(&pVTable[8], sizeof(void*), oldProtect, &oldProtect);
        }
    }
    
    if (oD3D11CreateDeviceAndSwapChain) {
        MH_DisableHook((void*)oD3D11CreateDeviceAndSwapChain);
        MH_RemoveHook((void*)oD3D11CreateDeviceAndSwapChain);
    }
}

