#pragma once
#include <cmath>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    float Distance(const Vector3& other) const {
        return (*this - other).Length();
    }

    Vector3 Normalize() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector3(x / len, y / len, z / len);
        }
        return Vector3(0, 0, 0);
    }
};

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

// ViewMatrix in CS2: Can be stored as flat array or 2D array
// Common formats: row-major flat[16] or column-major
struct view_matrix_t {
    union {
        float matrix[4][4];
        float m[16];  // Flat array access
    };

    bool WorldToScreen(const Vector3& world, Vector2& screen, int width, int height) const {
        // CS2 view matrix: Try multiple formats
        // Format 1: Column-major (most common for DirectX/CS2)
        // Matrix stored as: m[col * 4 + row] when accessed as flat array
        // But C arrays are row-major, so matrix[4][4] means m[row * 4 + col]
        
        // Try standard DirectX column-major format
        // If matrix is column-major in memory: m[col * 4 + row]
        // Column 0: m[0], m[1], m[2], m[3]
        // Column 1: m[4], m[5], m[6], m[7]
        // Column 2: m[8], m[9], m[10], m[11]
        // Column 3: m[12], m[13], m[14], m[15]
        
        // W component (row 3 of matrix)
        float w = m[12] * world.x + m[13] * world.y + m[14] * world.z + m[15];
        
        if (w < 0.001f) {
            return false;
        }
        
        // X component (row 0)
        float x = m[0] * world.x + m[1] * world.y + m[2] * world.z + m[3];
        // Y component (row 1)  
        float y = m[4] * world.x + m[5] * world.y + m[6] * world.z + m[7];
        
        // Convert to NDC (Normalized Device Coordinates) [-1, 1]
        float ndcX = x / w;
        float ndcY = y / w;
        
        // Convert NDC to screen coordinates
        // Screen origin is top-left, Y increases downward
        screen.x = (width / 2.0f) * (1.0f + ndcX);
        screen.y = (height / 2.0f) * (1.0f - ndcY);
        
        return true;
    }
};

struct QAngle {
    float pitch, yaw, roll;

    QAngle() : pitch(0), yaw(0), roll(0) {}
    QAngle(float pitch, float yaw, float roll) : pitch(pitch), yaw(yaw), roll(roll) {}

    void Normalize() {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        while (yaw > 180.0f) yaw -= 360.0f;
        while (yaw < -180.0f) yaw += 360.0f;
        roll = 0.0f;
    }
};

