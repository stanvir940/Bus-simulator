#pragma once

#include <GL/glew.h>

struct DirectionalLight {
    float dir[3];
    float ambient[3];
    float diffuse[3];
    float specular[3];
};

struct PointLight {
    float position[3];
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float constant;
    float linear;
    float quadratic;
};

// Direction from light toward lit surface (spot axis). cosInner > cosOuter (narrower cone = larger cos).
struct Spotlight {
    float position[3];
    float direction[3];
    float cosInner;
    float cosOuter;
    float diffuse[3];
    float specular[3];
    float constant;
    float linear;
    float quadratic;
};

constexpr int kMaxSpotlights = 24;

void applyLightingUniforms(GLuint program, const float* cameraWorldPos3, const float* globalAmbient3,
    const DirectionalLight& sun, int numPoints, const PointLight* points, float specularGlobalScale = 1.0f);

void applySpotlights(GLuint program, int numSpots, const Spotlight* spots);

void applyFogUniforms(GLuint program, float fogDensity, const float* fogColor3);
