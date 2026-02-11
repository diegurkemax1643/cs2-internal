#include "aimbot.h"
#include "../sdk/memory.h"
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include "../sdk/offsets.h"
#include "imgui/imgui.h"
#include <Windows.h>
#include <cmath>
#include <cstdio>

bool Aimbot::enabled = false;
float Aimbot::fov = 5.0f;
float Aimbot::smooth = 5.0f;
int Aimbot::aimKey = VK_RBUTTON; // Default: Right mouse button
bool Aimbot::waitingForKey = false; // True when waiting for user to press a key for binding

QAngle Aimbot::CalculateAngle(const Vector3& from, const Vector3& to) {
    Vector3 delta = to - from;
    
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    float pitch = atan2f(-delta.z, hyp) * (180.0f / 3.14159265359f);
    float yaw = atan2f(delta.y, delta.x) * (180.0f / 3.14159265359f);
    
    return QAngle(pitch, yaw, 0.0f);
}

float Aimbot::GetFOV(const QAngle& angle, const QAngle& currentAngle) {
    float deltaPitch = angle.pitch - currentAngle.pitch;
    float deltaYaw = angle.yaw - currentAngle.yaw;
    
    // Normalize yaw difference
    while (deltaYaw > 180.0f) deltaYaw -= 360.0f;
    while (deltaYaw < -180.0f) deltaYaw += 360.0f;
    
    return sqrtf(deltaPitch * deltaPitch + deltaYaw * deltaYaw);
}

const char* Aimbot::GetKeyName(int vkCode) {
    // Convert VK code to readable name
    switch (vkCode) {
        case VK_LBUTTON: return "Left Mouse";
        case VK_RBUTTON: return "Right Mouse";
        case VK_MBUTTON: return "Middle Mouse";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        case VK_SPACE: return "Space";
        case VK_TAB: return "Tab";
        case VK_CAPITAL: return "Caps Lock";
        default:
            if (vkCode >= 'A' && vkCode <= 'Z') {
                static char keyName[2] = {0};
                keyName[0] = (char)vkCode;
                return keyName;
            }
            if (vkCode >= '0' && vkCode <= '9') {
                static char keyName[2] = {0};
                keyName[0] = (char)vkCode;
                return keyName;
            }
            if (vkCode >= VK_F1 && vkCode <= VK_F12) {
                static char fKeyName[4];
                sprintf(fKeyName, "F%d", vkCode - VK_F1 + 1);
                return fKeyName;
            }
            return "Unknown";
    }
}

Vector3 Aimbot::GetHeadPosition(const Entity& entity) {
    // Use same calculation as ESP to get head position
    // ESP calculates head position as: origin + Vector3(0, 0, 72) (player height)
    Vector3 origin = entity.GetPosition();
    if (origin.x == 0 && origin.y == 0 && origin.z == 0) return Vector3();
    
    // Player head is approximately 72 units above feet in CS2
    // This matches the ESP box calculation
    Vector3 headPos = origin + Vector3(0, 0, 72.0f);
    
    return headPos;
}

int Aimbot::GetPressedKey() {
    // Check mouse buttons first
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) return VK_LBUTTON;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) return VK_RBUTTON;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) return VK_MBUTTON;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) return VK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) return VK_XBUTTON2;
    
    // Check keyboard keys
    for (int vk = 'A'; vk <= 'Z'; vk++) {
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    for (int vk = '0'; vk <= '9'; vk++) {
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    
    // Check function keys
    for (int vk = VK_F1; vk <= VK_F12; vk++) {
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    
    // Check modifier keys
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) return VK_LSHIFT;
    if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) return VK_RSHIFT;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) return VK_LCONTROL;
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) return VK_RCONTROL;
    if (GetAsyncKeyState(VK_LMENU) & 0x8000) return VK_LMENU;
    if (GetAsyncKeyState(VK_RMENU) & 0x8000) return VK_RMENU;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) return VK_SPACE;
    if (GetAsyncKeyState(VK_TAB) & 0x8000) return VK_TAB;
    if (GetAsyncKeyState(VK_CAPITAL) & 0x8000) return VK_CAPITAL;
    
    return 0; // No key pressed
}

void Aimbot::UpdateKeyBinding() {
    if (!waitingForKey) return;
    
    int pressedKey = GetPressedKey();
    if (pressedKey != 0) {
        aimKey = pressedKey;
        waitingForKey = false;
    }
}

