#include "scene.h"
#include "scene_constants.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

void Scene::initGlResources() {
    Bus::initGraphics();
}

Scene::Scene()
    : m_timeSec(0.0f),
      m_keyW(false),
      m_keyS(false),
      m_keyA(false),
      m_keyD(false),
      m_specialLeft(false),
      m_specialRight(false) {
    const float loopLength = 216.0f;
    m_buses.emplace_back(7.0f, 0.0f, 5.2f, 2.3f, 2.0f);
    m_buses.emplace_back(7.0f, loopLength * 0.33f, 5.2f, 2.3f, 2.0f);
    m_buses.emplace_back(7.0f, loopLength * 0.66f, 5.2f, 2.3f, 2.0f);
    m_buses[0].setAutopilot(false);
}

void Scene::update(float dt) {
    m_timeSec += dt;
    if (!m_buses.empty()) {
        m_buses[0].control(m_keyW, m_keyS, (m_keyA || m_specialLeft), (m_keyD || m_specialRight), dt);
    }
    for (Bus& bus : m_buses) {
        bus.update(dt);
    }
}

void Scene::onKeyState(int key, bool isPressed) {
    if (key == GLFW_KEY_W) m_keyW = isPressed;
    if (key == GLFW_KEY_S) m_keyS = isPressed;
    if (key == GLFW_KEY_A) m_keyA = isPressed;
    if (key == GLFW_KEY_D) m_keyD = isPressed;
}

void Scene::onSpecialState(int key, bool isPressed) {
    if (key == GLFW_KEY_LEFT) m_specialLeft = isPressed;
    if (key == GLFW_KEY_RIGHT) m_specialRight = isPressed;
}

const Bus& Scene::getDriverBus() const {
    return m_buses.front();
}

float Scene::terrainHeight(float x, float z) const {
    const bool stationZone = (std::fabs(x) < kStationFlatRadius && std::fabs(z) < kStationFlatRadius);
    if (stationZone) {
        return 0.0f;
    }
    // Lake basin: bowl so water sits below rim (not “floating in the sky”).
    const float lakeCx = 63.0f;
    const float lakeCz = 62.0f;
    const float ldx = (x - lakeCx) / 34.0f;
    const float ldz = (z - lakeCz) / 29.0f;
    const float lakeT = ldx * ldx + ldz * ldz;
    float lakeDepress = 0.0f;
    if (lakeT < 1.0f) {
        const float rim = 1.0f - lakeT;
        lakeDepress = -1.85f * rim * rim;
    }

    const float dist = std::sqrt(x * x + z * z);
    const float edgeFade = std::min(1.0f, (kTerrainHalfExtent - dist) / 90.0f);
    const float base = (std::sin(x * 0.038f) * 1.1f + std::cos(z * 0.033f) * 0.85f) * (0.55f + 0.45f * edgeFade);
    const float detail = std::sin((x + z) * 0.065f) * 0.45f * edgeFade;
    const float fine =
        (std::sin(x * 0.11f) * 0.14f + std::cos(z * 0.09f) * 0.12f + std::sin((x * 0.53f + z * 0.61f) * 0.18f) * 0.09f) *
        edgeFade;
    return base + detail + fine + lakeDepress;
}

void Scene::terrainGrassColor(float x, float z, float height, const Vec3& normal, float* outR, float* outG,
    float* outB) const {
    const float flat = std::max(normal.y, 0.0f);
    const float slope = std::min(1.0f, (1.0f - flat) * 2.2f);
    const float high = (height + 2.0f) * 0.07f;

    float r = 0.11f + high * 0.035f + flat * 0.06f + slope * 0.08f;
    float g = 0.26f + high * 0.14f + flat * 0.22f - slope * 0.08f;
    float b = 0.08f + high * 0.028f + flat * 0.07f - slope * 0.02f;

    const float patch = 0.5f + 0.5f * std::sin(x * 0.081f + 1.7f) * std::cos(z * 0.074f + 0.4f);
    g += patch * 0.045f * flat;
    r += patch * 0.012f * flat;

    const float damp = 0.88f + 0.12f * std::sin((x - z) * 0.031f);
    r *= damp;
    g *= damp;
    b *= damp;

    if (r > 0.95f) r = 0.95f;
    if (g > 0.95f) g = 0.95f;
    if (b > 0.85f) b = 0.85f;
    *outR = r;
    *outG = g;
    *outB = b;
}

Vec3 Scene::terrainNormal(float x, float z) const {
    const float eps = 0.7f;
    const float hL = terrainHeight(x - eps, z);
    const float hR = terrainHeight(x + eps, z);
    const float hD = terrainHeight(x, z - eps);
    const float hU = terrainHeight(x, z + eps);

    Vec3 n = {hL - hR, 2.0f * eps, hD - hU};
    const float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0.0001f) {
        n.x /= len;
        n.y /= len;
        n.z /= len;
    } else {
        n = {0.0f, 1.0f, 0.0f};
    }
    return n;
}
