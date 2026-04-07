#include "camera.h"

#include "math_gl.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;
}

CameraController::CameraController()
    : m_mode(CameraMode::FREE),
      m_driverCam(DriverCamStyle::COCKPIT),
      m_interiorMode(false),
      m_yawDeg(35.0f),
      m_pitchDeg(22.0f),
      m_distance(65.0f),
      m_panX(0.0f),
      m_panY(0.0f),
      m_panZ(0.0f),
      m_leftMouseDown(false),
      m_lastMouseX(0.0),
      m_lastMouseY(0.0) {}

void CameraController::setMode(CameraMode mode) {
    m_mode = mode;
}

CameraMode CameraController::getMode() const {
    return m_mode;
}

void CameraController::setInteriorMode(bool interior) {
    m_interiorMode = interior;
}

bool CameraController::isInteriorMode() const {
    return m_interiorMode;
}

void CameraController::cycleDriverCam() {
    int n = static_cast<int>(DriverCamStyle::SIDE_RIGHT) + 1;
    int k = (static_cast<int>(m_driverCam) + 1) % n;
    m_driverCam = static_cast<DriverCamStyle>(k);
}

void CameraController::getCameraWorldPosition(const Bus& driverBus, float* outXYZ) const {
    if (m_interiorMode) {
        outXYZ[0] = 0.0f;
        outXYZ[1] = 1.65f;
        outXYZ[2] = 8.5f;
        return;
    }
    if (m_mode == CameraMode::TOP) {
        outXYZ[0] = 0.0f;
        outXYZ[1] = 78.0f;
        outXYZ[2] = 0.01f;
        return;
    }

    if (m_mode == CameraMode::DRIVER) {
        const Vec3 pos = driverBus.getPosition();
        const float heading = driverBus.getHeadingDegrees() * (PI / 180.0f);
        const float fx = std::cos(heading);
        const float fz = std::sin(heading);
        const float rx = -fz;
        const float rz = fx;
        switch (m_driverCam) {
            case DriverCamStyle::COCKPIT:
                outXYZ[0] = pos.x + fx * 2.6f;
                outXYZ[1] = 1.9f;
                outXYZ[2] = pos.z + fz * 2.6f;
                break;
            case DriverCamStyle::CHASE_BACK:
                outXYZ[0] = pos.x - fx * 16.0f;
                outXYZ[1] = 6.0f;
                outXYZ[2] = pos.z - fz * 16.0f;
                break;
            case DriverCamStyle::CHASE_FRONT:
                outXYZ[0] = pos.x + fx * 14.0f;
                outXYZ[1] = 5.0f;
                outXYZ[2] = pos.z + fz * 14.0f;
                break;
            case DriverCamStyle::SIDE_LEFT:
                outXYZ[0] = pos.x + rx * 9.0f;
                outXYZ[1] = 3.6f;
                outXYZ[2] = pos.z + rz * 9.0f;
                break;
            case DriverCamStyle::SIDE_RIGHT:
                outXYZ[0] = pos.x - rx * 9.0f;
                outXYZ[1] = 3.6f;
                outXYZ[2] = pos.z - rz * 9.0f;
                break;
        }
        return;
    }

    const float yaw = m_yawDeg * (PI / 180.0f);
    const float pitch = m_pitchDeg * (PI / 180.0f);
    outXYZ[0] = m_panX + m_distance * std::cos(pitch) * std::cos(yaw);
    outXYZ[1] = m_panY + m_distance * std::sin(pitch);
    outXYZ[2] = m_panZ + m_distance * std::cos(pitch) * std::sin(yaw);
}

