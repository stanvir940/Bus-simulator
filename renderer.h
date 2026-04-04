#pragma once

#include "lighting.h"
#include "render_settings.h"
#include "shader_program.h"

#include <GL/glew.h>
#include <string>
#include <vector>

class Scene;
class CameraController;
class Bus;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(const std::string& shaderRoot, const Scene& scene);
    void draw(const Scene& scene, const CameraController& camera, const Bus& driverBus, int fbWidth, int fbHeight,
        float timeSec, WorldMode worldMode, const AppRenderSettings& settings);

private:
    void destroyGpu();
    void loadTextures();
    void buildTerrain(const Scene& scene);
    void buildRoadMesh(const Scene& scene);
    void buildTriplanarMesh(const Scene& scene);
    void buildColoredStatic(const Scene& scene);
    void buildWater(const Scene& scene);
    void buildInterior();

    void drawOutdoor(const Scene& scene, const CameraController& camera, const Bus& driverBus, int fbWidth,
        int fbHeight, float timeSec, const AppRenderSettings& settings);
    void drawInteriorScene(float timeSec, const float* view, const float* proj, const float* camPos,
        const AppRenderSettings& settings);

    void drawBuses(const Scene& scene, const float* view, const float* proj, const float* cameraPos,
        const AppRenderSettings& settings);

    void drawTexturedQuad(GLuint program, const float* view, const float* proj, const float* model, float nx, float ny,
        float nz, const float* verts12, float u0, float v0, float u1, float v1, GLuint tex, float fallbackR,
        float fallbackG, float fallbackB, std::vector<float>& scratch);

    int fillSpotlights(const Scene& scene, Spotlight* out);

    ShaderProgram m_standard;
    ShaderProgram m_terrain;
    ShaderProgram m_water;
    ShaderProgram m_triplanar;
    ShaderProgram m_stall;

    GLuint m_terrainVao = 0;
    GLuint m_terrainVbo = 0;
    GLsizei m_terrainVertexCount = 0;

    GLuint m_staticVao = 0;
    GLuint m_staticVbo = 0;
    GLsizei m_staticVertexCount = 0;

    GLuint m_roadVao = 0;
    GLuint m_roadVbo = 0;
    GLsizei m_roadVertexCount = 0;

    GLuint m_triplanarVao = 0;
    GLuint m_triplanarVbo = 0;
    GLsizei m_triplanarVertexCount = 0;

    GLuint m_waterVao = 0;
    GLuint m_waterVbo = 0;
    GLsizei m_waterVertexCount = 0;

    GLuint m_interiorVao = 0;
    GLuint m_interiorVbo = 0;
    GLsizei m_interiorVertexCount = 0;

    GLuint m_grassTex = 0;
    GLuint m_roadTex = 0;
    GLuint m_buildingTex = 0;
    GLuint m_lampTex = 0;
    GLuint m_chairTex = 0;
    GLuint m_npcTex = 0;
    GLuint m_waterTex = 0;
    GLuint m_whiteTex = 0;

    bool m_hasGrassTex = false;
    bool m_hasRoadTex = false;
    bool m_hasBuildingTex = false;
    bool m_hasLampTex = false;
    bool m_hasChairTex = false;
    bool m_hasNpcTex = false;
    bool m_hasWaterTex = false;

    std::string m_shaderRoot;
};
