#pragma once
#include <Windows.h>
#include "structs.h"
#include "offsets.h"

class Entity {
public:
    uintptr_t pawn;

    Entity(uintptr_t pawn) : pawn(pawn) {}

    bool IsValid() const {
        return pawn != 0;
    }

    int GetHealth() const {
        if (!pawn) return 0;
        return *(int*)(pawn + schemas::client_dll::C_BaseEntity::m_iHealth);
    }

    int GetTeam() const {
        if (!pawn) return 0;
        return *(int*)(pawn + schemas::client_dll::C_BaseEntity::m_iTeamNum);
    }

    Vector3 GetPosition() const {
        if (!pawn) return Vector3();
        // m_vecAbsOrigin is in CGameSceneNode, accessed through m_pGameSceneNode
        uintptr_t sceneNode = *(uintptr_t*)(pawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!sceneNode) return Vector3();
        return *(Vector3*)(sceneNode + schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);
    }

    Vector3 GetBonePosition(int boneIndex) const {
        if (!pawn) return Vector3();
        
        // Get scene node
        uintptr_t sceneNode = *(uintptr_t*)(pawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!sceneNode) return Vector3();
        
        // Get skeleton instance (CSkeletonInstance extends CGameSceneNode)
        // m_modelState is at offset 0x160 in CSkeletonInstance
        uintptr_t modelState = sceneNode + schemas::client_dll::CSkeletonInstance::m_modelState;
        if (!modelState) return Vector3();
        
        // Bone matrices are typically at modelState + some offset
        // In Source 2, bones are accessed through the model state
        // Common pattern: bone matrices start at modelState + 0x80 or similar
        // For CS2, the bone array is typically at modelState + 0x80
        uintptr_t boneArray = modelState + 0x80; // Bone array offset (may need adjustment)
        if (!boneArray) return Vector3();
        
        // Bone matrix stride is 32 bytes (4x4 matrix, but stored as 3x4 + padding)
        float* boneMatrix = (float*)(boneArray + boneIndex * 32);
        if (!boneMatrix) return Vector3();
        
        // Extract position from bone matrix (columns 0,1,2,3 where 3 is translation)
        return Vector3(boneMatrix[3], boneMatrix[7], boneMatrix[11]);
    }

    bool IsDormant() const {
        if (!pawn) return true;
        // m_bDormant is in CGameSceneNode
        uintptr_t sceneNode = *(uintptr_t*)(pawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!sceneNode) return true;
        return *(bool*)(sceneNode + schemas::client_dll::CGameSceneNode::m_bDormant);
    }

    bool IsAlive() const {
        return GetHealth() > 0 && !IsDormant();
    }
    
    QAngle GetViewAngles() const {
        if (!pawn) return QAngle();
        // m_angEyeAngles is at offset 0x3DD0 in C_CSPlayerPawn
        return *(QAngle*)(pawn + schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles);
    }
    
    // Glow ESP - Internal game glow system using CGlowProperty
    // Based on CS2-External-C++ implementation
    // CGlowProperty is located at C_BaseModelEntity + 0xCC0
    void SetGlow(float r, float g, float b, float a) const {
        if (!pawn) return;
        
        // C_BaseModelEntity::m_Glow offset (from cs2-dumper)
        constexpr uintptr_t uGlowPropertyOffset = 0xCC0;
        
        // CGlowProperty structure offsets
        constexpr uintptr_t uGlowColorOverrideOffset = 0x40; // Color m_glowColorOverride (4 floats: RGBA)
        constexpr uintptr_t uGlowingOffset = 0x51; // bool m_bGlowing
        
        // Get CGlowProperty address
        uintptr_t glowPropertyAddress = pawn + uGlowPropertyOffset;
        
        // Validate address is reasonable
        if (IsBadWritePtr((void*)glowPropertyAddress, sizeof(uintptr_t))) {
            return;
        }
        
        __try {
            // Write color override (RGBA as 4 floats)
            *(float*)(glowPropertyAddress + uGlowColorOverrideOffset + 0x0) = r;
            *(float*)(glowPropertyAddress + uGlowColorOverrideOffset + 0x4) = g;
            *(float*)(glowPropertyAddress + uGlowColorOverrideOffset + 0x8) = b;
            *(float*)(glowPropertyAddress + uGlowColorOverrideOffset + 0xC) = a;
            
            // Enable glowing
            *(bool*)(glowPropertyAddress + uGlowingOffset) = true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            // Skip if access violation
        }
    }
    
