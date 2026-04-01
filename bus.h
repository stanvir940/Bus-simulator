#pragma once

#include <vector>

struct Vec3 {
    float x;
    float y;
    float z;
};

class Bus {
public:
    Bus(float speedUnitsPerSec, float initialDistance, float busLength, float busWidth, float busHeight);

    // Call once after an OpenGL context exists (e.g. after glutCreateWindow).
    static void initGraphics();

    void update(float dt);
    void render() const;
    void setAutopilot(bool enabled);
    bool isAutopilot() const;
    void control(bool accelerate, bool brake, bool turnLeft, bool turnRight, float dt);

    Vec3 getPosition() const;
    float getHeadingDegrees() const;

private:
    Vec3 evaluatePath(float distance, Vec3* tangent = nullptr) const;
    float wrapDistance(float value) const;
    void drawUnitCylinder(float radius, float length, int slices) const;
    void drawTexturedBodyBox(float sx, float sy, float sz) const;
    void drawFallbackBodyBox(float sx, float sy, float sz) const;

    float m_speed;
    float m_distance;
    float m_length;
    float m_width;
    float m_height;
    bool m_autopilot;
    Vec3 m_manualPos;
    float m_manualHeadingDeg;
    float m_manualSpeed;
    float m_steerAngleDeg;
    float m_wheelBase;
    float m_maxSteerDeg;

    std::vector<Vec3> m_pathPoints;
    std::vector<float> m_segmentLengths;
    float m_totalPathLength;

    static unsigned int s_bodyTexture;
    static unsigned int s_frontTexture;
    static unsigned int s_backTexture;
    static unsigned int s_windowTexture;
    static bool s_graphicsInitialized;
    static bool s_bodyTextureOk;
    static bool s_frontTextureOk;
    static bool s_backTextureOk;
    static bool s_windowTextureOk;

    static bool hasAnyBusTexture();
    void drawTexturedQuadFace(unsigned int tex, float r, float g, float b, float nx, float ny, float nz,
        float u0, float v0, float u1, float v1, const float* verts12) const;
    void drawWindowBandTextured(float sx, float sy, float sz) const;
};
