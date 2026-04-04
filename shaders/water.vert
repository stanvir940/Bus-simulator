#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;
uniform vec2 uLakeCenterXZ;
uniform float uLakeFadeScale;

out vec3 vWorldPos;
out vec3 vNormal;
out float vWave;
out float vEdgeFade;

void main() {
    vec3 p = aPos;
    float w = sin(p.x * 0.22 + uTime * 1.4) * 0.055 + cos(p.z * 0.19 + uTime * 1.05) * 0.048;
    p.y += w;
    vec4 wp = uModel * vec4(p, 1.0);
    vWorldPos = wp.xyz;
    mat3 normalMat = mat3(transpose(inverse(uModel)));
    vNormal = normalize(normalMat * aNormal);
    vWave = w;
    vec2 d = p.xz - uLakeCenterXZ;
    vEdgeFade = 1.0 - smoothstep(0.78, 1.02, length(d) * uLakeFadeScale);
    gl_Position = uProjection * uView * wp;
}