    void EnableGlow(float r, float g, float b, float a) const {
        SetGlow(r, g, b, a);
    }
    
    void DisableGlow() const {
        if (!pawn) return;
        
        // C_BaseModelEntity::m_Glow offset
        constexpr uintptr_t uGlowPropertyOffset = 0xCC0;
        constexpr uintptr_t uGlowingOffset = 0x51; // bool m_bGlowing
        
        uintptr_t glowPropertyAddress = pawn + uGlowPropertyOffset;
        
        if (IsBadWritePtr((void*)glowPropertyAddress, sizeof(uintptr_t))) {
            return;
        }
        
        __try {
            // Disable glow by setting m_bGlowing to false
            *(bool*)(glowPropertyAddress + uGlowingOffset) = false;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            // Skip if access violation
        }
    }
    
    // Chams ESP - Simple implementation using render flags
    // CS2 Chams works by modifying render mode or material properties
    // Simplest method: Use render flags to make models visible through walls
    void SetChams(bool enabled, float r, float g, float b, float a) const {
        if (!pawn) return;
        
        // Get scene node
        uintptr_t sceneNode = *(uintptr_t*)(pawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!sceneNode) return;
        
        // Simple Chams: Modify render flags to enable X-Ray/see-through effect
        // In CS2, this is typically done through render mode flags
        // Common approach: Set a flag that makes the model render through walls
        
        // Try to find render mode flag offset
        // Based on common CS2 internal cheat implementations
        // Render mode is often at sceneNode + offset
        
        if (enabled) {
            // Enable chams - try multiple common offsets
            uintptr_t chamsOffsets[] = {
                0x274,  // Common render mode offset
                0x270,  // Alternative
                0x278,  // Alternative
                0x27C,  // Alternative
                0x280,  // Alternative
            };
            
            // Set render mode to enable see-through (value varies, try common ones)
            for (int i = 0; i < sizeof(chamsOffsets) / sizeof(chamsOffsets[0]); i++) {
                uintptr_t renderModePtr = sceneNode + chamsOffsets[i];
                if (renderModePtr) {
                    // Try setting render mode flags
                    // Common values: 1 = normal, 2 = wireframe, 3 = flat, etc.
                    // For chams, we might need a specific value
                    *(int*)(renderModePtr) = 2; // Try wireframe mode
                }
            }
            
            // Also try direct material color modification
            // Material color is often stored at a specific offset
            uintptr_t colorOffsets[] = {
                0x300,  // Common material color offset
                0x304,  // Alternative
                0x308,  // Alternative
            };
            
            for (int i = 0; i < sizeof(colorOffsets) / sizeof(colorOffsets[0]); i++) {
                uintptr_t colorPtr = sceneNode + colorOffsets[i];
                if (colorPtr) {
                    // Set RGBA color
                    *(float*)(colorPtr) = r;
                    *(float*)(colorPtr + 4) = g;
                    *(float*)(colorPtr + 8) = b;
                    *(float*)(colorPtr + 12) = a;
                }
            }
        } else {
            // Disable chams - reset to default
            uintptr_t chamsOffsets[] = {
                0x274, 0x270, 0x278, 0x27C, 0x280,
            };
            
            for (int i = 0; i < sizeof(chamsOffsets) / sizeof(chamsOffsets[0]); i++) {
                uintptr_t renderModePtr = sceneNode + chamsOffsets[i];
                if (renderModePtr) {
                    *(int*)(renderModePtr) = 0; // Reset to default
                }
            }
        }
    }
    
    void EnableChams(float r, float g, float b, float a) const {
        SetChams(true, r, g, b, a);
    }
    
    void DisableChams() const {
        SetChams(false, 0.0f, 0.0f, 0.0f, 0.0f);
    }
};

