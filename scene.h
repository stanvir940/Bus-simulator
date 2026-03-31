#pragma once

#include "bus.h"

#include <vector>

class Scene {
public:
    Scene();

    void update(float dt);
    void render() const;
    void onKeyState(unsigned char key, bool isPressed);
    void onSpecialState(int key, bool isPressed);

    const Bus& getDriverBus() const;

private:
    void setupLighting() const;
    void drawGround() const;
    void drawRoadNetwork() const;
    void drawPlatforms() const;
    void drawStationBuilding() const;
    void drawBoundaryWalls() const;
    void drawLightPoles() const;
    void drawTreesAndJungle() const;
    void drawLake() const;
    void drawTicketCounterHouse() const;
    void drawNPCs() const;
    void drawBuses() const;
    void drawRoadDetail() const;
    void drawSkyBackdrop() const;
    void drawHuman(float animPhase) const;
    float terrainHeight(float x, float z) const;
    Vec3 terrainNormal(float x, float z) const;
    void applyMaterial(float r, float g, float b, float shininess, float specular) const;

    void drawCuboid(float sx, float sy, float sz) const;
    void drawCylinder(float radius, float height, int slices) const;

    std::vector<Bus> m_buses;
    float m_timeSec;
    bool m_keyW;
    bool m_keyS;
    bool m_keyA;
    bool m_keyD;
    bool m_specialLeft;
    bool m_specialRight;
};
