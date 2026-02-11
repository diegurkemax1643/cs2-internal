#include "esp.h"
#include "../sdk/memory.h"
#include "../sdk/entity.h"
#include "../sdk/structs.h"
#include "imgui/imgui.h"
#include <d3d11.h>
#include <cstdio>
#include <cmath>

bool ESP::enabled = false;
bool ESP::boxEnabled = true;
bool ESP::corneredBoxEnabled = false;
bool ESP::boxFillEnabled = false;
bool ESP::glowEnabled = true;
bool ESP::healthBarEnabled = true;
bool ESP::nameEnabled = true;
bool ESP::distanceEnabled = true;
bool ESP::weaponEnabled = true;
bool ESP::headCircleEnabled = false;
bool ESP::linesEnabled = false;
int ESP::linesOrigin = 2; // Default: crosshair
bool ESP::viewEspEnabled = false;
bool ESP::teamEspEnabled = false;
bool ESP::chamsEnabled = false;

// Default colors (RGBA)
ImVec4 ESP::enemyBoxColor  = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
ImVec4 ESP::teamBoxColor   = ImVec4(0.0f, 0.4f, 1.0f, 1.0f); // Blue
ImVec4 ESP::enemyGlowColor = ImVec4(77.0f / 255.0f, 5.0f / 255.0f, 210.0f / 255.0f, 1.0f); // Purple glow (hardcoded from color picker: RGB 77, 5, 210)
ImVec4 ESP::teamGlowColor  = ImVec4(0.0f, 0.4f, 1.0f, 0.6f); // Blue glow
ImVec4 ESP::healthBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
ImVec4 ESP::nameColor      = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
ImVec4 ESP::enemyChamsColor = ImVec4(1.0f, 0.0f, 0.0f, 0.8f); // Red chams
ImVec4 ESP::teamChamsColor  = ImVec4(0.0f, 0.4f, 1.0f, 0.8f); // Blue chams
ImVec4 ESP::linesColor      = ImVec4(1.0f, 1.0f, 0.5f, 1.0f); // Yellow

