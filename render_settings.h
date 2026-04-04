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
    bool pointLightsEnabled = true;
    bool spotLightsEnabled = true;
    bool nightMode = false;
};
