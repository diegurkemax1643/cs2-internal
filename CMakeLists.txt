cmake_minimum_required(VERSION 3.15)
project(CS2Internal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    dllmain.cpp
    hooks/present.cpp
    features/esp.cpp
    features/config.cpp
    utils/texture_loader.cpp
    features/aimbot.cpp
    features/triggerbot.cpp
    sdk/memory.cpp
)

# ImGui sources (only the files we need)
set(IMGUI_SOURCES
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imgui_impl_dx11.cpp"
    "imgui/imgui_impl_win32.cpp"
)

# MinHook sources (if available, otherwise link against library)
file(GLOB MINHOOK_SOURCES
    "minhook/*.c"
    "minhook/src/*.c"
)

# Headers
set(HEADERS
    hooks/present.h
    features/esp.h
    features/config.h
    features/aimbot.h
    features/triggerbot.h
    sdk/structs.h
    sdk/offsets.h
    sdk/entity.h
    sdk/memory.h
)

# Create DLL
if(MINHOOK_SOURCES)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${IMGUI_SOURCES} ${MINHOOK_SOURCES} ${HEADERS})
    message(STATUS "Building MinHook from source")
else()
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${IMGUI_SOURCES} ${HEADERS})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/minhook
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    gdiplus
)

# MinHook - try to find library or compile from source
file(GLOB MINHOOK_SOURCES
    "minhook/src/*.c"
    "minhook/src/*.cpp"
    "minhook/lib/*.c"
    "minhook/lib/*.cpp"
    "minhook/*.c"
    "minhook/*.cpp"
)

# Try to find pre-built library first
find_library(MINHOOK_LIB
    NAMES minhook libminhook MinHook libMinHook
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/minhook
        ${CMAKE_CURRENT_SOURCE_DIR}/minhook/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/minhook/x64/Release
        ${CMAKE_CURRENT_SOURCE_DIR}/minhook/lib/x64
        ${CMAKE_CURRENT_SOURCE_DIR}/minhook/Release
    NO_DEFAULT_PATH
)

if(MINHOOK_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MINHOOK_LIB})
    message(STATUS "Found MinHook library: ${MINHOOK_LIB}")
elseif(MINHOOK_SOURCES)
    message(STATUS "Building MinHook from source")
    # Add MinHook sources to the project
    target_sources(${PROJECT_NAME} PRIVATE ${MINHOOK_SOURCES})
else()
    message(WARNING "MinHook library not found. Attempting to link without explicit path...")
    # Try system path or just the name
    target_link_libraries(${PROJECT_NAME} PRIVATE minhook)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

