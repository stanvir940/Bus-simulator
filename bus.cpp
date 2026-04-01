#include "bus.h"
#include "texture_util.h"

#include <GLUT/glut.h>
#include <cstdio>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;
}

unsigned int Bus::s_bodyTexture = 0;
unsigned int Bus::s_frontTexture = 0;
unsigned int Bus::s_backTexture = 0;
unsigned int Bus::s_windowTexture = 0;
bool Bus::s_graphicsInitialized = false;
bool Bus::s_bodyTextureOk = false;
bool Bus::s_frontTextureOk = false;
bool Bus::s_backTextureOk = false;
bool Bus::s_windowTextureOk = false;

namespace {

unsigned int loadNamedTexture(const char* baseFileStem) {
    char pathBuf[8][256];
    const char* paths[8];
    int n = 0;
    const char* prefixes[] = {
        "textures/",
        "../textures/",
        "../../BusStandSimulator/textures/",
    };
    const char* exts[] = {".png", ".jpg"};
    for (const char* pre : prefixes) {
        for (const char* ext : exts) {
            if (n >= 8) {
                break;
            }
            const int written = snprintf(pathBuf[n], sizeof(pathBuf[n]), "%s%s%s", pre, baseFileStem, ext);
            if (written > 0 && written < static_cast<int>(sizeof(pathBuf[n]))) {
                paths[n] = pathBuf[n];
                ++n;
            }
        }
        if (n >= 8) {
            break;
        }
    }
    return loadTextureFromSearchPaths(paths, n);
}

}  // namespace

bool Bus::hasAnyBusTexture() {
    return s_bodyTextureOk || s_frontTextureOk || s_backTextureOk || s_windowTextureOk;
}

void Bus::initGraphics() {
    if (s_graphicsInitialized) {
        return;
    }
    s_graphicsInitialized = true;

    const char* bodyOnlyPaths[] = {
        "textures/bus_body.png",
        "textures/bus_body.jpg",
        "textures/bus_body1.png",
        "textures/bus_body1.jpg",
        "../textures/bus_body.png",
        "../textures/bus_body.jpg",
        "../textures/bus_body1.png",
        "../../BusStandSimulator/textures/bus_body.png",
        "../../BusStandSimulator/textures/bus_body.jpg",
        "../../BusStandSimulator/textures/bus_body1.png",
    };
    s_bodyTexture =
        loadTextureFromSearchPaths(bodyOnlyPaths, static_cast<int>(sizeof(bodyOnlyPaths) / sizeof(bodyOnlyPaths[0])));
    s_bodyTextureOk = (s_bodyTexture != 0);

    s_frontTexture = loadNamedTexture("bus_front");
    s_frontTextureOk = (s_frontTexture != 0);

    s_backTexture = loadNamedTexture("bus_back");
    s_backTextureOk = (s_backTexture != 0);

    s_windowTexture = loadNamedTexture("bus_window");
    s_windowTextureOk = (s_windowTexture != 0);
}

Bus::Bus(float speedUnitsPerSec, float initialDistance, float busLength, float busWidth, float busHeight)
    : m_speed(speedUnitsPerSec),
      m_distance(initialDistance),
      m_length(busLength),
      m_width(busWidth),
      m_height(busHeight),
      m_autopilot(true),
      m_manualPos{-34.0f, 0.0f, -20.0f},
      m_manualHeadingDeg(0.0f),
      m_manualSpeed(0.0f),
      m_steerAngleDeg(0.0f),
      m_wheelBase(3.2f),
      m_maxSteerDeg(30.0f),
      m_totalPathLength(0.0f) {
    m_pathPoints = {
        {-34.0f, 0.0f, -20.0f},
        {34.0f, 0.0f, -20.0f},
        {34.0f, 0.0f, 20.0f},
        {-34.0f, 0.0f, 20.0f},
    };

    for (size_t i = 0; i < m_pathPoints.size(); ++i) {
        const Vec3& a = m_pathPoints[i];
        const Vec3& b = m_pathPoints[(i + 1) % m_pathPoints.size()];
        const float dx = b.x - a.x;
        const float dz = b.z - a.z;
        const float len = std::sqrt(dx * dx + dz * dz);
        m_segmentLengths.push_back(len);
        m_totalPathLength += len;
    }

    m_distance = wrapDistance(m_distance);
}

float Bus::wrapDistance(float value) const {
    if (m_totalPathLength <= 0.0f) {
        return 0.0f;
    }
    while (value >= m_totalPathLength) {
        value -= m_totalPathLength;
    }
    while (value < 0.0f) {
        value += m_totalPathLength;
    }
    return value;
}

