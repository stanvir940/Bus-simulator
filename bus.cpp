#include "bus.h"
#include "texture_util.h"

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
    char pathBuf[12][256];
    const char* paths[12];
    int n = 0;
    const char* prefixes[] = {
        "textures/",
        "../textures/",
        "../../BusStandSimulator/textures/",
    };
    const char* exts[] = {".png", ".jpg", ".webp"};
    for (const char* pre : prefixes) {
        for (const char* ext : exts) {
            if (n >= 12) {
                break;
            }
            const int written = snprintf(pathBuf[n], sizeof(pathBuf[n]), "%s%s%s", pre, baseFileStem, ext);
            if (written > 0 && written < static_cast<int>(sizeof(pathBuf[n]))) {
                paths[n] = pathBuf[n];
                ++n;
            }
        }
        if (n >= 12) {
            break;
        }
    }
    return loadTextureFromSearchPaths(paths, n);
}

}  // namespace

bool Bus::hasAnyBusTexture() {
    return s_bodyTextureOk || s_frontTextureOk || s_backTextureOk || s_windowTextureOk;
}

unsigned int Bus::bodyTextureId() {
    return s_bodyTexture;
}
unsigned int Bus::frontTextureId() {
    return s_frontTexture;
}
unsigned int Bus::backTextureId() {
    return s_backTexture;
}
unsigned int Bus::windowTextureId() {
    return s_windowTexture;
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
        "textures/bus_back.jpg",
        "textures/bus_front.jpg",
        "textures/bus_window.webp",
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

    s_windowTexture = loadNamedTexture("bus-window");
    if (s_windowTexture == 0) {
        s_windowTexture = loadNamedTexture("bus_window");
    }
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
