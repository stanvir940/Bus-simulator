#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in float vWave;
in float vEdgeFade;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform float uTime;
uniform float uFogDensity;
uniform vec3 uFogColor;
uniform sampler2D uWaterTex;
uniform float uUseWaterTex;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    float NdotL = max(dot(N, L), 0.0);

    vec2 uv = vWorldPos.xz * 0.08 + uTime * 0.02;
    vec3 texDet = vec3(1.0);
    if (uUseWaterTex > 0.5) {
        texDet = texture(uWaterTex, uv).rgb;
    }

    vec3 deep = vec3(0.02, 0.07, 0.12);
    vec3 shallow = vec3(0.06, 0.22, 0.34);
    vec3 caust = vec3(0.12, 0.38, 0.48) * (0.5 + 0.5 * sin(vWave * 22.0 + uTime));
    vec3 waterCol = mix(deep, shallow, 0.35 + 0.4 * NdotL) + caust * 0.08;
    waterCol *= mix(vec3(1.0), texDet, 0.35);

    float fres = pow(1.0 - max(dot(N, V), 0.0), 2.5);
    vec3 spec = vec3(0.85) * pow(max(dot(reflect(-L, N), V), 0.0), 64.0);
    vec3 color = waterCol * (0.2 + 0.8 * NdotL) + spec * 0.45;
    color = mix(color, vec3(0.5, 0.72, 0.92), fres * 0.35);

    float dist = length(uCameraPos - vWorldPos);
    float fogF = 1.0 - exp(-uFogDensity * dist * 0.85);
    fogF = clamp(fogF, 0.0, 1.0);
    color = mix(color, uFogColor, fogF);

    float a = clamp(vEdgeFade, 0.0, 1.0) * 0.92;
    FragColor = vec4(color, a);
}
