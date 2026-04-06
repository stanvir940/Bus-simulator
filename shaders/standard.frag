#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uCameraPos;
uniform vec3 uGlobalAmbient;
uniform vec3 uDirDirection;
uniform vec3 uDirAmbient;
uniform vec3 uDirDiffuse;
uniform vec3 uDirSpecular;
uniform int uNumPointLights;
uniform vec3 uPointPos[8];
uniform vec3 uPointAmbient[8];
uniform vec3 uPointDiffuse[8];
uniform vec3 uPointSpecular[8];
uniform vec3 uPointAtten[8];

uniform int uNumSpotLights;
uniform vec3 uSpotPos[24];
uniform vec3 uSpotDir[24];
uniform float uSpotCosInner[24];
uniform float uSpotCosOuter[24];
uniform vec3 uSpotDiffuse[24];
uniform vec3 uSpotSpecular[24];
uniform vec3 uSpotAtten[24];

uniform vec3 uBaseColor;
uniform float uUseTexture;
uniform float uShininess;
uniform float uSpecularStrength;
uniform float uSpecularGlobalScale;
uniform sampler2D uTex;

uniform float uFogDensity;
uniform vec3 uFogColor;

out vec4 FragColor;

vec3 phongPoint(int i, vec3 N, vec3 V, vec3 base) {
    vec3 L = uPointPos[i] - vWorldPos;
    float dist = length(L);
    L = normalize(L);
    float diff = max(dot(N, L), 0.0);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), uShininess);
    float att = 1.0 / (uPointAtten[i].x + uPointAtten[i].y * dist + uPointAtten[i].z * dist * dist);
    vec3 amb = uPointAmbient[i] * base;
    vec3 dif = uPointDiffuse[i] * diff * base;
    vec3 spc = uPointSpecular[i] * spec * uSpecularStrength * uSpecularGlobalScale;
    return (amb + dif + spc) * att;
}

vec3 phongSpot(int i, vec3 N, vec3 V, vec3 base) {
    vec3 toFrag = vWorldPos - uSpotPos[i];
    float dist = length(toFrag);
    vec3 L = normalize(uSpotPos[i] - vWorldPos);
    vec3 F = normalize(toFrag);
    float cd = dot(F, uSpotDir[i]);
    if (cd < uSpotCosOuter[i]) {
        return vec3(0.0);
    }
    float spotInt = smoothstep(uSpotCosOuter[i], uSpotCosInner[i], cd);
    float diff = max(dot(N, L), 0.0);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), uShininess);
    float att = 1.0 / (uSpotAtten[i].x + uSpotAtten[i].y * dist + uSpotAtten[i].z * dist * dist);
    vec3 dif = uSpotDiffuse[i] * diff * base * spotInt;
    vec3 spc = uSpotSpecular[i] * spec * uSpecularStrength * uSpecularGlobalScale * spotInt;
    return (dif + spc) * att;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    vec3 albedo = uBaseColor;
    if (uUseTexture > 0.5) {
        albedo *= texture(uTex, vTexCoord).rgb;
    }

    vec3 color = uGlobalAmbient * albedo;

    vec3 Ld = normalize(uDirDirection);
    float dDiff = max(dot(N, Ld), 0.0);
    vec3 Rd = reflect(-Ld, N);
    float dSpec = pow(max(dot(V, Rd), 0.0), uShininess);
    color += uDirAmbient * albedo;
    color += uDirDiffuse * dDiff * albedo;
    color += uDirSpecular * dSpec * uSpecularStrength * uSpecularGlobalScale;

    for (int i = 0; i < uNumPointLights && i < 8; ++i) {
        color += phongPoint(i, N, V, albedo);
    }
    for (int i = 0; i < uNumSpotLights && i < 24; ++i) {
        color += phongSpot(i, N, V, albedo);
    }

    float dist = length(uCameraPos - vWorldPos);
    float fogF = 1.0 - exp(-uFogDensity * dist);
    fogF = clamp(fogF, 0.0, 1.0);
    color = mix(color, uFogColor, fogF);

    FragColor = vec4(color, 1.0);
}
