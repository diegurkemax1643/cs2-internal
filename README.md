# CS2 Internal Cheat

An internal cheat for Counter-Strike 2 built with C++ using DLL injection.

## Project Structure

```
Internal/
├── dllmain.cpp              # DLL entry point
├── hooks/
│   ├── present.h            # Present hook header
│   └── present.cpp          # DirectX 11 Present hook implementation
├── features/
│   ├── esp.h/cpp            # ESP (boxes, health bars, names)
│   ├── aimbot.h/cpp         # Aimbot with FOV and smoothing
│   └── triggerbot.h/cpp     # Triggerbot
├── sdk/
│   ├── offsets.h            # Includes dump files
│   ├── structs.h            # Vector3, QAngle, view_matrix_t
│   ├── entity.h             # Entity class
│   └── memory.h             # Memory reading utilities
├── imgui/                   # [ADD THIS FOLDER]
└── minhook/                 # [ADD THIS FOLDER]
```

## Setup Instructions

1. **Add Dependencies:**
   - Copy ImGui files to `/imgui/` folder
   - Copy MinHook files to `/minhook/` folder

2. **Update Offsets:**
   - The project includes offsets from `C:/Users/MSZ/Downloads/output/`
   - Make sure these dump files are up to date
   - **IMPORTANT:** Update the view angles offset in `sdk/memory.h` (currently placeholder at 0x1530)

3. **Build Configuration:**
   - Set project type to DLL
   - Link against:
     - `d3d11.lib`
     - `dxgi.lib`
     - `minhook/minhook.lib`
   - Set C++ standard to C++17 or later

4. **Key Features:**
   - **ESP:** Boxes, health bars, and player names
   - **Aimbot:** FOV-based targeting with smoothing and bone selection
   - **Triggerbot:** Automatic shooting when crosshair is on enemy

## Controls

- **INSERT:** Toggle menu
- **END:** Unload cheat

## Important Notes

1. **Entity List Stride:** Uses 0x70 (NOT 0x78)
2. **Entity Handles:** Uses `& 0x7FFF` mask
3. **Player Index:** Starts from 1 (index 0 is invalid)
4. **Bone Matrix:** 32 bytes per bone
5. **View Matrix:** Column-major format

## TODO

- [ ] Find and update view angles offset in `sdk/memory.h`
- [ ] Verify bone indices (head = 6, chest = 5)
- [ ] Test and adjust ESP box calculations
- [ ] Fine-tune aimbot smoothing and FOV
- [ ] Add proper player name reading for ESP

## Disclaimer

This is for educational purposes only. Use at your own risk.

