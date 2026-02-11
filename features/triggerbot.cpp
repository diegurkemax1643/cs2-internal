#include "triggerbot.h"
#include "../sdk/memory.h"
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include "../sdk/offsets.h"
#include "imgui/imgui.h"
#include <Windows.h>
#include <cmath>

bool Triggerbot::enabled = false;
int Triggerbot::delay = 0;

void Triggerbot::Update() {
    if (!enabled) return;

    uintptr_t localPawn = Memory::GetLocalPlayerPawn();
    if (!localPawn) return;

    Entity localPlayer(localPawn);
    if (!localPlayer.IsValid()) return;

    int localTeam = localPlayer.GetTeam();
    if (localTeam == 0) return;

    // Check if we're aiming at an enemy
    // In CS2, you can check crosshair entity or trace ray
    // For simplicity, we'll check entities near crosshair
    
    view_matrix_t viewMatrix = Memory::GetViewMatrix();
    
    // Get screen center (crosshair position)
    ImGuiIO& io = ImGui::GetIO();
    int screenWidth = (int)io.DisplaySize.x;
    int screenHeight = (int)io.DisplaySize.y;
    Vector2 crosshairPos(screenWidth / 2.0f, screenHeight / 2.0f);

    // Check entities near crosshair
    Entity targetEntity(0);
    float closestDistance = 50.0f; // Pixels

    for (int i = 1; i <= 64; i++) {
        Entity entity = Memory::GetEntityByIndex(i);
        if (!entity.IsValid() || !entity.IsAlive()) continue;
        if (entity.GetTeam() == localTeam) continue;

        Vector3 headPos = entity.GetBonePosition(6); // Head bone
        Vector2 screenPos;
        
        if (!viewMatrix.WorldToScreen(headPos, screenPos, screenWidth, screenHeight)) {
            continue;
        }

        // Calculate distance from crosshair
        float distance = sqrtf(
            (screenPos.x - crosshairPos.x) * (screenPos.x - crosshairPos.x) +
            (screenPos.y - crosshairPos.y) * (screenPos.y - crosshairPos.y)
        );

        if (distance < closestDistance) {
            closestDistance = distance;
            targetEntity = entity;
        }
    }

    if (targetEntity.IsValid()) {
        // Get attack button offset
        uintptr_t clientBase = Memory::GetClientBase();
        if (clientBase) {
            // buttons::attack is the offset for the attack button state
            // buttons is in cs2_dumper namespace, not offsets
            uintptr_t attackButton = clientBase + cs2_dumper::buttons::attack;
            
            // In CS2, button states are typically:
            // 0 = not pressed
            // 1 = pressed
            // We need to check the actual button state structure
            // For now, we'll use a simple approach
            int* buttonState = (int*)attackButton;
            if (buttonState) {
                // Add delay
                if (delay > 0) {
                    Sleep(delay);
                }
                
                // Trigger attack (set button state to pressed)
                *buttonState = 1;
                Sleep(10); // Hold for a bit
                *buttonState = 0; // Release
            }
        }
    }
}

