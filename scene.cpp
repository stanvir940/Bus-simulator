#include "scene.h"

#include <GLUT/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

Vec3 crossVec(const Vec3& a, const Vec3& b) {
    Vec3 c = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    return c;
}

float vecLen(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 normalizeVec(const Vec3& v) {
    const float L = vecLen(v);
    if (L < 1e-5f) {
        return {0.0f, 1.0f, 0.0f};
    }
    return {v.x / L, v.y / L, v.z / L};
}

Vec3 triNormalVec(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2) {
    Vec3 u = {x1 - x0, y1 - y0, z1 - z0};
    Vec3 w = {x2 - x0, y2 - y0, z2 - z0};
    return normalizeVec(crossVec(u, w));
}

}  // namespace

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

void Scene::onKeyState(unsigned char key, bool isPressed) {
    if (key == 'w' || key == 'W') m_keyW = isPressed;
    if (key == 's' || key == 'S') m_keyS = isPressed;
    if (key == 'a' || key == 'A') m_keyA = isPressed;
    if (key == 'd' || key == 'D') m_keyD = isPressed;
}

void Scene::onSpecialState(int key, bool isPressed) {
    if (key == GLUT_KEY_LEFT) m_specialLeft = isPressed;
    if (key == GLUT_KEY_RIGHT) m_specialRight = isPressed;
}

const Bus& Scene::getDriverBus() const {
    return m_buses.front();
}

void Scene::drawCuboid(float sx, float sy, float sz) const {
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void Scene::drawCylinder(float radius, float height, int slices) const {
    GLUquadric* quadric = gluNewQuadric();
    glPushMatrix();
    // gluCylinder runs along +Z; rotate so it extends along +Y (up).
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, radius, radius, height, slices, 1);
    glPopMatrix();
    gluDeleteQuadric(quadric);
}

void Scene::drawConeUp(float baseRadius, float height, int slices, int stacks) const {
    glPushMatrix();
    // glutSolidCone grows along +Z with base at z=0; align +Z with world +Y.
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(baseRadius, height, slices, stacks);
    glPopMatrix();
}

float Scene::terrainHeight(float x, float z) const {
    const bool stationZone = (std::fabs(x) < 45.0f && std::fabs(z) < 45.0f);
    if (stationZone) {
        return 0.0f;
    }
    const float base = std::sin(x * 0.055f) * 1.2f + std::cos(z * 0.047f) * 0.9f;
    const float detail = std::sin((x + z) * 0.085f) * 0.55f;
    const float fine =
        std::sin(x * 0.14f) * 0.18f + std::cos(z * 0.11f) * 0.15f + std::sin((x * 0.73f + z * 0.81f) * 0.22f) * 0.11f;
    return base + detail + fine;
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

void Scene::applyMaterial(float r, float g, float b, float shininess, float specular) const {
    glColor3f(r, g, b);
    const GLfloat spec[] = {specular, specular, specular, 1.0f};
    const GLfloat shin[] = {shininess};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shin);
}

void Scene::setupLighting() const {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);

    const GLfloat globalAmbient[] = {0.26f, 0.26f, 0.30f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    const GLfloat ambient[] = {0.22f, 0.22f, 0.25f, 1.0f};
    const GLfloat diffuse[] = {0.95f, 0.93f, 0.85f, 1.0f};
    const GLfloat spec0[] = {0.75f, 0.72f, 0.65f, 1.0f};
    const GLfloat position[] = {40.0f, 55.0f, 10.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    const GLfloat roadAmbient[] = {0.03f, 0.03f, 0.03f, 1.0f};
    const GLfloat roadDiffuse[] = {0.45f, 0.43f, 0.35f, 1.0f};
    const GLfloat roadSpec[] = {0.28f, 0.25f, 0.20f, 1.0f};
    const GLfloat roadPos[] = {0.0f, 12.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, roadAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, roadDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, roadSpec);
    glLightfv(GL_LIGHT1, GL_POSITION, roadPos);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.03f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.004f);
}