void Bus::update(float dt) {
    if (!m_autopilot) {
        return;
    }
    m_distance = wrapDistance(m_distance + m_speed * dt);
}

void Bus::setAutopilot(bool enabled) {
    m_autopilot = enabled;
    if (!m_autopilot) {
        m_manualPos = evaluatePath(m_distance, nullptr);
        m_manualHeadingDeg = getHeadingDegrees();
        m_manualSpeed = 0.0f;
        m_steerAngleDeg = 0.0f;
    }
}

bool Bus::isAutopilot() const {
    return m_autopilot;
}

void Bus::control(bool accelerate, bool brake, bool turnLeft, bool turnRight, float dt) {
    if (m_autopilot) {
        return;
    }

    const float engineAccel = 16.0f;
    const float brakeAccel = 24.0f;
    const float rolling = 1.9f;
    const float aeroDrag = 0.055f;
    const float maxForward = 22.0f;
    const float maxReverse = -5.5f;

    float throttle = 0.0f;
    if (accelerate) throttle += 1.0f;
    if (brake) throttle -= 1.0f;

    float accel = 0.0f;
    if (throttle > 0.0f) {
        accel += engineAccel * throttle;
    } else if (throttle < 0.0f) {
        accel += brakeAccel * throttle;
    }

    accel -= rolling * m_manualSpeed;
    accel -= aeroDrag * m_manualSpeed * std::fabs(m_manualSpeed);
    m_manualSpeed += accel * dt;

    if (m_manualSpeed > maxForward) m_manualSpeed = maxForward;
    if (m_manualSpeed < maxReverse) m_manualSpeed = maxReverse;

    float steerInput = 0.0f;
    if (turnLeft) steerInput += 1.0f;
    if (turnRight) steerInput -= 1.0f;

    const float speedFactor = 1.0f - std::fmin(std::fabs(m_manualSpeed) / maxForward, 0.7f);
    const float targetSteer = steerInput * (m_maxSteerDeg * speedFactor);
    const float steerResponse = 120.0f;
    const float steerDelta = targetSteer - m_steerAngleDeg;
    const float steerStep = steerResponse * dt;
    if (steerDelta > steerStep) {
        m_steerAngleDeg += steerStep;
    } else if (steerDelta < -steerStep) {
        m_steerAngleDeg -= steerStep;
    } else {
        m_steerAngleDeg = targetSteer;
    }

    const float headingRad = m_manualHeadingDeg * (PI / 180.0f);
    const float steerRad = m_steerAngleDeg * (PI / 180.0f);
    const float yawRateRad = (m_wheelBase > 0.01f) ? (m_manualSpeed / m_wheelBase) * std::tan(steerRad) : 0.0f;
    m_manualHeadingDeg += yawRateRad * dt * (180.0f / PI);

    const float newHeadingRad = m_manualHeadingDeg * (PI / 180.0f);
    m_manualPos.x += std::cos(newHeadingRad) * m_manualSpeed * dt;
    m_manualPos.z += std::sin(newHeadingRad) * m_manualSpeed * dt;

    if (m_manualPos.x < -77.0f) m_manualPos.x = -77.0f;
    if (m_manualPos.x > 77.0f) m_manualPos.x = 77.0f;
    if (m_manualPos.z < -77.0f) m_manualPos.z = -77.0f;
    if (m_manualPos.z > 77.0f) m_manualPos.z = 77.0f;
}

Vec3 Bus::evaluatePath(float distance, Vec3* tangent) const {
    float remaining = wrapDistance(distance);

    for (size_t i = 0; i < m_segmentLengths.size(); ++i) {
        const float segLen = m_segmentLengths[i];
        if (remaining <= segLen) {
            const Vec3& a = m_pathPoints[i];
            const Vec3& b = m_pathPoints[(i + 1) % m_pathPoints.size()];
            const float t = (segLen > 0.0f) ? (remaining / segLen) : 0.0f;
            Vec3 pos = {
                a.x + (b.x - a.x) * t,
                0.0f,
                a.z + (b.z - a.z) * t,
            };

            if (tangent != nullptr) {
                tangent->x = b.x - a.x;
                tangent->y = 0.0f;
                tangent->z = b.z - a.z;
            }
            return pos;
        }
        remaining -= segLen;
    }

    if (tangent != nullptr) {
        tangent->x = 1.0f;
        tangent->y = 0.0f;
        tangent->z = 0.0f;
    }
    return m_pathPoints.front();
}

