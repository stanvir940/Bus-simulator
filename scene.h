#pragma once

#include "bus.h"

#include <vector>

class Scene {
public:
    Scene();

    // Call once after OpenGL context exists (loads bus texture, etc.).
    void initGlResources();

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
    void drawCitySkyline() const;
    void drawCityRoads() const;
    void drawTerrainRoadPatch(float x0, float x1, float z0, float z1, int splits) const;
    void drawShadedBuilding(
        float cx, float baseY, float cz, float w, float h, float d, float r, float g, float b) const;
    void drawShadedBuildingFace(
        float r, float g, float b, float nx, float ny, float nz, const float* verts12) const;
    void drawHuman(float animPhase) const;
    float terrainHeight(float x, float z) const;
    Vec3 terrainNormal(float x, float z) const;
    void terrainGrassColor(float x, float z, float height, const Vec3& normal, float* outR, float* outG,
        float* outB) const;
    void applyMaterial(float r, float g, float b, float shininess, float specular) const;
    void drawRoadGridRect(float x0, float x1, float z0, float z1, float y, int splits) const;
    void roadAsphaltColorAt(float x, float z, float x0, float z0, float x1, float z1) const;

    void drawCuboid(float sx, float sy, float sz) const;
    void drawCylinder(float radius, float height, int slices) const;
    void drawConeUp(float baseRadius, float height, int slices, int stacks) const;

    std::vector<Bus> m_buses;
    float m_timeSec;
    bool m_keyW;
    bool m_keyS;
    bool m_keyA;
    bool m_keyD;
    bool m_specialLeft;
    bool m_specialRight;
};