void Scene::drawGround() const {
    const float minX = -90.0f;
    const float maxX = 90.0f;
    const float minZ = -90.0f;
    const float maxZ = 90.0f;
    const float step = 2.4f;

    const GLfloat grassSpec[] = {0.06f, 0.09f, 0.05f, 1.0f};
    const GLfloat grassShin[] = {10.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, grassSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, grassShin);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (float x = minX; x < maxX; x += step) {
        glBegin(GL_TRIANGLE_STRIP);
        for (float z = minZ; z <= maxZ; z += step) {
            const float h0 = terrainHeight(x, z);
            const Vec3 n0 = terrainNormal(x, z);
            const float h1 = terrainHeight(x + step, z);
            const Vec3 n1 = terrainNormal(x + step, z);

            float r0 = 0.0f;
            float g0 = 0.0f;
            float b0 = 0.0f;
            float r1 = 0.0f;
            float g1 = 0.0f;
            float b1 = 0.0f;
            terrainGrassColor(x, z, h0, n0, &r0, &g0, &b0);
            terrainGrassColor(x + step, z, h1, n1, &r1, &g1, &b1);
            glColor3f(r0, g0, b0);
            glNormal3f(n0.x, n0.y, n0.z);
            glVertex3f(x, h0, z);

            glColor3f(r1, g1, b1);
            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(x + step, h1, z);
        }
        glEnd();
    }
}

void Scene::roadAsphaltColorAt(float x, float z, float x0, float z0, float x1, float z1) const {
    const float edge = std::min(std::min(x - x0, x1 - x), std::min(z - z0, z1 - z));
    const float shoulder = std::min(edge / 2.6f, 1.0f);
    const float wear = 0.9f + 0.08f * std::sin(x * 1.05f + z * 0.88f) + 0.06f * std::sin(x * 4.4f - z * 3.9f);
    const float tire = 0.86f + 0.14f * std::sin(z * 0.19f + 0.3f);
    const float dim = (0.62f + 0.38f * shoulder) * wear * tire;
    const float r = 0.09f * dim;
    const float g = 0.09f * dim;
    const float b = 0.10f * dim;
    glColor3f(r, g, b);
}

void Scene::drawRoadGridRect(float x0, float x1, float z0, float z1, float y, int splits) const {
    const int n = (splits < 2) ? 2 : splits;
    glNormal3f(0.0f, 1.0f, 0.0f);
    for (int j = 0; j < n; ++j) {
        const float za = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zb = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= n; ++i) {
            const float x = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            roadAsphaltColorAt(x, za, x0, z0, x1, z1);
            glVertex3f(x, y, za);
            roadAsphaltColorAt(x, zb, x0, z0, x1, z1);
            glVertex3f(x, y, zb);
        }
        glEnd();
    }
}

