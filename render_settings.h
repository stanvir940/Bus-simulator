#pragma once

enum class WorldMode {
    Outdoor,
    TicketOffice,
};

// Mutable from main (keyboard); read each frame by Renderer.
struct AppRenderSettings {
    float fogDensity = 0.018f;
    float fogColor[3] = {0.58f, 0.76f, 0.92f};
    float globalAmbientScale = 1.0f;
    float sunDiffuseScale = 1.0f;
    float sunAmbientScale = 1.0f;
    /// Horizontal angle (degrees): 0 = +Z, increases toward +X (matches former fixed sun vector).
    float sunAzimuthDeg = 76.0f;
    /// Angle above horizon (degrees).
    float sunElevationDeg = 52.5f;
    /// Multiplies directional + point/spot specular contribution after lighting is set up.
    float specularScale = 1.0f;
    bool pointLightsEnabled = true;
    bool spotLightsEnabled = true;
    bool nightMode = false;
    bool sunEnabled = true;
    bool rainEnabled = false;
};