void Aimbot::Update() {
    if (!enabled) return;
    
    // Check if aim key is pressed
    if (!(GetAsyncKeyState(aimKey) & 0x8000)) return;

    uintptr_t localPawn = Memory::GetLocalPlayerPawn();
    if (!localPawn) return;

    Entity localPlayer(localPawn);
    if (!localPlayer.IsValid()) return;

    int localTeam = localPlayer.GetTeam();
    if (localTeam == 0) return;

    // Get view matrix for WorldToScreen
    view_matrix_t viewMatrix = Memory::GetViewMatrix();
    
    // Get screen size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    ImVec2 screenCenter(screenWidth / 2.0f, screenHeight / 2.0f);
    
    // Find best target using WorldToScreen (same method as ESP head circle)
    Entity bestTarget(0);
    float bestScreenDistance = fov * 10.0f; // Convert FOV to screen pixels (approximate)
    Vector3 bestHeadPos;

    for (int i = 1; i <= 64; i++) {
        Entity entity = Memory::GetEntityByIndex(i);
        if (!entity.IsValid() || !entity.IsAlive()) continue;
        if (entity.GetTeam() == localTeam) continue;

        // Get head position using same method as ESP (origin + 72 units)
        Vector3 headPos = GetHeadPosition(entity);
        
        if (headPos.x == 0 && headPos.y == 0 && headPos.z == 0) continue;

        // Convert head position to screen coordinates (same as ESP head circle)
        Vector2 headScreen;
        if (!viewMatrix.WorldToScreen(headPos, headScreen, screenWidth, screenHeight)) {
            continue;
        }
        
        // Calculate screen distance from crosshair to head circle position
        float screenDistance = sqrtf(
            (headScreen.x - screenCenter.x) * (headScreen.x - screenCenter.x) +
            (headScreen.y - screenCenter.y) * (headScreen.y - screenCenter.y)
        );
        
        // Check if within FOV (convert FOV degrees to pixels - approximate)
        float fovPixels = fov * 10.0f; // Rough conversion
        if (screenDistance < bestScreenDistance && screenDistance < fovPixels) {
            bestScreenDistance = screenDistance;
            bestTarget = entity;
            bestHeadPos = headPos;
        }
    }

    if (!bestTarget.IsValid()) return;

    // Convert best head position to screen coordinates
    Vector2 targetScreen;
    if (!viewMatrix.WorldToScreen(bestHeadPos, targetScreen, screenWidth, screenHeight)) {
        return;
    }
    
    // Calculate pixel difference from crosshair to head circle position
    float deltaX = targetScreen.x - screenCenter.x;
    float deltaY = targetScreen.y - screenCenter.y;
    
    // Convert pixel difference to angle change
    // Use same calculation as external project
    Vector3 localPos = localPlayer.GetPosition();
    QAngle targetAngle = CalculateAngle(localPos, bestHeadPos);
    
    // Get current view angles
    QAngle currentAngle = Memory::GetViewAngles();
    
    // Smooth the angle
    QAngle deltaAngle;
    deltaAngle.pitch = targetAngle.pitch - currentAngle.pitch;
    deltaAngle.yaw = targetAngle.yaw - currentAngle.yaw;
    
    // Normalize yaw
    while (deltaAngle.yaw > 180.0f) deltaAngle.yaw -= 360.0f;
    while (deltaAngle.yaw < -180.0f) deltaAngle.yaw += 360.0f;
    
    // Apply smoothing
    QAngle newAngle;
    newAngle.pitch = currentAngle.pitch + deltaAngle.pitch / smooth;
    newAngle.yaw = currentAngle.yaw + deltaAngle.yaw / smooth;
    newAngle.roll = 0.0f;
    newAngle.Normalize();
    
    // Clamp pitch to valid range
    if (newAngle.pitch > 89.0f) newAngle.pitch = 89.0f;
    if (newAngle.pitch < -89.0f) newAngle.pitch = -89.0f;
    
    // Write new angles to memory - try multiple methods
    Memory::SetViewAngles(newAngle);
    
    // Also try writing directly to ensure it works
    if (localPawn) {
        *(QAngle*)(localPawn + schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles) = newAngle;
    }
}

