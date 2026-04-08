#include "camera.h"
#include "render_settings.h"
#include "renderer.h"
#include "scene.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace {
Scene g_scene;
CameraController g_camera;
Renderer g_renderer;
double g_prevTime = 0.0;
int g_winWidth = 1280;
int g_winHeight = 720;
WorldMode g_worldMode = WorldMode::Outdoor;
AppRenderSettings g_renderSettings;
enum class SoundMode { None, Outdoor, TicketOffice };
SoundMode g_soundMode = SoundMode::None;

void stopAllSoundNow() {
#if defined(__APPLE__)
    std::system("sh -c 'if [ -f /tmp/busstandsim_sound.pid ]; then kill $(cat /tmp/busstandsim_sound.pid) >/dev/null 2>&1; rm -f /tmp/busstandsim_sound.pid; fi'");
    std::system("pkill -f \"ambient-sound.mp3\" >/dev/null 2>&1");
    std::system("pkill -f \"ticket-counter-sound.mp3\" >/dev/null 2>&1");
    std::system("killall afplay >/dev/null 2>&1");
#endif
    g_soundMode = SoundMode::None;
}

void switchSound(SoundMode mode) {
    if (g_soundMode == mode) {
        return;
    }
#if defined(__APPLE__)
    stopAllSoundNow();
    if (mode == SoundMode::Outdoor) {
        std::system(
            "sh -c '(while true; do afplay ../sounds/ambient-sound.mp3 >/dev/null 2>&1 || afplay sounds/ambient-sound.mp3 >/dev/null 2>&1; sleep 0.2; done) & echo $! > /tmp/busstandsim_sound.pid'");
    } else if (mode == SoundMode::TicketOffice) {
        std::system(
            "sh -c '(while true; do afplay ../sounds/ticket-counter-sound.mp3 >/dev/null 2>&1 || afplay sounds/ticket-counter-sound.mp3 >/dev/null 2>&1; sleep 0.2; done) & echo $! > /tmp/busstandsim_sound.pid'");
    }
#else
    (void)mode;
#endif
    g_soundMode = mode;
}
}

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_winWidth = (w > 1) ? w : 1;
    g_winHeight = (h > 1) ? h : 1;
    glViewport(0, 0, g_winWidth, g_winHeight);
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        stopAllSoundNow();
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    const bool down = (action != GLFW_RELEASE);
    const bool outdoor = (g_worldMode == WorldMode::Outdoor);
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        if (outdoor && (key == GLFW_KEY_I || key == GLFW_KEY_K || key == GLFW_KEY_J || key == GLFW_KEY_L)) {
            g_scene.onKeyState(key, down);
        }
        if (outdoor && (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT || key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)) {
            g_scene.onSpecialState(key, down);
        }
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_B) {
            g_worldMode = (g_worldMode == WorldMode::Outdoor) ? WorldMode::TicketOffice : WorldMode::Outdoor;
            g_camera.setInteriorMode(g_worldMode == WorldMode::TicketOffice);
            if (g_worldMode == WorldMode::TicketOffice) {
                switchSound(SoundMode::TicketOffice);
            } else {
                switchSound(SoundMode::Outdoor);
            }
        } else if (key == GLFW_KEY_LEFT_BRACKET) {
            g_renderSettings.fogDensity = std::max(0.002f, g_renderSettings.fogDensity - 0.002f);
        } else if (key == GLFW_KEY_RIGHT_BRACKET) {
            g_renderSettings.fogDensity = std::min(0.06f, g_renderSettings.fogDensity + 0.002f);
        } else if (key == GLFW_KEY_SEMICOLON) {
            g_renderSettings.globalAmbientScale = std::max(0.15f, g_renderSettings.globalAmbientScale - 0.05f);
        } else if (key == GLFW_KEY_APOSTROPHE) {
            g_renderSettings.globalAmbientScale = std::min(1.8f, g_renderSettings.globalAmbientScale + 0.05f);
        } else if (key == GLFW_KEY_COMMA) {
            g_renderSettings.sunDiffuseScale = std::max(0.1f, g_renderSettings.sunDiffuseScale - 0.08f);
        } else if (key == GLFW_KEY_PERIOD) {
            g_renderSettings.sunDiffuseScale = std::min(1.6f, g_renderSettings.sunDiffuseScale + 0.08f);
        } else if (key == GLFW_KEY_7) {
            g_renderSettings.sunAmbientScale = std::max(0.1f, g_renderSettings.sunAmbientScale - 0.08f);
        } else if (key == GLFW_KEY_8) {
            g_renderSettings.sunAmbientScale = std::min(1.6f, g_renderSettings.sunAmbientScale + 0.08f);
        } else if (key == GLFW_KEY_Z) {
            g_renderSettings.sunAzimuthDeg -= 4.0f;
        } else if (key == GLFW_KEY_X) {
            g_renderSettings.sunAzimuthDeg += 4.0f;
        } else if (key == GLFW_KEY_C) {
            g_renderSettings.sunElevationDeg = std::max(8.0f, g_renderSettings.sunElevationDeg - 3.0f);
        } else if (key == GLFW_KEY_V) {
            g_renderSettings.sunElevationDeg = std::min(85.0f, g_renderSettings.sunElevationDeg + 3.0f);
        } else if (key == GLFW_KEY_MINUS) {
            g_renderSettings.specularScale = std::max(0.0f, g_renderSettings.specularScale - 0.12f);
        } else if (key == GLFW_KEY_EQUAL) {
            g_renderSettings.specularScale = std::min(2.2f, g_renderSettings.specularScale + 0.12f);
        } else if (key == GLFW_KEY_P) {
            g_renderSettings.pointLightsEnabled = !g_renderSettings.pointLightsEnabled;
        } else if (key == GLFW_KEY_O) {
            g_renderSettings.spotLightsEnabled = !g_renderSettings.spotLightsEnabled;
        } else if (key == GLFW_KEY_M) {
            g_scene.toggleAutoBus1();
        } else if (key == GLFW_KEY_N) {
            g_scene.toggleAutoBus2();
        } else if (key == GLFW_KEY_H) {
            g_renderSettings.nightMode = !g_renderSettings.nightMode;
        } else if (key == GLFW_KEY_R) {
            g_renderSettings.rainEnabled = !g_renderSettings.rainEnabled;
        } else if (key == GLFW_KEY_T) {
            g_renderSettings.sunEnabled = !g_renderSettings.sunEnabled;
        } else if (key == GLFW_KEY_L) {
            const bool on = !(g_renderSettings.pointLightsEnabled || g_renderSettings.spotLightsEnabled);
            g_renderSettings.pointLightsEnabled = on;
            g_renderSettings.spotLightsEnabled = on;
        } else {
            g_camera.onKeyboard(key);
        }
    }
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT || key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
            g_camera.onSpecial(key, down);
        }
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    double x = 0, y = 0;
    glfwGetCursorPos(window, &x, &y);
    g_camera.onMouseButton(button, action, x, y);
}