Vec3 Bus::getPosition() const {
    if (!m_autopilot) {
        return m_manualPos;
    }
    return evaluatePath(m_distance, nullptr);
}

float Bus::getHeadingDegrees() const {
    if (!m_autopilot) {
        return m_manualHeadingDeg;
    }
    Vec3 tangent{};
    evaluatePath(m_distance, &tangent);
    return std::atan2(tangent.z, tangent.x) * (180.0f / PI);
}

void Bus::drawUnitCylinder(float radius, float length, int slices) const {
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    GLUquadric* quadric = gluNewQuadric();
    gluCylinder(quadric, radius, radius, length, slices, 1);
    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void Bus::drawTexturedQuadFace(unsigned int tex, float r, float g, float b, float nx, float ny, float nz,
    float u0, float v0, float u1, float v1, const float* verts12) const {
    if (tex != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_COLOR_MATERIAL);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_COLOR_MATERIAL);
        glColor3f(r, g, b);
    }
    glNormal3f(nx, ny, nz);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex3f(verts12[0], verts12[1], verts12[2]);
    glTexCoord2f(u1, v0);
    glVertex3f(verts12[3], verts12[4], verts12[5]);
    glTexCoord2f(u1, v1);
    glVertex3f(verts12[6], verts12[7], verts12[8]);
    glTexCoord2f(u0, v1);
    glVertex3f(verts12[9], verts12[10], verts12[11]);
    glEnd();
}

void Bus::drawTexturedBodyBox(float sx, float sy, float sz) const {
    const float hx = sx * 0.5f;
    const float hy = sy * 0.5f;
    const float hz = sz * 0.5f;

    // One texture per logical face: front = +X (direction of travel after Y-rotation),
    // rear = -X, long sides = ±Z use bus_window, roof/underbody use bus_body.
    const unsigned int texRight = s_windowTextureOk ? s_windowTexture : (s_bodyTextureOk ? s_bodyTexture : 0);
    const unsigned int texLeft = s_windowTextureOk ? s_windowTexture : (s_bodyTextureOk ? s_bodyTexture : 0);
    const unsigned int texFront = s_frontTextureOk ? s_frontTexture : (s_bodyTextureOk ? s_bodyTexture : 0);
    const unsigned int texRear = s_backTextureOk ? s_backTexture : (s_bodyTextureOk ? s_bodyTexture : 0);
    const unsigned int texTop = s_bodyTextureOk ? s_bodyTexture : 0;
    const unsigned int texBottom = s_bodyTextureOk ? s_bodyTexture : 0;

    const float sideR = 0.12f;
    const float sideG = 0.12f;
    const float sideB = 0.14f;
    const float frontR = 0.14f;
    const float frontG = 0.14f;
    const float frontB = 0.16f;
    const float rearR = 0.10f;
    const float rearG = 0.10f;
    const float rearB = 0.12f;
    const float topR = 0.16f;
    const float topG = 0.16f;
    const float topB = 0.18f;
    const float bottomR = 0.05f;
    const float bottomG = 0.05f;
    const float bottomB = 0.06f;

    float v[12] = {};

    // +Z (length along X)
    v[0] = -hx;
    v[1] = -hy;
    v[2] = hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    // +Z = right side of bus (when facing +X forward); U along bus length.
    drawTexturedQuadFace(texRight, sideR, sideG, sideB, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f, 1.0f, v);

    // -Z
    v[0] = -hx;
    v[1] = -hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = -hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = -hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = -hz;
    // -Z = left side; mirror U vs +Z so the window texture faces outward correctly.
    drawTexturedQuadFace(texLeft, sideR, sideG, sideB, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f, 0.0f, 1.0f, v);

    // +X (front cap — bus_front)
    v[0] = hx;
    v[1] = -hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = hx;
    v[10] = hy;
    v[11] = -hz;
    drawTexturedQuadFace(texFront, frontR, frontG, frontB, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, v);

    // -X (rear cap — bus_back)
    v[0] = -hx;
    v[1] = -hy;
    v[2] = hz;
    v[3] = -hx;
    v[4] = -hy;
    v[5] = -hz;
    v[6] = -hx;
    v[7] = hy;
    v[8] = -hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    // Rear: flip U so “bus_back” reads correctly from outside (-X).
    drawTexturedQuadFace(texRear, rearR, rearG, rearB, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, v);

    // +Y (roof — bus_body livery under green strip)
    v[0] = -hx;
    v[1] = hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = hy;
    v[5] = -hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    drawTexturedQuadFace(texTop, topR, topG, topB, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 2.0f, v);

    // -Y (underbody)
    v[0] = -hx;
    v[1] = -hy;
    v[2] = hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = hz;
    v[6] = hx;
    v[7] = -hy;
    v[8] = -hz;
    v[9] = -hx;
    v[10] = -hy;
    v[11] = -hz;
    drawTexturedQuadFace(texBottom, bottomR, bottomG, bottomB, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 2.0f, v);

    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
}

