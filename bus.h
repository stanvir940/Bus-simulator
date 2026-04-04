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

    // Call once after an OpenGL context exists.
    static void initGraphics();

    void update(float dt);
    void setAutopilot(bool enabled);
    bool isAutopilot() const;
    void control(bool accelerate, bool brake, bool turnLeft, bool turnRight, float dt);

    Vec3 getPosition() const;
    float getHeadingDegrees() const;
    float getLength() const { return m_length; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }

    static unsigned int bodyTextureId();
    static unsigned int frontTextureId();
    static unsigned int backTextureId();
    static unsigned int windowTextureId();
    static bool hasAnyBusTexture();

private:
    Vec3 evaluatePath(float distance, Vec3* tangent = nullptr) const;
    float wrapDistance(float value) const;

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
};
