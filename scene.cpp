#include "scene.h"

#include <GLUT/glut.h>
#include <cmath>

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
    gluCylinder(quadric, radius, radius, height, slices, 1);
    gluDeleteQuadric(quadric);
}

float Scene::terrainHeight(float x, float z) const {
    const bool stationZone = (std::fabs(x) < 45.0f && std::fabs(z) < 45.0f);
    if (stationZone) {
        return 0.0f;
    }
    const float base = std::sin(x * 0.055f) * 1.2f + std::cos(z * 0.047f) * 0.9f;
    const float detail = std::sin((x + z) * 0.085f) * 0.55f;
    return base + detail;
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
    const float step = 2.8f;

    for (float x = minX; x < maxX; x += step) {
        glBegin(GL_TRIANGLE_STRIP);
        for (float z = minZ; z <= maxZ; z += step) {
            const float h0 = terrainHeight(x, z);
            const Vec3 n0 = terrainNormal(x, z);
            const float h1 = terrainHeight(x + step, z);
            const Vec3 n1 = terrainNormal(x + step, z);

            const float green0 = 0.30f + (h0 + 2.0f) * 0.035f;
            const float green1 = 0.30f + (h1 + 2.0f) * 0.035f;
            applyMaterial(0.14f, green0, 0.13f, 8.0f, 0.08f);
            glNormal3f(n0.x, n0.y, n0.z);
            glVertex3f(x, h0, z);

            applyMaterial(0.14f, green1, 0.13f, 8.0f, 0.08f);
            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(x + step, h1, z);
        }
        glEnd();
    }
}

void Scene::drawRoadNetwork() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.12f, 0.12f, 0.13f);

    glBegin(GL_QUADS);
    glVertex3f(-40.0f, 0.02f, -24.0f);
    glVertex3f(40.0f, 0.02f, -24.0f);
    glVertex3f(40.0f, 0.02f, 24.0f);
    glVertex3f(-40.0f, 0.02f, 24.0f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(-80.0f, 0.02f, -8.0f);
    glVertex3f(-40.0f, 0.02f, -8.0f);
    glVertex3f(-40.0f, 0.02f, 8.0f);
    glVertex3f(-80.0f, 0.02f, 8.0f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(40.0f, 0.02f, -8.0f);
    glVertex3f(80.0f, 0.02f, -8.0f);
    glVertex3f(80.0f, 0.02f, 8.0f);
    glVertex3f(40.0f, 0.02f, 8.0f);
    glEnd();

    glColor3f(0.95f, 0.95f, 0.10f);
    glBegin(GL_LINES);
    for (float x = -35.0f; x < 35.0f; x += 10.0f) {
        glVertex3f(x, 0.025f, -20.0f);
        glVertex3f(x + 5.0f, 0.025f, -20.0f);
        glVertex3f(x, 0.025f, 20.0f);
        glVertex3f(x + 5.0f, 0.025f, 20.0f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void Scene::drawRoadDetail() const {
    glDisable(GL_LIGHTING);
    glColor3f(0.82f, 0.82f, 0.82f);
    glBegin(GL_LINES);
    for (float z = -78.0f; z < 78.0f; z += 8.0f) {
        glVertex3f(-60.0f, 0.026f, z);
        glVertex3f(-60.0f, 0.026f, z + 4.0f);
        glVertex3f(60.0f, 0.026f, z);
        glVertex3f(60.0f, 0.026f, z + 4.0f);
    }
    glEnd();
    glColor3f(0.95f, 0.75f, 0.08f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-40.0f, 0.026f, -24.0f);
    glVertex3f(40.0f, 0.026f, -24.0f);
    glVertex3f(40.0f, 0.026f, 24.0f);
    glVertex3f(-40.0f, 0.026f, 24.0f);
    glEnd();
    glEnable(GL_LIGHTING);
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

        glPushMatrix();
        glTranslatef(x, y, z);
        applyMaterial(0.45f, 0.26f, 0.12f, 10.0f, 0.06f);
        drawCylinder(0.42f, 3.8f, 10);
        glTranslatef(0.0f, 4.0f, 0.0f);
        applyMaterial(0.11f, 0.46f, 0.16f, 14.0f, 0.08f);
        glutSolidSphere(1.9f, 12, 12);
        glPopMatrix();
    }

    for (int i = 0; i < 10; ++i) {
        const float x = 58.0f + static_cast<float>(i % 3) * 7.0f;
        const float z = -65.0f + static_cast<float>(i / 3) * 9.0f;
        const float y = terrainHeight(x, z);

        glPushMatrix();
        glTranslatef(x, y, z);
        applyMaterial(0.42f, 0.24f, 0.12f, 10.0f, 0.06f);
        drawCylinder(0.38f, 3.2f, 10);
        glTranslatef(0.0f, 3.2f, 0.0f);
        applyMaterial(0.06f, 0.38f, 0.12f, 14.0f, 0.08f);
        glutSolidCone(1.8f, 3.2f, 10, 4);
        glPopMatrix();
    }

    // Roadside tree rows.
    for (int i = -8; i <= 8; ++i) {
        const float z = i * 10.0f;
        const float xLeft = -74.0f;
        const float xRight = 74.0f;
        const float yLeft = terrainHeight(xLeft, z);
        const float yRight = terrainHeight(xRight, z);

        glPushMatrix();
        glTranslatef(xLeft, yLeft, z);
        applyMaterial(0.42f, 0.22f, 0.10f, 10.0f, 0.05f);
        drawCylinder(0.32f, 3.0f, 10);
        glTranslatef(0.0f, 3.1f, 0.0f);
        applyMaterial(0.08f, 0.42f, 0.14f, 12.0f, 0.06f);
        glutSolidCone(1.7f, 2.8f, 10, 4);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(xRight, yRight, z);
        applyMaterial(0.42f, 0.22f, 0.10f, 10.0f, 0.05f);
        drawCylinder(0.32f, 3.0f, 10);
        glTranslatef(0.0f, 3.1f, 0.0f);
        applyMaterial(0.08f, 0.42f, 0.14f, 12.0f, 0.06f);
        glutSolidCone(1.7f, 2.8f, 10, 4);
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