void Scene::drawRoadNetwork() const {
    const float y = 0.038f;

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const GLfloat roadSpec[] = {0.16f, 0.16f, 0.17f, 1.0f};
    const GLfloat roadShin[] = {36.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, roadSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, roadShin);

    drawRoadGridRect(-40.0f, 40.0f, -24.0f, 24.0f, y, 14);
    drawRoadGridRect(-80.0f, -40.0f, -8.0f, 8.0f, y, 10);
    drawRoadGridRect(40.0f, 80.0f, -8.0f, 8.0f, y, 10);

    glDisable(GL_LIGHTING);
    glColor3f(0.96f, 0.86f, 0.22f);
    glBegin(GL_LINES);
    for (float x = -35.0f; x < 35.0f; x += 10.0f) {
        glVertex3f(x, 0.055f, -20.0f);
        glVertex3f(x + 5.0f, 0.055f, -20.0f);
        glVertex3f(x, 0.055f, 20.0f);
        glVertex3f(x + 5.0f, 0.055f, 20.0f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void Scene::drawRoadDetail() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.92f, 0.91f, 0.90f);
    glBegin(GL_LINES);
    for (float z = -78.0f; z < 78.0f; z += 8.0f) {
        glVertex3f(-60.0f, 0.056f, z);
        glVertex3f(-60.0f, 0.056f, z + 4.0f);
        glVertex3f(60.0f, 0.056f, z);
        glVertex3f(60.0f, 0.056f, z + 4.0f);
    }
    glEnd();
    glColor3f(0.98f, 0.82f, 0.18f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-40.0f, 0.056f, -24.0f);
    glVertex3f(40.0f, 0.056f, -24.0f);
    glVertex3f(40.0f, 0.056f, 24.0f);
    glVertex3f(-40.0f, 0.056f, 24.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

void Scene::drawShadedBuildingFace(float r, float g, float b, float nx, float ny, float nz,
    const float* verts12) const {
    const float lx = 0.56f;
    const float ly = 0.78f;
    const float lz = 0.30f;
    float d = nx * lx + ny * ly + nz * lz;
    if (d < 0.0f) {
        d = 0.0f;
    }
    float shade = 0.24f + 0.76f * d;
    if (ny > 0.85f) {
        shade = std::min(1.0f, shade * 1.12f);
    }
    if (ny < -0.85f) {
        shade *= 0.48f;
    }
    glNormal3f(nx, ny, nz);
    glColor3f(r * shade, g * shade, b * shade);
    glBegin(GL_QUADS);
    glVertex3f(verts12[0], verts12[1], verts12[2]);
    glVertex3f(verts12[3], verts12[4], verts12[5]);
    glVertex3f(verts12[6], verts12[7], verts12[8]);
    glVertex3f(verts12[9], verts12[10], verts12[11]);
    glEnd();
}

void Scene::drawShadedBuilding(
    float cx, float baseY, float cz, float w, float h, float d, float br, float bg, float bb) const {
    const float hx = w * 0.5f;
    const float hz = d * 0.5f;
    const float y0 = baseY;
    const float y1 = baseY + h;
    float v[12] = {};

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const GLfloat wallSpec[] = {0.13f, 0.13f, 0.15f, 1.0f};
    const GLfloat wallShin[] = {32.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wallSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, wallShin);

    // +Z
    v[0] = cx - hx;
    v[1] = y0;
    v[2] = cz + hz;
    v[3] = cx + hx;
    v[4] = y0;
    v[5] = cz + hz;
    v[6] = cx + hx;
    v[7] = y1;
    v[8] = cz + hz;
    v[9] = cx - hx;
    v[10] = y1;
    v[11] = cz + hz;
    drawShadedBuildingFace(br, bg, bb, 0.0f, 0.0f, 1.0f, v);

    // -Z
    v[0] = cx + hx;
    v[1] = y0;
    v[2] = cz - hz;
    v[3] = cx - hx;
    v[4] = y0;
    v[5] = cz - hz;
    v[6] = cx - hx;
    v[7] = y1;
    v[8] = cz - hz;
    v[9] = cx + hx;
    v[10] = y1;
    v[11] = cz - hz;
    drawShadedBuildingFace(br, bg, bb, 0.0f, 0.0f, -1.0f, v);

    // +X
    v[0] = cx + hx;
    v[1] = y0;
    v[2] = cz + hz;
    v[3] = cx + hx;
    v[4] = y0;
    v[5] = cz - hz;
    v[6] = cx + hx;
    v[7] = y1;
    v[8] = cz - hz;
    v[9] = cx + hx;
    v[10] = y1;
    v[11] = cz + hz;
    drawShadedBuildingFace(br, bg, bb, 1.0f, 0.0f, 0.0f, v);

    // -X
    v[0] = cx - hx;
    v[1] = y0;
    v[2] = cz - hz;
    v[3] = cx - hx;
    v[4] = y0;
    v[5] = cz + hz;
    v[6] = cx - hx;
    v[7] = y1;
    v[8] = cz + hz;
    v[9] = cx - hx;
    v[10] = y1;
    v[11] = cz - hz;
    drawShadedBuildingFace(br, bg, bb, -1.0f, 0.0f, 0.0f, v);

    // +Y
    v[0] = cx - hx;
    v[1] = y1;
    v[2] = cz - hz;
    v[3] = cx + hx;
    v[4] = y1;
    v[5] = cz - hz;
    v[6] = cx + hx;
    v[7] = y1;
    v[8] = cz + hz;
    v[9] = cx - hx;
    v[10] = y1;
    v[11] = cz + hz;
    drawShadedBuildingFace(br * 1.04f, bg * 1.04f, bb * 1.04f, 0.0f, 1.0f, 0.0f, v);

    // -Y
    v[0] = cx - hx;
    v[1] = y0;
    v[2] = cz + hz;
    v[3] = cx + hx;
    v[4] = y0;
    v[5] = cz + hz;
    v[6] = cx + hx;
    v[7] = y0;
    v[8] = cz - hz;
    v[9] = cx - hx;
    v[10] = y0;
    v[11] = cz - hz;
    drawShadedBuildingFace(br * 0.52f, bg * 0.52f, bb * 0.52f, 0.0f, -1.0f, 0.0f, v);
}

void Scene::drawTerrainRoadPatch(float x0, float x1, float z0, float z1, int splits) const {
    if (x1 <= x0 || z1 <= z0) {
        return;
    }
    const int n = std::max(3, splits);
    const float yOff = 0.055f;

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const GLfloat roadSpec[] = {0.16f, 0.16f, 0.17f, 1.0f};
    const GLfloat roadShin[] = {40.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, roadSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, roadShin);

    for (int j = 0; j < n; ++j) {
        const float zA = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zB = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        for (int i = 0; i < n; ++i) {
            const float xA = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            const float xB = x0 + (x1 - x0) * static_cast<float>(i + 1) / static_cast<float>(n);
            const float h00 = terrainHeight(xA, zA) + yOff;
            const float h10 = terrainHeight(xB, zA) + yOff;
            const float h11 = terrainHeight(xB, zB) + yOff;
            const float h01 = terrainHeight(xA, zB) + yOff;

            Vec3 nA = triNormalVec(xA, h00, zA, xB, h10, zA, xB, h11, zB);
            glBegin(GL_TRIANGLES);
            roadAsphaltColorAt(xA, zA, x0, z0, x1, z1);
            glNormal3f(nA.x, nA.y, nA.z);
            glVertex3f(xA, h00, zA);
            roadAsphaltColorAt(xB, zA, x0, z0, x1, z1);
            glVertex3f(xB, h10, zA);
            roadAsphaltColorAt(xB, zB, x0, z0, x1, z1);
            glVertex3f(xB, h11, zB);

            Vec3 nB = triNormalVec(xA, h00, zA, xB, h11, zB, xA, h01, zB);
            roadAsphaltColorAt(xA, zA, x0, z0, x1, z1);
            glNormal3f(nB.x, nB.y, nB.z);
            glVertex3f(xA, h00, zA);
            roadAsphaltColorAt(xB, zB, x0, z0, x1, z1);
            glVertex3f(xB, h11, zB);
            roadAsphaltColorAt(xA, zB, x0, z0, x1, z1);
            glVertex3f(xA, h01, zB);
            glEnd();
        }
    }
}

void Scene::drawCityRoads() const {
    // Outer boulevards (follow terrain outside the flat station pad).
    drawTerrainRoadPatch(-96.0f, 96.0f, -93.6f, -86.4f, 20);
    drawTerrainRoadPatch(-96.0f, 96.0f, 86.4f, 93.6f, 20);
    drawTerrainRoadPatch(-93.6f, -86.4f, -91.0f, 91.0f, 18);
    drawTerrainRoadPatch(86.4f, 93.6f, -91.0f, 91.0f, 18);
    // Connect station edges toward the outer ring.
    drawTerrainRoadPatch(50.0f, 95.0f, -5.2f, 5.2f, 16);
    drawTerrainRoadPatch(-95.0f, -50.0f, -5.2f, 5.2f, 16);
    drawTerrainRoadPatch(-5.2f, 5.2f, 50.0f, 92.0f, 16);
    drawTerrainRoadPatch(-5.2f, 5.2f, -92.0f, -50.0f, 16);
    // Narrow strips through dense building rows (city grid).
    drawTerrainRoadPatch(-90.0f, 90.0f, -99.2f, -96.2f, 12);
    drawTerrainRoadPatch(-90.0f, 90.0f, 96.2f, 99.2f, 12);
}

void Scene::drawPlatforms() const {
    applyMaterial(0.72f, 0.72f, 0.72f, 24.0f, 0.20f);
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(-22.0f + i * 14.0f, 0.35f, 30.0f);
        drawCuboid(10.0f, 0.7f, 8.0f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-22.0f + i * 14.0f, 3.3f, 30.0f);
        applyMaterial(0.30f, 0.32f, 0.35f, 36.0f, 0.26f);
        drawCuboid(11.0f, 0.35f, 8.6f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-26.8f + i * 14.0f, 1.9f, 30.0f);
        applyMaterial(0.15f, 0.15f, 0.18f, 32.0f, 0.22f);
        drawCuboid(0.3f, 3.2f, 0.3f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-17.2f + i * 14.0f, 1.9f, 30.0f);
        applyMaterial(0.15f, 0.15f, 0.18f, 32.0f, 0.22f);
        drawCuboid(0.3f, 3.2f, 0.3f);
        glPopMatrix();
    }
}

void Scene::drawStationBuilding() const {
    glColor3f(0.65f, 0.70f, 0.78f);
    glPushMatrix();
    glTranslatef(0.0f, 4.0f, -40.0f);
    drawCuboid(36.0f, 8.0f, 16.0f);
    glPopMatrix();

    glColor3f(0.42f, 0.48f, 0.60f);
    glPushMatrix();
    glTranslatef(0.0f, 8.7f, -40.0f);
    drawCuboid(38.0f, 1.0f, 17.0f);
    glPopMatrix();

    glColor3f(0.80f, 0.85f, 0.90f);
    for (int i = 0; i < 5; ++i) {
        glPushMatrix();
        glTranslatef(-13.0f + i * 6.5f, 4.0f, -31.8f);
        drawCuboid(3.0f, 4.0f, 0.2f);
        glPopMatrix();
    }
}

void Scene::drawTicketCounterHouse() const {
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -10.0f);

    glColor3f(0.76f, 0.68f, 0.58f);
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    drawCuboid(26.0f, 5.0f, 12.0f);
    glPopMatrix();

    glColor3f(0.28f, 0.22f, 0.18f);
    glPushMatrix();
    glTranslatef(0.0f, 5.3f, 0.0f);
    drawCuboid(27.0f, 0.8f, 13.0f);
    glPopMatrix();

    // Counter sections inside the ticket house.
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glTranslatef(-7.5f + i * 7.5f, 1.2f, 3.6f);
        glColor3f(0.35f, 0.20f, 0.12f);
        drawCuboid(5.5f, 2.4f, 0.5f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-7.5f + i * 7.5f, 2.5f, 4.2f);
        glColor3f(0.83f, 0.88f, 0.95f);
        drawCuboid(2.4f, 1.2f, 0.2f);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(-8.8f, 1.6f, -3.2f);
    glColor3f(0.60f, 0.58f, 0.56f);
    drawCuboid(0.35f, 3.2f, 7.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.6f, -3.2f);
    glColor3f(0.60f, 0.58f, 0.56f);
    drawCuboid(0.35f, 3.2f, 7.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(8.8f, 1.6f, -3.2f);
    glColor3f(0.60f, 0.58f, 0.56f);
    drawCuboid(0.35f, 3.2f, 7.0f);
    glPopMatrix();

    glPopMatrix();
}

void Scene::drawCitySkyline() const {
    auto buildingColor = [](int seed, float& r, float& g, float& b) {
        const int variant = seed % 5;
        if (variant == 0) {
            r = 0.42f;
            g = 0.45f;
            b = 0.52f;
        } else if (variant == 1) {
            r = 0.52f;
            g = 0.40f;
            b = 0.38f;
        } else if (variant == 2) {
            r = 0.36f;
            g = 0.42f;
            b = 0.50f;
        } else if (variant == 3) {
            r = 0.48f;
            g = 0.50f;
            b = 0.46f;
        } else {
            r = 0.40f;
            g = 0.38f;
            b = 0.47f;
        }
    };

    auto hash = [](int a, int b) { return (a * 73856093) ^ (b * 19349663); };

    auto placeBuilding = [&](float cx, float cz, int id) {
        if (std::fabs(cx) < 46.0f && std::fabs(cz) < 46.0f) {
            return;
        }
        const int hKey = hash(static_cast<int>(cx * 2.0f), static_cast<int>(cz * 2.0f)) ^ id;
        const float h = 10.0f + static_cast<float>((std::abs(hKey) % 38));
        const float w = 4.2f + static_cast<float>((std::abs(hKey / 7) % 5));
        const float d = 4.2f + static_cast<float>((std::abs(hKey / 13) % 5));
        const float y0 = terrainHeight(cx, cz);
        float r = 0.4f;
        float g = 0.42f;
        float b = 0.48f;
        buildingColor(std::abs(hKey), r, g, b);

        drawShadedBuilding(cx, y0, cz, w, h, d, r, g, b);

        glDisable(GL_LIGHTING);
        glColor3f(0.68f, 0.78f, 0.92f);
        const float wy = y0 + h * 0.35f;
        const float wh = h * 0.45f;
        glBegin(GL_QUADS);
        glVertex3f(cx - w * 0.35f, wy, cz + d * 0.51f);
        glVertex3f(cx + w * 0.35f, wy, cz + d * 0.51f);
        glVertex3f(cx + w * 0.35f, wy + wh, cz + d * 0.51f);
        glVertex3f(cx - w * 0.35f, wy + wh, cz + d * 0.51f);
        glEnd();
        glEnable(GL_LIGHTING);
    };

    for (int i = -12; i <= 12; ++i) {
        const float x = static_cast<float>(i) * 9.0f;
        placeBuilding(x, -98.0f, i);
        placeBuilding(x, 98.0f, i + 1000);
    }

    for (int j = -10; j <= 10; ++j) {
        const float z = static_cast<float>(j) * 9.0f;
        placeBuilding(-98.0f, z, j + 2000);
        placeBuilding(98.0f, z, j + 3000);
    }

    for (int t = 0; t < 22; ++t) {
        const int hk = hash(t, 404);
        const float cx = (std::abs(hk % 180)) - 90.0f;
        const float cz = (std::abs((hk / 180) % 180)) - 90.0f;
        if (std::fabs(cx) < 50.0f && std::fabs(cz) < 50.0f) {
            continue;
        }
        const float y0 = terrainHeight(cx, cz);
        const float h = 22.0f + static_cast<float>((std::abs(hk) % 55));
        const float w = 6.0f;
        const float d = 6.0f;
        drawShadedBuilding(cx, y0, cz, w, h, d, 0.32f, 0.34f, 0.40f);
    }
}

void Scene::drawBoundaryWalls() const {
    glColor3f(0.55f, 0.50f, 0.45f);

    glPushMatrix();
    glTranslatef(0.0f, 1.8f, -84.0f);
    drawCuboid(168.0f, 3.6f, 1.2f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.8f, 84.0f);
    drawCuboid(168.0f, 3.6f, 1.2f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-84.0f, 1.8f, 0.0f);
    drawCuboid(1.2f, 3.6f, 168.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(84.0f, 1.8f, 0.0f);
    drawCuboid(1.2f, 3.6f, 168.0f);
    glPopMatrix();
}

void Scene::drawLightPoles() const {
    for (int i = -4; i <= 4; ++i) {
        const float z = static_cast<float>(i) * 18.0f;
        const float sideX[2] = {-64.0f, 64.0f};
        for (float x : sideX) {
            glPushMatrix();
            glTranslatef(x, terrainHeight(x, z), z);
            applyMaterial(0.30f, 0.30f, 0.34f, 40.0f, 0.30f);
            drawCylinder(0.22f, 8.0f, 10);
            glTranslatef(0.0f, 8.0f, 0.0f);
            applyMaterial(1.0f, 0.93f, 0.62f, 70.0f, 0.55f);
            glutSolidSphere(0.42f, 12, 12);
            glPopMatrix();
        }
    }

    const float platformPoles[] = {-34.0f, -12.0f, 12.0f, 34.0f};
    for (float x : platformPoles) {
        glPushMatrix();
        glTranslatef(x, 0.0f, 24.5f);
        applyMaterial(0.35f, 0.35f, 0.38f, 40.0f, 0.30f);
        drawCylinder(0.25f, 7.0f, 10);
        glTranslatef(0.0f, 7.0f, 0.0f);
        applyMaterial(0.97f, 0.92f, 0.55f, 70.0f, 0.45f);
        glutSolidSphere(0.45f, 10, 10);
        glPopMatrix();
    }
}

void Scene::drawTreesAndJungle() const {
    for (int i = 0; i < 20; ++i) {
        const float x = -72.0f + static_cast<float>(i % 5) * 9.0f;
        const float z = -68.0f + static_cast<float>(i / 5) * 10.0f;
        const float y = terrainHeight(x, z);
        const float trunkH = 3.8f;
        const float foliageR = 1.9f;

        glPushMatrix();
        glTranslatef(x, y, z);
        applyMaterial(0.45f, 0.26f, 0.12f, 10.0f, 0.06f);
        drawCylinder(0.42f, trunkH, 10);
        glTranslatef(0.0f, trunkH + foliageR * 0.85f, 0.0f);
        applyMaterial(0.11f, 0.46f, 0.16f, 14.0f, 0.08f);
        glutSolidSphere(foliageR, 12, 12);
        glPopMatrix();
    }

    for (int i = 0; i < 10; ++i) {
        const float x = 58.0f + static_cast<float>(i % 3) * 7.0f;
        const float z = -65.0f + static_cast<float>(i / 3) * 9.0f;
        const float y = terrainHeight(x, z);
        const float trunkH = 3.2f;
        const float coneH = 3.2f;

        glPushMatrix();
        glTranslatef(x, y, z);
        applyMaterial(0.42f, 0.24f, 0.12f, 10.0f, 0.06f);
        drawCylinder(0.38f, trunkH, 10);
        glTranslatef(0.0f, trunkH, 0.0f);
        applyMaterial(0.06f, 0.38f, 0.12f, 14.0f, 0.08f);
        drawConeUp(1.8f, coneH, 10, 4);
        glPopMatrix();
    }

    // Roadside tree rows.
    for (int i = -8; i <= 8; ++i) {
        const float z = i * 10.0f;
        const float xLeft = -74.0f;
        const float xRight = 74.0f;
        const float yLeft = terrainHeight(xLeft, z);
        const float yRight = terrainHeight(xRight, z);
        const float trunkH = 3.0f;

        glPushMatrix();
        glTranslatef(xLeft, yLeft, z);
        applyMaterial(0.42f, 0.22f, 0.10f, 10.0f, 0.05f);
        drawCylinder(0.32f, trunkH, 10);
        glTranslatef(0.0f, trunkH, 0.0f);
        applyMaterial(0.08f, 0.42f, 0.14f, 12.0f, 0.06f);
        drawConeUp(1.7f, 2.8f, 10, 4);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(xRight, yRight, z);
        applyMaterial(0.42f, 0.22f, 0.10f, 10.0f, 0.05f);
        drawCylinder(0.32f, trunkH, 10);
        glTranslatef(0.0f, trunkH, 0.0f);
        applyMaterial(0.08f, 0.42f, 0.14f, 12.0f, 0.06f);
        drawConeUp(1.7f, 2.8f, 10, 4);
        glPopMatrix();
    }
}

void Scene::drawLake() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.18f, 0.46f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(46.0f, 0.03f, 46.0f);
    glVertex3f(80.0f, 0.03f, 46.0f);
    glVertex3f(80.0f, 0.03f, 78.0f);
    glVertex3f(46.0f, 0.03f, 78.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

void Scene::drawHuman(float animPhase) const {
    const float swing = std::sin(animPhase) * 20.0f;
    glPushMatrix();
    glColor3f(0.93f, 0.78f, 0.62f);
    glTranslatef(0.0f, 1.7f, 0.0f);
    glutSolidSphere(0.22f, 10, 10);

    glColor3f(0.10f, 0.30f, 0.65f);
    glTranslatef(0.0f, -0.55f, 0.0f);
    glPushMatrix();
    drawCuboid(0.45f, 0.85f, 0.25f);
    glPopMatrix();

    glColor3f(0.08f, 0.08f, 0.08f);
    glPushMatrix();
    glTranslatef(-0.12f, -0.55f, 0.0f);
    glRotatef(swing, 1.0f, 0.0f, 0.0f);
    drawCuboid(0.12f, 0.70f, 0.12f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.12f, -0.55f, 0.0f);
    glRotatef(-swing, 1.0f, 0.0f, 0.0f);
    drawCuboid(0.12f, 0.70f, 0.12f);
    glPopMatrix();
    glPopMatrix();
}

void Scene::drawNPCs() const {
    for (int i = 0; i < 8; ++i) {
        glPushMatrix();
        glTranslatef(-14.0f + i * 3.8f, 0.0f, 9.0f + ((i % 2) ? 1.0f : -1.0f));
        drawHuman(m_timeSec * 3.0f + static_cast<float>(i));
        glPopMatrix();
    }
    for (int i = 0; i < 6; ++i) {
        glPushMatrix();
        glTranslatef(-24.0f + i * 10.0f, 0.0f, 29.0f);
        drawHuman(m_timeSec * 2.5f + static_cast<float>(i));
        glPopMatrix();
    }
}

void Scene::drawSkyBackdrop() const {
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glColor3f(0.60f, 0.78f, 0.95f);
    glBegin(GL_QUADS);
    glVertex3f(-95.0f, 0.0f, -95.0f);
    glVertex3f(95.0f, 0.0f, -95.0f);
    glVertex3f(95.0f, 70.0f, -95.0f);
    glVertex3f(-95.0f, 70.0f, -95.0f);

    glVertex3f(-95.0f, 0.0f, 95.0f);
    glVertex3f(95.0f, 0.0f, 95.0f);
    glVertex3f(95.0f, 70.0f, 95.0f);
    glVertex3f(-95.0f, 70.0f, 95.0f);

    glVertex3f(-95.0f, 0.0f, -95.0f);
    glVertex3f(-95.0f, 0.0f, 95.0f);
    glVertex3f(-95.0f, 70.0f, 95.0f);
    glVertex3f(-95.0f, 70.0f, -95.0f);

    glVertex3f(95.0f, 0.0f, -95.0f);
    glVertex3f(95.0f, 0.0f, 95.0f);
    glVertex3f(95.0f, 70.0f, 95.0f);
    glVertex3f(95.0f, 70.0f, -95.0f);
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

void Scene::drawBuses() const {
    for (const Bus& bus : m_buses) {
        bus.render();
    }
}

void Scene::render() const {
    drawSkyBackdrop();
    setupLighting();
    drawGround();
    drawCityRoads();
    drawCitySkyline();
    drawRoadNetwork();
    drawRoadDetail();
    drawPlatforms();
    drawStationBuilding();
    drawTicketCounterHouse();
    drawBoundaryWalls();
    drawLightPoles();
    drawTreesAndJungle();
    drawLake();
    drawNPCs();
    drawBuses();
}
