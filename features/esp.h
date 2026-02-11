#pragma once
#include "imgui/imgui.h"

namespace ESP {
    // Toggles
    extern bool enabled;
    extern bool boxEnabled;
    extern bool corneredBoxEnabled;
    extern bool boxFillEnabled;
    extern bool glowEnabled;
    extern bool healthBarEnabled;
    extern bool nameEnabled;
    extern bool distanceEnabled;
    extern bool weaponEnabled;
    extern bool headCircleEnabled;
    extern bool linesEnabled;
    extern int linesOrigin; // 0 = top, 1 = bottom, 2 = crosshair
    extern bool viewEspEnabled;
    extern bool teamEspEnabled;
    extern bool chamsEnabled;
    
    // Colors (RGBA)
    extern ImVec4 enemyBoxColor;
    extern ImVec4 teamBoxColor;
    extern ImVec4 enemyGlowColor;
    extern ImVec4 teamGlowColor;
    extern ImVec4 healthBarColor;
    extern ImVec4 nameColor;
    extern ImVec4 linesColor;
    extern ImVec4 enemyChamsColor;
    extern ImVec4 teamChamsColor;

    void Render();
}

