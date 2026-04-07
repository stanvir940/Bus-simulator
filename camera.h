#pragma once

#include "bus.h"

enum class CameraMode {
    TOP = 0,
    DRIVER = 1,
    FREE = 2,
};

// While driving (DRIVER mode), cycle with V.
enum class DriverCamStyle {
    COCKPIT = 0,
    CHASE_BACK,
    CHASE_FRONT,
    SIDE_LEFT,
    SIDE_RIGHT,
};

class CameraController {
public:
    CameraController();

    void setMode(CameraMode mode);
    CameraMode getMode() const;

    void setInteriorMode(bool interior);
    bool isInteriorMode() const;

    void cycleDriverCam();

    void getViewMatrix(const Bus& driverBus, float* outColumnMajor16) const;
    void getCameraWorldPosition(const Bus& driverBus, float* outXYZ) const;

    void onKeyboard(int key);
    void onSpecial(int key, bool pressed);
    void onMouseButton(int button, int action, double x, double y);
    void onMouseMove(double x, double y);

private:
    CameraMode m_mode;
    DriverCamStyle m_driverCam;
    bool m_interiorMode;
    float m_yawDeg;
    float m_pitchDeg;
    float m_distance;
    float m_panX;
    float m_panY;
    float m_panZ;
    bool m_leftMouseDown;
    double m_lastMouseX;
    double m_lastMouseY;
};
