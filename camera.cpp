#include "camera.h"

#include <GLUT/glut.h>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;
}

CameraController::CameraController()
    : m_mode(CameraMode::FREE),
      m_yawDeg(35.0f),
      m_pitchDeg(22.0f),
      m_distance(65.0f),
      m_panX(0.0f),
      m_panY(0.0f),
      m_leftMouseDown(false),
      m_lastMouseX(0),
      m_lastMouseY(0) {}

void CameraController::setMode(CameraMode mode) {
    m_mode = mode;
}

CameraMode CameraController::getMode() const {
    return m_mode;
}

void CameraController::applyView(const Bus& driverBus) const {
    if (m_mode == CameraMode::TOP) {
        gluLookAt(
            0.0, 78.0, 0.01,
            0.0, 0.0, 0.0,
            0.0, 0.0, -1.0);
        return;
    }

    if (m_mode == CameraMode::DRIVER) {
        const Vec3 pos = driverBus.getPosition();
        const float heading = driverBus.getHeadingDegrees() * (PI / 180.0f);
        const float fx = std::cos(heading);
        const float fz = std::sin(heading);
        gluLookAt(
            pos.x + fx * 2.6f, 1.9f, pos.z + fz * 2.6f,
            pos.x + fx * 10.0f, 1.8f, pos.z + fz * 10.0f,
            0.0, 1.0, 0.0);
        return;
    }

    const float yaw = m_yawDeg * (PI / 180.0f);
    const float pitch = m_pitchDeg * (PI / 180.0f);
    const float cx = m_panX + m_distance * std::cos(pitch) * std::cos(yaw);
    const float cy = m_panY + m_distance * std::sin(pitch);
    const float cz = m_distance * std::cos(pitch) * std::sin(yaw);

    gluLookAt(
        cx, cy, cz,
        m_panX, m_panY, 0.0f,
        0.0, 1.0, 0.0);
}

void CameraController::onKeyboard(unsigned char key) {
    switch (key) {
        case '1':
            setMode(CameraMode::TOP);
            break;
        case '2':
            setMode(CameraMode::DRIVER);
            break;
        case '3':
            setMode(CameraMode::FREE);
            break;
        case '+':
        case '=':
            m_distance -= 2.0f;
            if (m_distance < 10.0f) {
                m_distance = 10.0f;
            }
            break;
        case '-':
        case '_':
            m_distance += 2.0f;
            if (m_distance > 140.0f) {
                m_distance = 140.0f;
            }
            break;
        case 'i':
        case 'I':
            m_panY += 1.0f;
            break;
        case 'k':
        case 'K':
            m_panY -= 1.0f;
            break;
        case 'j':
        case 'J':
            m_panX -= 1.0f;
            break;
        case 'l':
        case 'L':
            m_panX += 1.0f;
            break;
        default:
            break;
    }
}

void CameraController::onSpecial(int key) {
    const float rotStep = 2.0f;
    if (key == GLUT_KEY_LEFT) {
        m_yawDeg -= rotStep;
    } else if (key == GLUT_KEY_RIGHT) {
        m_yawDeg += rotStep;
    } else if (key == GLUT_KEY_UP) {
        m_pitchDeg += rotStep;
        if (m_pitchDeg > 85.0f) {
            m_pitchDeg = 85.0f;
        }
    } else if (key == GLUT_KEY_DOWN) {
        m_pitchDeg -= rotStep;
        if (m_pitchDeg < -10.0f) {
            m_pitchDeg = -10.0f;
        }
    }
}

void CameraController::onMouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        m_leftMouseDown = (state == GLUT_DOWN);
        m_lastMouseX = x;
        m_lastMouseY = y;
    }
}

void CameraController::onMouseMove(int x, int y) {
    if (!m_leftMouseDown) {
        return;
    }
    const int dx = x - m_lastMouseX;
    const int dy = y - m_lastMouseY;
    m_lastMouseX = x;
    m_lastMouseY = y;

    m_yawDeg += static_cast<float>(dx) * 0.3f;
    m_pitchDeg -= static_cast<float>(dy) * 0.2f;
    if (m_pitchDeg > 85.0f) {
        m_pitchDeg = 85.0f;
    }
    if (m_pitchDeg < -10.0f) {
        m_pitchDeg = -10.0f;
    }
}
