#pragma once
#include "../sdk/structs.h"

class Entity; // Forward declaration

namespace Aimbot {
    extern bool enabled;
    extern float fov;
    extern float smooth;
    extern int aimKey; // Virtual key code for aim key (default: VK_RBUTTON = right mouse button)
    extern bool waitingForKey; // True when waiting for user to press a key for binding

    void Update();
    void UpdateKeyBinding(); // Call this to handle key binding
    QAngle CalculateAngle(const Vector3& from, const Vector3& to);
    float GetFOV(const QAngle& angle, const QAngle& currentAngle);
    const char* GetKeyName(int vkCode);
    int GetPressedKey(); // Returns the currently pressed key (0 if none)
    Vector3 GetHeadPosition(const Entity& entity); // Get head position from ESP box calculation
}