void CameraController::getViewMatrix(const Bus& driverBus, float* out) const {
    float eye[3];
    float center[3];
    float up[3] = {0.0f, 1.0f, 0.0f};

    if (m_interiorMode) {
        eye[0] = 0.0f;
        eye[1] = 1.65f;
        eye[2] = 8.5f;
        center[0] = 0.0f;
        center[1] = 1.35f;
        center[2] = -1.0f;
        mat4LookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2], out);
        return;
    }

    if (m_mode == CameraMode::TOP) {
        eye[0] = 0.0f;
        eye[1] = 78.0f;
        eye[2] = 0.01f;
        center[0] = 0.0f;
        center[1] = 0.0f;
        center[2] = 0.0f;
        up[0] = 0.0f;
        up[1] = 0.0f;
        up[2] = -1.0f;
    } else if (m_mode == CameraMode::DRIVER) {
        const Vec3 pos = driverBus.getPosition();
        const float heading = driverBus.getHeadingDegrees() * (PI / 180.0f);
        const float fx = std::cos(heading);
        const float fz = std::sin(heading);
        const float rx = -fz;
        const float rz = fx;
        getCameraWorldPosition(driverBus, eye);
        switch (m_driverCam) {
            case DriverCamStyle::COCKPIT:
                center[0] = pos.x + fx * 10.0f;
                center[1] = 1.8f;
                center[2] = pos.z + fz * 10.0f;
                break;
            case DriverCamStyle::CHASE_BACK:
                center[0] = pos.x + fx * 3.0f;
                center[1] = 2.0f;
                center[2] = pos.z + fz * 3.0f;
                break;
            case DriverCamStyle::CHASE_FRONT:
                center[0] = pos.x - fx * 5.0f;
                center[1] = 1.6f;
                center[2] = pos.z - fz * 5.0f;
                break;
            case DriverCamStyle::SIDE_LEFT:
            case DriverCamStyle::SIDE_RIGHT:
                center[0] = pos.x + fx * 8.0f;
                center[1] = 1.8f;
                center[2] = pos.z + fz * 8.0f;
                break;
        }
        mat4LookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2], out);
        return;
    } else {
        getCameraWorldPosition(driverBus, eye);
        center[0] = m_panX;
        center[1] = m_panY;
        center[2] = m_panZ;
    }

    mat4LookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2], out);
}

void CameraController::onKeyboard(int key) {
    switch (key) {
        case GLFW_KEY_1:
            setMode(CameraMode::TOP);
            break;
        case GLFW_KEY_2:
            setMode(CameraMode::DRIVER);
            break;
        case GLFW_KEY_3:
            setMode(CameraMode::FREE);
            break;
        case GLFW_KEY_V:
            if (m_mode == CameraMode::DRIVER) {
                cycleDriverCam();
            }
            break;
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD:
            m_distance -= 2.0f;
            if (m_distance < 10.0f) {
                m_distance = 10.0f;
            }
            break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT:
            m_distance += 2.0f;
            if (m_distance > 220.0f) {
                m_distance = 220.0f;
            }
            break;
        case GLFW_KEY_I:
            m_panY += 1.0f;
            break;
        case GLFW_KEY_K:
            m_panY -= 1.0f;
            break;
        case GLFW_KEY_J:
            m_panX -= 1.0f;
            break;
        case GLFW_KEY_L:
            m_panX += 1.0f;
            break;
        case GLFW_KEY_W:
            if (m_mode == CameraMode::FREE) {
                m_panZ -= 1.5f;
            }
            break;
        case GLFW_KEY_S:
            if (m_mode == CameraMode::FREE) {
                m_panZ += 1.5f;
            }
            break;
        case GLFW_KEY_A:
            if (m_mode == CameraMode::FREE) {
                m_panX -= 1.5f;
            }
            break;
        case GLFW_KEY_D:
            if (m_mode == CameraMode::FREE) {
                m_panX += 1.5f;
            }
            break;
        default:
            break;
    }
}

void CameraController::onSpecial(int key, bool pressed) {
    if (!pressed || m_interiorMode) {
        return;
    }
    const float rotStep = 2.0f;
    if (key == GLFW_KEY_LEFT) {
        m_yawDeg -= rotStep;
    } else if (key == GLFW_KEY_RIGHT) {
        m_yawDeg += rotStep;
    } else if (key == GLFW_KEY_UP) {
        m_pitchDeg += rotStep;
        if (m_pitchDeg > 85.0f) {
            m_pitchDeg = 85.0f;
        }
    } else if (key == GLFW_KEY_DOWN) {
        m_pitchDeg -= rotStep;
        if (m_pitchDeg < -10.0f) {
            m_pitchDeg = -10.0f;
        }
    }
}

void CameraController::onMouseButton(int button, int action, double x, double y) {
    if (m_interiorMode) {
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_leftMouseDown = (action == GLFW_PRESS);
        m_lastMouseX = x;
        m_lastMouseY = y;
    }
}

void CameraController::onMouseMove(double x, double y) {
    if (!m_leftMouseDown || m_interiorMode) {
        return;
    }
    const double dx = x - m_lastMouseX;
    const double dy = y - m_lastMouseY;
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
