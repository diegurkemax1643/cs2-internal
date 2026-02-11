#pragma once

namespace Present {
    bool Initialize();
    bool Hook();
    void Shutdown();
    bool FindAndHookSwapChain(); // New function to find existing swap chain
}

