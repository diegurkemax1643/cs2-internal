#pragma once
#include <Windows.h>
#include "offsets.h"
#include "structs.h"
#include "entity.h"

class Memory {
public:
    static uintptr_t GetClientBase() {
        return (uintptr_t)GetModuleHandleA("client.dll");
    }

    static uintptr_t GetEntityList() {
        uintptr_t clientBase = GetClientBase();
        if (!clientBase) return 0;
        return *(uintptr_t*)(clientBase + offsets::client_dll::dwEntityList);
    }

    static uintptr_t GetLocalPlayerPawn() {
        uintptr_t clientBase = GetClientBase();
        if (!clientBase) return 0;
        return *(uintptr_t*)(clientBase + offsets::client_dll::dwLocalPlayerPawn);
    }

    static view_matrix_t GetViewMatrix() {
        uintptr_t clientBase = GetClientBase();
        if (!clientBase) return view_matrix_t();
        
        uintptr_t viewMatrixPtr = clientBase + offsets::client_dll::dwViewMatrix;
        return *(view_matrix_t*)viewMatrixPtr;
    }

    static QAngle GetViewAngles() {
        uintptr_t localPawn = GetLocalPlayerPawn();
        if (!localPawn) return QAngle();
        
        // m_angEyeAngles is at offset 0x3DD0 in C_CSPlayerPawn
        return *(QAngle*)(localPawn + schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles);
    }

    static void SetViewAngles(const QAngle& angles) {
        uintptr_t localPawn = GetLocalPlayerPawn();
        if (!localPawn) return;
        
        QAngle normalized = angles;
        normalized.Normalize();
        
        // Clamp pitch to valid range (-89 to 89)
        if (normalized.pitch > 89.0f) normalized.pitch = 89.0f;
        if (normalized.pitch < -89.0f) normalized.pitch = -89.0f;
        
        // Write view angles to m_angEyeAngles
        *(QAngle*)(localPawn + schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles) = normalized;
        
        // Also try writing to controller view angles if available
        // Get local player controller
        uintptr_t clientBase = GetClientBase();
        if (clientBase) {
            uintptr_t localPlayerController = *(uintptr_t*)(clientBase + offsets::client_dll::dwLocalPlayerController);
            if (localPlayerController) {
                // Try to write to controller's view angles (if offset exists)
                // This is a fallback method
            }
        }
    }

    static Entity GetEntityByIndex(int index) {
        if (index < 1 || index > 64) return Entity(0);
        
        uintptr_t entityList = GetEntityList();
        if (!entityList) return Entity(0);

        // Entity list stride is 0x70, NOT 0x78
        uintptr_t listEntry = *(uintptr_t*)(entityList + 0x10 + 8 * (index >> 9));
        if (!listEntry) return Entity(0);

        uintptr_t controller = *(uintptr_t*)(listEntry + 0x70 * (index & 0x1FF));
        if (!controller) return Entity(0);

        uint32_t pawnHandle = *(uint32_t*)(controller + schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnHandle) return Entity(0);

        // Entity handles use & 0x7FFF to mask the index
        uint32_t pawnIndex = pawnHandle & 0x7FFF;
        uintptr_t pawnEntry = *(uintptr_t*)(entityList + 0x10 + 8 * (pawnIndex >> 9));
        if (!pawnEntry) return Entity(0);

        uintptr_t pawn = *(uintptr_t*)(pawnEntry + 0x70 * (pawnIndex & 0x1FF));
        return Entity(pawn);
    }
};

