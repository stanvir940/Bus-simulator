#include "lighting.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

void setVec3(GLint loc, float x, float y, float z) {
    if (loc >= 0) {
        glUniform3f(loc, x, y, z);
    }
}

}  // namespace

void applyLightingUniforms(GLuint program, const float* cameraWorldPos3, const float* globalAmbient3,
    const DirectionalLight& sun, int numPoints, const PointLight* points, float specularGlobalScale) {
    glUseProgram(program);

    GLint uCam = glGetUniformLocation(program, "uCameraPos");
    setVec3(uCam, cameraWorldPos3[0], cameraWorldPos3[1], cameraWorldPos3[2]);

    GLint uGA = glGetUniformLocation(program, "uGlobalAmbient");
    setVec3(uGA, globalAmbient3[0], globalAmbient3[1], globalAmbient3[2]);

    GLint uDir = glGetUniformLocation(program, "uDirDirection");
    float dx = sun.dir[0];
    float dy = sun.dir[1];
    float dz = sun.dir[2];
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len > 1e-6f) {
        dx /= len;
        dy /= len;
        dz /= len;
    }
    setVec3(uDir, dx, dy, dz);

    setVec3(glGetUniformLocation(program, "uDirAmbient"), sun.ambient[0], sun.ambient[1], sun.ambient[2]);
    setVec3(glGetUniformLocation(program, "uDirDiffuse"), sun.diffuse[0], sun.diffuse[1], sun.diffuse[2]);
    setVec3(glGetUniformLocation(program, "uDirSpecular"), sun.specular[0], sun.specular[1], sun.specular[2]);

    const int n = std::min(numPoints, 8);
    glUniform1i(glGetUniformLocation(program, "uNumPointLights"), n);
    for (int i = 0; i < n; ++i) {
        char buf[48];
        const PointLight& p = points[i];
        std::snprintf(buf, sizeof(buf), "uPointPos[%d]", i);
        setVec3(glGetUniformLocation(program, buf), p.position[0], p.position[1], p.position[2]);
        std::snprintf(buf, sizeof(buf), "uPointAmbient[%d]", i);
        setVec3(glGetUniformLocation(program, buf), p.ambient[0], p.ambient[1], p.ambient[2]);
        std::snprintf(buf, sizeof(buf), "uPointDiffuse[%d]", i);
        setVec3(glGetUniformLocation(program, buf), p.diffuse[0], p.diffuse[1], p.diffuse[2]);
        std::snprintf(buf, sizeof(buf), "uPointSpecular[%d]", i);
        setVec3(glGetUniformLocation(program, buf), p.specular[0], p.specular[1], p.specular[2]);
        std::snprintf(buf, sizeof(buf), "uPointAtten[%d]", i);
        GLint loc = glGetUniformLocation(program, buf);
        if (loc >= 0) {
            glUniform3f(loc, p.constant, p.linear, p.quadratic);
        }
    }

    GLint uSpecG = glGetUniformLocation(program, "uSpecularGlobalScale");
    if (uSpecG >= 0) {
        glUniform1f(uSpecG, specularGlobalScale);
    }
}

void applySpotlights(GLuint program, int numSpots, const Spotlight* spots) {
    glUseProgram(program);
    const int n = std::min(numSpots, kMaxSpotlights);
    glUniform1i(glGetUniformLocation(program, "uNumSpotLights"), n);
    for (int i = 0; i < n; ++i) {
        const Spotlight& s = spots[i];
        char buf[56];
        std::snprintf(buf, sizeof(buf), "uSpotPos[%d]", i);
        setVec3(glGetUniformLocation(program, buf), s.position[0], s.position[1], s.position[2]);
        float dx = s.direction[0];
        float dy = s.direction[1];
        float dz = s.direction[2];
        float L = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (L > 1e-6f) {
            dx /= L;
            dy /= L;
            dz /= L;
        }
        std::snprintf(buf, sizeof(buf), "uSpotDir[%d]", i);
        setVec3(glGetUniformLocation(program, buf), dx, dy, dz);
        std::snprintf(buf, sizeof(buf), "uSpotCosInner[%d]", i);
        GLint li = glGetUniformLocation(program, buf);
        if (li >= 0) {
            glUniform1f(li, s.cosInner);
        }
        std::snprintf(buf, sizeof(buf), "uSpotCosOuter[%d]", i);
        li = glGetUniformLocation(program, buf);
        if (li >= 0) {
            glUniform1f(li, s.cosOuter);
        }
        std::snprintf(buf, sizeof(buf), "uSpotDiffuse[%d]", i);
        setVec3(glGetUniformLocation(program, buf), s.diffuse[0], s.diffuse[1], s.diffuse[2]);
        std::snprintf(buf, sizeof(buf), "uSpotSpecular[%d]", i);
        setVec3(glGetUniformLocation(program, buf), s.specular[0], s.specular[1], s.specular[2]);
        std::snprintf(buf, sizeof(buf), "uSpotAtten[%d]", i);
        li = glGetUniformLocation(program, buf);
        if (li >= 0) {
            glUniform3f(li, s.constant, s.linear, s.quadratic);
        }
    }
}

void applyFogUniforms(GLuint program, float fogDensity, const float* fogColor3) {
    glUseProgram(program);
    GLint fd = glGetUniformLocation(program, "uFogDensity");
    if (fd >= 0) {
        glUniform1f(fd, fogDensity);
    }
    setVec3(glGetUniformLocation(program, "uFogColor"), fogColor3[0], fogColor3[1], fogColor3[2]);
}
