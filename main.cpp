#include "camera.h"
#include "scene.h"

#include <GLUT/glut.h>
#include <cstdlib>

namespace {
Scene g_scene;
CameraController g_camera;
int g_prevTimeMs = 0;
int g_winWidth = 1280;
int g_winHeight = 720;
}

void renderOverlayText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        ++text;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = static_cast<float>(g_winWidth) / static_cast<float>(g_winHeight);
    gluPerspective(60.0f, aspect, 0.1f, 300.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    g_camera.applyView(g_scene.getDriverBus());
    g_scene.render();

    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderOverlayText(0.02f, 0.96f, "Bus Stand Simulator (Fixed Pipeline + GLUT)");
    renderOverlayText(0.02f, 0.92f, "1: Top View, 2: Driver View, 3: Free Cam");
    renderOverlayText(0.02f, 0.88f, "Drive Bus: W/S Accelerate-Brake, A/D or Arrows Steer");
    renderOverlayText(0.02f, 0.84f, "Mouse Drag: Free-Cam Rotate, +/-: Zoom, ESC: Exit");
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void idle() {
    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(nowMs - g_prevTimeMs) / 1000.0f;
    g_prevTimeMs = nowMs;

    if (dt < 0.0f) {
        dt = 0.0f;
    } else if (dt > 0.05f) {
        dt = 0.05f;
    }

    g_scene.update(dt);
    glutPostRedisplay();
}

void reshape(int width, int height) {
    g_winWidth = (width > 1) ? width : 1;
    g_winHeight = (height > 1) ? height : 1;
    glViewport(0, 0, g_winWidth, g_winHeight);
}

void keyboard(unsigned char key, int, int) {
    if (key == 27) {
        std::exit(0);
    }
    g_scene.onKeyState(key, true);
    g_camera.onKeyboard(key);
}

void keyboardUp(unsigned char key, int, int) {
    g_scene.onKeyState(key, false);
}

void special(int key, int, int) {
    g_scene.onSpecialState(key, true);
}

void specialUp(int key, int, int) {
    g_scene.onSpecialState(key, false);
}

void mouseButton(int button, int state, int x, int y) {
    g_camera.onMouseButton(button, state, x, y);
}

void mouseMove(int x, int y) {
    g_camera.onMouseMove(x, y);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(g_winWidth, g_winHeight);
    glutCreateWindow("Bus Stand Simulator");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.60f, 0.82f, 0.95f, 1.0f);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_FOG);
    const GLfloat fogColor[] = {0.62f, 0.78f, 0.92f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.012f);
    glFogi(GL_FOG_MODE, GL_EXP2);

    g_prevTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);

    glutMainLoop();
    return 0;
}
