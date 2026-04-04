#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightPos;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    float diff = max(dot(N, L), 0.0);
    float rim = 0.08 + 0.92 * diff;
    vec3 c = vColor * rim;
    FragColor = vec4(c, 1.0);
}