void ESP::Render() {
    // Use background draw list - more reliable for overlay rendering
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    // Get screen size
    ImGuiIO& io = ImGui::GetIO();
    int screenWidth = (int)io.DisplaySize.x;
    int screenHeight = (int)io.DisplaySize.y;
    
    if (screenWidth <= 0 || screenHeight <= 0) return;

    if (!enabled) {
        return;
    }

    uintptr_t localPawn = Memory::GetLocalPlayerPawn();
    if (!localPawn) {
        return;
    }

    Entity localPlayer(localPawn);
    if (!localPlayer.IsValid()) {
        return;
    }

    int localTeam = localPlayer.GetTeam();
    if (localTeam == 0) {
        return;
    }

    view_matrix_t viewMatrix = Memory::GetViewMatrix();
    
    // Validate view matrix - check if it's valid (not all zeros)
    bool viewMatrixValid = false;
    for (int i = 0; i < 16; i++) {
        if (viewMatrix.m[i] != 0.0f) {
            viewMatrixValid = true;
            break;
        }
    }
    if (!viewMatrixValid) {
        return; // Invalid view matrix, game might be in menu/loading
    }

    // Iterate through entities (start from 1, index 0 is invalid)
    int entityCount = 0;
    
    // Use structured exception handling to catch access violations
    __try {
        for (int i = 1; i <= 64; i++) {
            Entity entity = Memory::GetEntityByIndex(i);
            if (!entity.IsValid()) continue;
            
            // Validate entity pointer before accessing
            if (IsBadReadPtr((void*)entity.pawn, sizeof(uintptr_t))) {
                continue;
            }
            
            if (!entity.IsAlive()) continue;
            
            int entityTeam = entity.GetTeam();
            bool isEnemy = (entityTeam != 0 && entityTeam != localTeam);
            
            // Internal Glow ESP - Write to game memory using CGlowProperty
            // Only write if entity pointer is valid
            if (glowEnabled && !IsBadWritePtr((void*)entity.pawn, sizeof(uintptr_t))) {
                __try {
                    if (isEnemy || (teamEspEnabled && !isEnemy)) {
                        // Get glow color based on team
                        ImVec4 glowColor = isEnemy ? enemyGlowColor : teamGlowColor;
                        
                        // Convert ImVec4 (0-1 range) to float values for glow
                        // Maximum brightness and intensity
                        float r = glowColor.x;
                        float g = glowColor.y;
                        float b = glowColor.z;
                        
                        // Increase brightness significantly for maximum visibility
                        float brightnessMultiplier = 2.0f; // Make colors much brighter
                        r = fminf(r * brightnessMultiplier, 1.0f);
                        g = fminf(g * brightnessMultiplier, 1.0f);
                        b = fminf(b * brightnessMultiplier, 1.0f);
                        
                        // Ensure minimum color intensity (if color is too dark, boost it)
                        float minIntensity = 0.5f; // Minimum RGB value for visibility
                        if (r < minIntensity && g < minIntensity && b < minIntensity) {
                            // If all colors are dark, boost them
                            r = fmaxf(r, minIntensity);
                            g = fmaxf(g, minIntensity);
                            b = fmaxf(b, minIntensity);
                        }
                        
                        // Full opacity - no transparency (alpha = 1.0)
                        float a = 1.0f; // 100% opacity, fully visible
                        
                        entity.EnableGlow(r, g, b, a);
                    } else {
                        entity.DisableGlow();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    // Ignore access violations when writing glow
                    continue;
                }
            } else if (!IsBadWritePtr((void*)entity.pawn, sizeof(uintptr_t))) {
                __try {
                    entity.DisableGlow();
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    continue;
                }
            }
        
            // Skip teammates unless team ESP is enabled
            if (!isEnemy && !teamEspEnabled) {
                continue;
            }
            
            // Validate entity pointer before reading position
            if (IsBadReadPtr((void*)entity.pawn, sizeof(uintptr_t))) {
                continue;
            }
            
            Vector3 origin;
            __try {
                origin = entity.GetPosition();
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                continue; // Skip if we can't read position
            }
            
            if (origin.x == 0.0f && origin.y == 0.0f && origin.z == 0.0f) continue; // Invalid position
        
        Vector2 feetScreen;
        
        if (!viewMatrix.WorldToScreen(origin, feetScreen, screenWidth, screenHeight)) {
            continue;
        }
        
        // Only skip if way off-screen
        if (feetScreen.x < -2000 || feetScreen.x > screenWidth + 2000 || 
            feetScreen.y < -2000 || feetScreen.y > screenHeight + 2000) {
            continue;
        }
        
        entityCount++;

        int health = entity.GetHealth();
        if (health <= 0) continue;

        // --- Simple, reliable box using fixed player height ---
        // CS2 standing player height is ~72 units; use this instead of bones
        const float playerHeightWorld = 72.0f;

        Vector3 headWorld = origin;
        headWorld.z += playerHeightWorld;

        Vector2 headScreen;
        if (!viewMatrix.WorldToScreen(headWorld, headScreen, screenWidth, screenHeight)) {
            continue;
        }

        // Box height is distance between projected head and feet on screen
        float boxHeight = fabsf(feetScreen.y - headScreen.y);
        if (boxHeight < 8.0f || boxHeight > 800.0f) {
            // Unreasonable size, skip this entity
            continue;
        }

        // Box width as fraction of height (40% like external project)
        float boxWidth = boxHeight * 0.4f;

        // Calculate box corners - center horizontally on feet position
        float left = feetScreen.x - boxWidth / 2.0f;
        float right = feetScreen.x + boxWidth / 2.0f;
        
        // Top is head, bottom is feet
        float bottom = feetScreen.y;
        float top = headScreen.y;
        
        // Ensure top is above bottom (top should have smaller Y value)
        if (top > bottom) {
            float temp = top;
            top = bottom;
            bottom = temp;
        }
        
        // Additional validation: ensure box is reasonable size
        if (boxHeight < 1.0f || boxHeight > 2000.0f || boxWidth < 1.0f || boxWidth > 2000.0f) {
            continue; // Skip this entity if box is invalid
        }
        
        // Ensure box coordinates are valid and reasonable
        if (left >= right || top >= bottom) {
            continue;
        }
        
        // Validate coordinates are finite numbers (not NaN or Inf)
        if (!isfinite(left) || !isfinite(right) || !isfinite(top) || !isfinite(bottom)) {
            continue;
        }
        
        // Clamp coordinates to reasonable screen bounds (with large margin)
        left = fmaxf(-1000.0f, fminf(left, screenWidth + 1000.0f));
        right = fmaxf(-1000.0f, fminf(right, screenWidth + 1000.0f));
        top = fmaxf(-1000.0f, fminf(top, screenHeight + 1000.0f));
        bottom = fmaxf(-1000.0f, fminf(bottom, screenHeight + 1000.0f));

        // Choose colors based on team
        ImVec4 currentBoxColor  = isEnemy ? enemyBoxColor  : teamBoxColor;
        ImVec4 currentGlowColor = isEnemy ? enemyGlowColor : teamGlowColor;

        // Draw box
        if (boxEnabled) {
            ImU32 color = IM_COL32(
                (int)(currentBoxColor.x * 255.0f),
                (int)(currentBoxColor.y * 255.0f),
                (int)(currentBoxColor.z * 255.0f),
                (int)(currentBoxColor.w * 255.0f)
            );
            
            // Draw box fill (if enabled)
            if (boxFillEnabled) {
                ImU32 fillColor = IM_COL32(
                    (int)(currentBoxColor.x * 255.0f),
                    (int)(currentBoxColor.y * 255.0f),
                    (int)(currentBoxColor.z * 255.0f),
                    (int)(currentBoxColor.w * 255.0f * 0.3f) // 30% alpha
                );
                drawList->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), fillColor);
            }
            
            if (corneredBoxEnabled) {
                // Draw cornered box (like external project)
                float cornerLength = fminf(boxWidth, boxHeight) * 0.3f; // 30% of smaller dimension
                cornerLength = fmaxf(cornerLength, 6.0f); // Minimum 6px
                
                // Top-left corner
                drawList->AddLine(ImVec2(left, top), ImVec2(left + cornerLength, top), color, 2.0f);
                drawList->AddLine(ImVec2(left, top), ImVec2(left, top + cornerLength), color, 2.0f);
                
                // Top-right corner
                drawList->AddLine(ImVec2(right, top), ImVec2(right - cornerLength, top), color, 2.0f);
                drawList->AddLine(ImVec2(right, top), ImVec2(right, top + cornerLength), color, 2.0f);
                
                // Bottom-left corner
                drawList->AddLine(ImVec2(left, bottom), ImVec2(left + cornerLength, bottom), color, 2.0f);
                drawList->AddLine(ImVec2(left, bottom), ImVec2(left, bottom - cornerLength), color, 2.0f);
                
                // Bottom-right corner
                drawList->AddLine(ImVec2(right, bottom), ImVec2(right - cornerLength, bottom), color, 2.0f);
                drawList->AddLine(ImVec2(right, bottom), ImVec2(right, bottom - cornerLength), color, 2.0f);
            } else {
                // Draw normal box outline
                drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), color, 0.0f, 0, 2.0f);
            }
        }
        
        // Draw Lines ESP (from origin point to player center)
        if (linesEnabled) {
            ImVec2 originPoint;
            switch (linesOrigin) {
                case 0: // Top
                    originPoint = ImVec2(screenWidth / 2.0f, 0.0f);
                    break;
                case 1: // Bottom
                    originPoint = ImVec2(screenWidth / 2.0f, (float)screenHeight);
                    break;
                case 2: // Crosshair (center)
                default:
                    originPoint = ImVec2(screenWidth / 2.0f, screenHeight / 2.0f);
                    break;
            }
            
            ImVec2 playerCenter = ImVec2((left + right) / 2.0f, (top + bottom) / 2.0f);
            ImU32 lineColor = IM_COL32(
                (int)(linesColor.x * 255.0f),
                (int)(linesColor.y * 255.0f),
                (int)(linesColor.z * 255.0f),
                (int)(linesColor.w * 255.0f)
            );
            drawList->AddLine(originPoint, playerCenter, lineColor, 1.5f);
        }
        
        
        // Draw head circle (fixed small size, doesn't scale with distance)
        if (headCircleEnabled) {
            ImU32 color = IM_COL32(
                (int)(currentBoxColor.x * 255.0f),
                (int)(currentBoxColor.y * 255.0f),
                (int)(currentBoxColor.z * 255.0f),
                (int)(currentBoxColor.w * 255.0f)
            );
            float headRadius = 8.0f; // Small fixed size
            drawList->AddCircle(ImVec2(headScreen.x, headScreen.y), headRadius, color, 32, 2.0f);
        }
        
        // Draw View ESP - shows direction enemy is looking
        if (viewEspEnabled) {
            __try {
                QAngle viewAngles = entity.GetViewAngles();
                
                // Convert angles to direction vector
                // Yaw: rotation around Z axis (left/right)
                // Pitch: rotation up/down
                float yawRad = viewAngles.yaw * (3.14159265359f / 180.0f);
                float pitchRad = viewAngles.pitch * (3.14159265359f / 180.0f);
                
                // Calculate direction vector from angles
                // Forward direction in 3D space
                Vector3 direction;
                direction.x = cosf(pitchRad) * cosf(yawRad);
                direction.y = cosf(pitchRad) * sinf(yawRad);
                direction.z = -sinf(pitchRad); // Negative because pitch down is positive in CS2
                
                // Normalize direction
                direction = direction.Normalize();
                
                // Calculate end point (50 units in front of head)
                float lineLength = 50.0f;
                Vector3 headWorldPos = headWorld;
                Vector3 endWorldPos = headWorldPos + (direction * lineLength);
                
                // Project end point to screen
                Vector2 endScreen;
                if (viewMatrix.WorldToScreen(endWorldPos, endScreen, screenWidth, screenHeight)) {
                    // Draw line from head to end point
                    ImU32 viewColor = IM_COL32(
                        (int)(currentBoxColor.x * 255.0f),
                        (int)(currentBoxColor.y * 255.0f),
                        (int)(currentBoxColor.z * 255.0f),
                        (int)(currentBoxColor.w * 255.0f)
                    );
                    drawList->AddLine(ImVec2(headScreen.x, headScreen.y), ImVec2(endScreen.x, endScreen.y), viewColor, 2.0f);
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                // Skip if we can't read view angles
            }
        }

        // Note: Glow ESP is now handled internally via memory writes above
        // The game's native glow system will render the glow effect

        // Draw health bar
        if (healthBarEnabled) {
            float healthPercent = health / 100.0f;
            float barHeight = bottom - top;
            float healthHeight = barHeight * healthPercent;
            
            // Health bar colors
            ImU32 healthColor = IM_COL32(0, 255, 0, 255); // Green
            if (health < 50) healthColor = IM_COL32(255, 255, 0, 255); // Yellow
            if (health < 25) healthColor = IM_COL32(255, 0, 0, 255); // Red
            
            // Draw health bar background (black)
            drawList->AddRectFilled(
                ImVec2(left - 7, top),
                ImVec2(left - 2, bottom),
                IM_COL32(0, 0, 0, 200)
            );
            
            // Draw health fill (from bottom up)
            drawList->AddRectFilled(
                ImVec2(left - 6, bottom - healthHeight),
                ImVec2(left - 3, bottom),
                healthColor
            );
            
            // Draw health bar border
            drawList->AddRect(
                ImVec2(left - 7, top),
                ImVec2(left - 2, bottom),
                IM_COL32(255, 255, 255, 255),
                0.0f, 0, 1.5f
            );
        }

        // Calculate distance from local player
        float distance = 0.0f;
        if (distanceEnabled || weaponEnabled) {
            Vector3 localOrigin = localPlayer.GetPosition();
            Vector3 delta = origin - localOrigin;
            distance = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        }
        
        // Draw name and distance
        if (nameEnabled || distanceEnabled) {
            char nameBuffer[128];
            if (nameEnabled && distanceEnabled) {
                sprintf_s(nameBuffer, "Player %d [%d HP] %.0fm", i, health, distance);
            } else if (nameEnabled) {
                sprintf_s(nameBuffer, "Player %d [%d HP]", i, health);
            } else {
                sprintf_s(nameBuffer, "%.0fm", distance);
            }
            
            // Calculate text size for background
            ImVec2 textSize = ImGui::CalcTextSize(nameBuffer);
            ImVec2 textPos(left + (boxWidth - textSize.x) / 2.0f, top - textSize.y - 2);
            
            // Draw text background for better visibility
            drawList->AddRectFilled(
                ImVec2(textPos.x - 2, textPos.y - 1),
                ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 1),
                IM_COL32(0, 0, 0, 180)
            );
            
            // Draw text with custom color
            ImU32 nameCol = IM_COL32(
                (int)(nameColor.x * 255.0f),
                (int)(nameColor.y * 255.0f),
                (int)(nameColor.z * 255.0f),
                (int)(nameColor.w * 255.0f)
            );
            drawList->AddText(textPos, nameCol, nameBuffer);
        }
        
        // Draw weapon name (simplified - would need actual weapon reading)
        if (weaponEnabled) {
            char weaponBuffer[64];
            sprintf_s(weaponBuffer, "Weapon");
            
            // Calculate text size for background
            ImVec2 textSize = ImGui::CalcTextSize(weaponBuffer);
            ImVec2 textPos(left + (boxWidth - textSize.x) / 2.0f, bottom + 2);
            
            // Draw text background for better visibility
            drawList->AddRectFilled(
                ImVec2(textPos.x - 2, textPos.y - 1),
                ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 1),
                IM_COL32(0, 0, 0, 180)
            );
            
            // Draw text with custom color
            ImU32 nameCol = IM_COL32(
                (int)(nameColor.x * 255.0f),
                (int)(nameColor.y * 255.0f),
                (int)(nameColor.z * 255.0f),
                (int)(nameColor.w * 255.0f)
            );
            drawList->AddText(textPos, nameCol, weaponBuffer);
        }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // If we get an exception (like access violation), just return
        // This prevents crashes when entities become invalid during round transitions
        return;
    }
}