void Bus::drawWindowBandTextured(float sx, float sy, float sz) const {
    if (!s_windowTextureOk) {
        glEnable(GL_COLOR_MATERIAL);
        glColor3f(0.78f, 0.84f, 0.94f);
        glPushMatrix();
        glScalef(sx, sy, sz);
        glutSolidCube(1.0f);
        glPopMatrix();
        return;
    }

    const float hx = sx * 0.5f;
    const float hy = sy * 0.5f;
    const float hz = sz * 0.5f;

    float v[12] = {};

    // +Z
    v[0] = -hx;
    v[1] = -hy;
    v[2] = hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    drawTexturedQuadFace(s_windowTexture, 0.8f, 0.85f, 0.92f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, v);

    // -Z
    v[0] = -hx;
    v[1] = -hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = -hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = -hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = -hz;
    drawTexturedQuadFace(s_windowTexture, 0.8f, 0.85f, 0.92f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, v);

    // +X
    v[0] = hx;
    v[1] = -hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = -hy;
    v[5] = hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = hx;
    v[10] = hy;
    v[11] = -hz;
    drawTexturedQuadFace(s_windowTexture, 0.8f, 0.85f, 0.92f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, v);

    // -X
    v[0] = -hx;
    v[1] = -hy;
    v[2] = hz;
    v[3] = -hx;
    v[4] = -hy;
    v[5] = -hz;
    v[6] = -hx;
    v[7] = hy;
    v[8] = -hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    drawTexturedQuadFace(s_windowTexture, 0.8f, 0.85f, 0.92f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, v);

    // +Y
    v[0] = -hx;
    v[1] = hy;
    v[2] = -hz;
    v[3] = hx;
    v[4] = hy;
    v[5] = -hz;
    v[6] = hx;
    v[7] = hy;
    v[8] = hz;
    v[9] = -hx;
    v[10] = hy;
    v[11] = hz;
    drawTexturedQuadFace(s_windowTexture, 0.8f, 0.85f, 0.92f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, v);

    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glColor3f(0.12f, 0.12f, 0.14f);
    glBegin(GL_QUADS);
    glVertex3f(-hx, -hy, hz);
    glVertex3f(hx, -hy, hz);
    glVertex3f(hx, -hy, -hz);
    glVertex3f(-hx, -hy, -hz);
    glEnd();
}

void Bus::drawFallbackBodyBox(float sx, float sy, float sz) const {
    glColor3f(0.06f, 0.06f, 0.08f);
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void Bus::render() const {
    const Vec3 pos = getPosition();
    const float heading = getHeadingDegrees();

    glPushMatrix();
    glTranslatef(pos.x, 0.9f, pos.z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);

    if (hasAnyBusTexture()) {
        drawTexturedBodyBox(m_length, m_height * 0.92f, m_width);
    } else {
        drawFallbackBodyBox(m_length, m_height * 0.92f, m_width);
    }

    glColor3f(0.04f, 0.78f, 0.32f);
    glPushMatrix();
    glTranslatef(0.0f, m_height * 0.48f, 0.0f);
    glScalef(m_length * 0.99f, 0.22f, m_width * 0.98f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    drawWindowBandTextured(m_length * 0.52f, m_height * 0.42f, m_width * 0.88f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.95f, 0.95f, 0.18f);
    glTranslatef(m_length * 0.48f, -0.1f, 0.0f);
    glScalef(0.25f, 0.18f, m_width * 0.78f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.10f, 0.10f, 0.10f);
    const float wheelOffsetX = m_length * 0.30f;
    const float wheelOffsetZ = m_width * 0.55f;
    const float wheelY = -m_height * 0.45f;
    const float wheelRadius = 0.28f;
    const float wheelLength = 0.18f;

    for (int side = -1; side <= 1; side += 2) {
        for (int ax = -1; ax <= 1; ax += 2) {
            glPushMatrix();
            glTranslatef(ax * wheelOffsetX, wheelY, side * wheelOffsetZ);
            drawUnitCylinder(wheelRadius, wheelLength, 16);
            glPopMatrix();
        }
    }

    glPopMatrix();
}