static void cursorPosCallback(GLFWwindow*, double x, double y) {
    g_camera.onMouseMove(x, y);
}

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Bus Stand Simulator", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    glewExperimental = GL_TRUE;
    const GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::fprintf(stderr, "glewInit: %s\n", reinterpret_cast<const char*>(glewGetErrorString(glewErr)));
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glGetError();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.60f, 0.82f, 0.95f, 1.0f);

    g_scene.initGlResources();

    const char* shaderDirs[] = {
#ifdef SHADER_DIR
        SHADER_DIR,
#endif
        "shaders",
        "../shaders",
        "../../BusStandSimulator/shaders",
    };
    bool rendererOk = false;
    for (const char* d : shaderDirs) {
        if (d == nullptr || d[0] == '\0') {
            continue;
        }
        if (g_renderer.init(d, g_scene)) {
            rendererOk = true;
            break;
        }
    }
    if (!rendererOk) {
        std::fprintf(stderr, "Renderer init failed. Run from build dir with shaders/ copied next to executable, or set SHADER_DIR.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwGetFramebufferSize(window, &g_winWidth, &g_winHeight);
    glViewport(0, 0, g_winWidth, g_winHeight);

    if (access("../sounds/ambient-sound.mp3", F_OK) != 0 && access("sounds/ambient-sound.mp3", F_OK) != 0) {
        std::fprintf(stderr, "Warning: ambient sound file not found in ../sounds or sounds\n");
    }

    g_prevTime = glfwGetTime();
    switchSound(SoundMode::Outdoor);

    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        float dt = static_cast<float>(now - g_prevTime);
        g_prevTime = now;
        if (dt < 0.0f) {
            dt = 0.0f;
        } else if (dt > 0.05f) {
            dt = 0.05f;
        }

        g_scene.update(dt);

        glClearColor(g_renderSettings.fogColor[0], g_renderSettings.fogColor[1], g_renderSettings.fogColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_renderer.draw(g_scene, g_camera, g_scene.getDriverBus(), g_winWidth, g_winHeight, g_scene.timeSec(), g_worldMode,
            g_renderSettings);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    stopAllSoundNow();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
