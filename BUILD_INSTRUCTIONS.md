# Build Instructions

## Required Dependencies

Before building, you need to add these folders to the project:

### 1. MinHook
- Download MinHook from: https://github.com/TsudaKageyu/minhook
- Extract and copy the `minhook` folder to: `C:\Users\mas\Pictures\Internal\minhook\`
- Required files:
  - `MinHook.h`
  - `minhook.lib` (or build it from source)

### 2. ImGui
- Download ImGui from: https://github.com/ocornut/imgui
- Extract and copy the `imgui` folder to: `C:\Users\mas\Pictures\Internal\imgui\`
- Required files:
  - `imgui.h`
  - `imgui.cpp`
  - `imgui_draw.cpp`
  - `imgui_widgets.cpp`
  - `imgui_tables.cpp`
  - `imgui_impl_dx11.h`
  - `imgui_impl_dx11.cpp`
  - `imgui_impl_win32.h`
  - `imgui_impl_win32.cpp`

## Build Steps

Once dependencies are added:

1. Open Developer Command Prompt for VS 2022
2. Navigate to project:
   ```
   cd C:\Users\mas\Pictures\Internal
   ```
3. Configure CMake:
   ```
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   ```
4. Build Release:
   ```
   cmake --build build --config Release
   ```

The DLL will be at: `C:\Users\mas\Pictures\Internal\build\bin\Release\CS2Internal.dll`

## Alternative: Visual Studio

1. Open the solution in Visual Studio
2. Set Configuration to **Release** and Platform to **x64**
3. Add include directories for `imgui` and `minhook` in project properties
4. Add `minhook.lib` to linker dependencies
5. Build Solution

