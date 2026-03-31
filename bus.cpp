#include "bus.h"

#include <GLUT/glut.h>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;
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

void Bus::render() const {
    const Vec3 pos = getPosition();
    const float heading = getHeadingDegrees();

    glPushMatrix();
    glTranslatef(pos.x, 0.9f, pos.z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glColor3f(0.08f, 0.28f, 0.80f);
    glScalef(m_length, m_height, m_width);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.78f, 0.84f, 0.94f);
    glTranslatef(0.0f, 0.25f, 0.0f);
    glScalef(m_length * 0.55f, m_height * 0.55f, m_width * 0.85f);
    glutSolidCube(1.0f);
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
