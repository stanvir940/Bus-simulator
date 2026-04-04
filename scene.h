#pragma once

#include "bus.h"

#include <vector>

class Scene {
public:
    Scene();

    void initGlResources();
    void update(float dt);
    void onKeyState(int key, bool isPressed);
    void onSpecialState(int key, bool isPressed);

    const Bus& getDriverBus() const;
    const std::vector<Bus>& getBuses() const { return m_buses; }

    float terrainHeight(float x, float z) const;
    Vec3 terrainNormal(float x, float z) const;
    void terrainGrassColor(float x, float z, float height, const Vec3& normal, float* outR, float* outG,
        float* outB) const;

    float timeSec() const { return m_timeSec; }

private:
    std::vector<Bus> m_buses;
    float m_timeSec;
    bool m_keyW;
    bool m_keyS;
    bool m_keyA;
    bool m_keyD;
    bool m_specialLeft;
    bool m_specialRight;
};
