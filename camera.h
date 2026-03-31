#pragma once

#include "bus.h"

enum class CameraMode {
    TOP = 0,
    DRIVER = 1,
    FREE = 2,
};

class CameraController {
public:
    CameraController();

    void setMode(CameraMode mode);
    CameraMode getMode() const;

    void applyView(const Bus& driverBus) const;
    void onKeyboard(unsigned char key);
    void onSpecial(int key);
    void onMouseButton(int button, int state, int x, int y);
    void onMouseMove(int x, int y);

private:
    CameraMode m_mode;
    float m_yawDeg;
    float m_pitchDeg;
    float m_distance;
    float m_panX;
    float m_panY;
    bool m_leftMouseDown;
    int m_lastMouseX;
    int m_lastMouseY;
};
