# Implemented Offsets

This document lists all the offsets that have been implemented from the dump files located at `C:\Users\mas\Downloads\output\`.

## Base Offsets (offsets.hpp)

- `dwEntityList` = 0x24AA0D8
- `dwLocalPlayerPawn` = 0x2064AE0
- `dwViewMatrix` = 0x230ADE0
- `dwViewAngles` = 0x2314F98

## Schema Offsets (client_dll.hpp)

### C_BaseEntity
- `m_iHealth` = 0x354
- `m_iTeamNum` = 0x3F3
- `m_pGameSceneNode` = 0x338

### C_CSPlayerPawn
- `m_angEyeAngles` = 0x3DD0 (used for view angles)

### CCSPlayerController
- `m_hPlayerPawn` = 0x90C

### CGameSceneNode
- `m_vecAbsOrigin` = 0xD0
- `m_bDormant` = 0x10B

### CSkeletonInstance
- `m_modelState` = 0x160 (used for bone access)

## Button Offsets (buttons.hpp)

- `attack` = 0x205D860 (used for triggerbot)

## Notes

1. **View Angles**: Accessed through `C_CSPlayerPawn::m_angEyeAngles` at offset 0x3DD0
2. **Bone Access**: Bones are accessed through `CSkeletonInstance::m_modelState` + bone array offset (currently 0x80, may need adjustment)
3. **Entity Position**: Accessed through `CGameSceneNode::m_vecAbsOrigin` at offset 0xD0
4. **Dormant Check**: Accessed through `CGameSceneNode::m_bDormant` at offset 0x10B
5. **Entity List Stride**: 0x70 (NOT 0x78)
6. **Entity Handle Mask**: 0x7FFF

## Usage

All offsets are accessed through the namespace structure:
- `offsets::client_dll::dwEntityList`
- `schemas::client_dll::C_BaseEntity::m_iHealth`
- `schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles`
- `offsets::buttons::attack`

