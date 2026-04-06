#include "renderer.h"

#include "bus.h"
#include "camera.h"
#include "lighting.h"
#include "math_gl.h"
#include "render_settings.h"
#include "scene.h"
#include "scene_constants.h"
#include "texture_util.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <vector>

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr int kStride = 9;  // pos3 normal3 color3
constexpr int kStrideTp = 6;
constexpr int kStrideRoad = 8;
constexpr int kStrideIn = 8;

void pushVert(std::vector<float>& v, float px, float py, float pz, float nx, float ny, float nz, float r, float g,
    float b) {
    v.push_back(px);
    v.push_back(py);
    v.push_back(pz);
    v.push_back(nx);
    v.push_back(ny);
    v.push_back(nz);
    v.push_back(r);
    v.push_back(g);
    v.push_back(b);
}

void pushTri(std::vector<float>& v, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2,
    float z2, float nx, float ny, float nz, float r, float g, float b) {
    pushVert(v, x0, y0, z0, nx, ny, nz, r, g, b);
    pushVert(v, x1, y1, z1, nx, ny, nz, r, g, b);
    pushVert(v, x2, y2, z2, nx, ny, nz, r, g, b);
}

void addCuboid(std::vector<float>& v, float cx, float cy, float cz, float w, float h, float d, float r, float g,
    float b) {
    const float hx = w * 0.5f;
    const float hy = h * 0.5f;
    const float hz = d * 0.5f;
    const float x0 = cx - hx;
    const float x1 = cx + hx;
    const float y0 = cy - hy;
    const float y1 = cy + hy;
    const float z0 = cz - hz;
    const float z1 = cz + hz;

    pushTri(v, x0, y0, z1, x1, y0, z1, x1, y1, z1, 0, 0, 1, r, g, b);
    pushTri(v, x0, y0, z1, x1, y1, z1, x0, y1, z1, 0, 0, 1, r, g, b);

    pushTri(v, x1, y0, z0, x0, y0, z0, x0, y1, z0, 0, 0, -1, r, g, b);
    pushTri(v, x1, y0, z0, x0, y1, z0, x1, y1, z0, 0, 0, -1, r, g, b);

    pushTri(v, x1, y0, z1, x1, y0, z0, x1, y1, z0, 1, 0, 0, r, g, b);
    pushTri(v, x1, y0, z1, x1, y1, z0, x1, y1, z1, 1, 0, 0, r, g, b);

    pushTri(v, x0, y0, z0, x0, y0, z1, x0, y1, z1, -1, 0, 0, r, g, b);
    pushTri(v, x0, y0, z0, x0, y1, z1, x0, y1, z0, -1, 0, 0, r, g, b);

    pushTri(v, x0, y1, z0, x1, y1, z0, x1, y1, z1, 0, 1, 0, r, g, b);
    pushTri(v, x0, y1, z0, x1, y1, z1, x0, y1, z1, 0, 1, 0, r, g, b);

    pushTri(v, x0, y0, z1, x0, y0, z0, x1, y0, z0, 0, -1, 0, r, g, b);
    pushTri(v, x0, y0, z1, x1, y0, z0, x1, y0, z1, 0, -1, 0, r, g, b);
}

void addCylinderY(std::vector<float>& v, float cx, float y0, float cz, float radius, float height, int slices, float r,
    float g, float b) {
    const int n = std::max(8, slices);
    const float y1 = y0 + height;
    for (int i = 0; i < n; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(n) * (2.0f * PI);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(n) * (2.0f * PI);
        const float x0 = cx + std::cos(t0) * radius;
        const float z0 = cz + std::sin(t0) * radius;
        const float x1 = cx + std::cos(t1) * radius;
        const float z1 = cz + std::sin(t1) * radius;
        const float nx0 = std::cos(t0);
        const float nz0 = std::sin(t0);
        const float nx1 = std::cos(t1);
        const float nz1 = std::sin(t1);
        pushTri(v, x0, y0, z0, x1, y0, z1, x1, y1, z1, nx1, 0, nz1, r, g, b);
        pushTri(v, x0, y0, z0, x1, y1, z1, x0, y1, z0, nx0, 0, nz0, r, g, b);
    }
}

void addSphere(std::vector<float>& v, float cx, float cy, float cz, float radius, int slices, int stacks, float r,
    float g, float b) {
    const int lat = std::max(2, stacks);
    const int lon = std::max(3, slices);
    for (int i = 0; i < lat; ++i) {
        const float v0 = static_cast<float>(i) / static_cast<float>(lat);
        const float v1 = static_cast<float>(i + 1) / static_cast<float>(lat);
        const float phi0 = v0 * PI;
        const float phi1 = v1 * PI;
        for (int j = 0; j < lon; ++j) {
            const float u0 = static_cast<float>(j) / static_cast<float>(lon);
            const float u1 = static_cast<float>(j + 1) / static_cast<float>(lon);
            const float th0 = u0 * 2.0f * PI;
            const float th1 = u1 * 2.0f * PI;

            const float x00 = cx + radius * std::sin(phi0) * std::cos(th0);
            const float y00 = cy + radius * std::cos(phi0);
            const float z00 = cz + radius * std::sin(phi0) * std::sin(th0);
            const float x10 = cx + radius * std::sin(phi0) * std::cos(th1);
            const float y10 = cy + radius * std::cos(phi0);
            const float z10 = cz + radius * std::sin(phi0) * std::sin(th1);
            const float x01 = cx + radius * std::sin(phi1) * std::cos(th0);
            const float y01 = cy + radius * std::cos(phi1);
            const float z01 = cz + radius * std::sin(phi1) * std::sin(th0);
            const float x11 = cx + radius * std::sin(phi1) * std::cos(th1);
            const float y11 = cy + radius * std::cos(phi1);
            const float z11 = cz + radius * std::sin(phi1) * std::sin(th1);

            Vec3 a = {x00 - cx, y00 - cy, z00 - cz};
            Vec3 p1 = {x10 - cx, y10 - cy, z10 - cz};
            Vec3 p2 = {x11 - cx, y11 - cy, z11 - cz};
            Vec3 u = {p1.x - a.x, p1.y - a.y, p1.z - a.z};
            Vec3 w = {p2.x - a.x, p2.y - a.y, p2.z - a.z};
            Vec3 n = {u.y * w.z - u.z * w.y, u.z * w.x - u.x * w.z, u.x * w.y - u.y * w.x};
            float nl = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
            if (nl > 1e-5f) {
                n.x /= nl;
                n.y /= nl;
                n.z /= nl;
            } else {
                n = {0, 1, 0};
            }
            pushTri(v, x00, y00, z00, x10, y10, z10, x11, y11, z11, n.x, n.y, n.z, r, g, b);

            Vec3 a2 = {x00 - cx, y00 - cy, z00 - cz};
            Vec3 b2 = {x11 - cx, y11 - cy, z11 - cz};
            Vec3 c2 = {x01 - cx, y01 - cy, z01 - cz};
            u = {b2.x - a2.x, b2.y - a2.y, b2.z - a2.z};
            w = {c2.x - a2.x, c2.y - a2.y, c2.z - a2.z};
            n = {u.y * w.z - u.z * w.y, u.z * w.x - u.x * w.z, u.x * w.y - u.y * w.x};
            nl = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
            if (nl > 1e-5f) {
                n.x /= nl;
                n.y /= nl;
                n.z /= nl;
            } else {
                n = {0, 1, 0};
            }
            pushTri(v, x00, y00, z00, x11, y11, z11, x01, y01, z01, n.x, n.y, n.z, r, g, b);
        }
    }
}

void addConeY(std::vector<float>& v, float cx, float y0, float cz, float radius, float height, int slices, float r,
    float g, float b) {
    const int n = std::max(8, slices);
    const float y1 = y0 + height;
    for (int i = 0; i < n; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(n) * (2.0f * PI);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(n) * (2.0f * PI);
        const float x0 = cx + std::cos(t0) * radius;
        const float z0 = cz + std::sin(t0) * radius;
        const float x1 = cx + std::cos(t1) * radius;
        const float z1 = cz + std::sin(t1) * radius;
        Vec3 e0 = {x0 - cx, 0, z0 - cz};
        Vec3 e1 = {x1 - cx, 0, z1 - cz};
        Vec3 side = {e0.x + e1.x, height, e0.z + e1.z};
        float sl = std::sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
        if (sl > 1e-5f) {
            side.x /= sl;
            side.y /= sl;
            side.z /= sl;
        }
        pushTri(v, x0, y0, z0, x1, y0, z1, cx, y1, cz, side.x, side.y, side.z, r, g, b);
    }
}

void roadAsphaltTri(std::vector<float>& v, float x0, float z0, float x1, float z1, float x2, float z2, float y,
    float xMin, float zMin, float xMax, float zMax) {
    auto sample = [&](float x, float z) {
        const float edge = std::min(std::min(x - xMin, xMax - x), std::min(z - zMin, zMax - z));
        const float shoulder = std::min(edge / 2.6f, 1.0f);
        const float wear = 0.9f + 0.08f * std::sin(x * 1.05f + z * 0.88f) + 0.06f * std::sin(x * 4.4f - z * 3.9f);
        const float tire = 0.86f + 0.14f * std::sin(z * 0.19f + 0.3f);
        const float dim = (0.62f + 0.38f * shoulder) * wear * tire;
        const float r = 0.09f * dim;
        const float g = 0.09f * dim;
        const float b = 0.10f * dim;
        return std::tuple<float, float, float>(r, g, b);
    };
    auto c0 = sample(x0, z0);
    auto c1 = sample(x1, z1);
    auto c2 = sample(x2, z2);
    const float r = (std::get<0>(c0) + std::get<0>(c1) + std::get<0>(c2)) / 3.0f;
    const float g = (std::get<1>(c0) + std::get<1>(c1) + std::get<1>(c2)) / 3.0f;
    const float b = (std::get<2>(c0) + std::get<2>(c1) + std::get<2>(c2)) / 3.0f;
    pushTri(v, x0, y, z0, x1, y, z1, x2, y, z2, 0, 1, 0, r, g, b);
}

void addRoadGridRect(std::vector<float>& v, float x0, float x1, float z0, float z1, float y, int splits) {
    const int n = std::max(2, splits);
    for (int j = 0; j < n; ++j) {
        const float za = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zb = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        for (int i = 0; i < n; ++i) {
            const float xa = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            const float xb = x0 + (x1 - x0) * static_cast<float>(i + 1) / static_cast<float>(n);
            roadAsphaltTri(v, xa, za, xb, za, xb, zb, y, x0, z0, x1, z1);
            roadAsphaltTri(v, xa, za, xb, zb, xa, zb, y, x0, z0, x1, z1);
        }
    }
}

Vec3 crossVec(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 normalizeVec(const Vec3& v) {
    const float L = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (L < 1e-5f) {
        return {0.0f, 1.0f, 0.0f};
    }
    return {v.x / L, v.y / L, v.z / L};
}

Vec3 triNormal(
    float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2) {
    Vec3 u = {x1 - x0, y1 - y0, z1 - z0};
    Vec3 w = {x2 - x0, y2 - y0, z2 - z0};
    return normalizeVec(crossVec(u, w));
}

std::tuple<float, float, float> roadAsphaltRgbAt(float x, float z, float xMin, float zMin, float xMax, float zMax) {
    const float edge = std::min(std::min(x - xMin, xMax - x), std::min(z - zMin, zMax - z));
    const float shoulder = std::min(edge / 2.6f, 1.0f);
    const float wear = 0.9f + 0.08f * std::sin(x * 1.05f + z * 0.88f) + 0.06f * std::sin(x * 4.4f - z * 3.9f);
    const float tire = 0.86f + 0.14f * std::sin(z * 0.19f + 0.3f);
    const float dim = (0.62f + 0.38f * shoulder) * wear * tire;
    return {0.09f * dim, 0.09f * dim, 0.10f * dim};
}

void addTerrainRoadPatch(std::vector<float>& v, const Scene& scene, float x0, float x1, float z0, float z1, int splits) {
    if (x1 <= x0 || z1 <= z0) {
        return;
    }
    const int n = std::max(3, splits);
    const float yOff = 0.055f;
    for (int j = 0; j < n; ++j) {
        const float zA = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zB = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        for (int i = 0; i < n; ++i) {
            const float xA = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            const float xB = x0 + (x1 - x0) * static_cast<float>(i + 1) / static_cast<float>(n);
            const float h00 = scene.terrainHeight(xA, zA) + yOff;
            const float h10 = scene.terrainHeight(xB, zA) + yOff;
            const float h11 = scene.terrainHeight(xB, zB) + yOff;
            const float h01 = scene.terrainHeight(xA, zB) + yOff;

            Vec3 n1 = triNormal(xA, h00, zA, xB, h10, zA, xB, h11, zB);
            auto c1a = roadAsphaltRgbAt(xA, zA, x0, z0, x1, z1);
            auto c1b = roadAsphaltRgbAt(xB, zA, x0, z0, x1, z1);
            auto c1c = roadAsphaltRgbAt(xB, zB, x0, z0, x1, z1);
            const float r1 = (std::get<0>(c1a) + std::get<0>(c1b) + std::get<0>(c1c)) / 3.0f;
            const float g1 = (std::get<1>(c1a) + std::get<1>(c1b) + std::get<1>(c1c)) / 3.0f;
            const float b1 = (std::get<2>(c1a) + std::get<2>(c1b) + std::get<2>(c1c)) / 3.0f;
            pushTri(v, xA, h00, zA, xB, h10, zA, xB, h11, zB, n1.x, n1.y, n1.z, r1, g1, b1);

            Vec3 n2 = triNormal(xA, h00, zA, xB, h11, zB, xA, h01, zB);
            auto c2a = roadAsphaltRgbAt(xA, zA, x0, z0, x1, z1);
            auto c2b = roadAsphaltRgbAt(xB, zB, x0, z0, x1, z1);
            auto c2c = roadAsphaltRgbAt(xA, zB, x0, z0, x1, z1);
            const float r2 = (std::get<0>(c2a) + std::get<0>(c2b) + std::get<0>(c2c)) / 3.0f;
            const float g2 = (std::get<1>(c2a) + std::get<1>(c2b) + std::get<1>(c2c)) / 3.0f;
            const float b2 = (std::get<2>(c2a) + std::get<2>(c2b) + std::get<2>(c2c)) / 3.0f;
            pushTri(v, xA, h00, zA, xB, h11, zB, xA, h01, zB, n2.x, n2.y, n2.z, r2, g2, b2);
        }
    }
}

void buildingColor(int seed, float& r, float& g, float& b) {
    const int variant = std::abs(seed) % 5;
    if (variant == 0) {
        r = 0.42f;
        g = 0.45f;
        b = 0.52f;
    } else if (variant == 1) {
        r = 0.52f;
        g = 0.40f;
        b = 0.38f;
    } else if (variant == 2) {
        r = 0.36f;
        g = 0.42f;
        b = 0.50f;
    } else if (variant == 3) {
        r = 0.48f;
        g = 0.50f;
        b = 0.46f;
    } else {
        r = 0.40f;
        g = 0.38f;
        b = 0.47f;
    }
}

int hash2(int a, int b) {
    return (a * 73856093) ^ (b * 19349663);
}

void uploadColoredMesh(GLuint& vao, GLuint& vbo, const std::vector<float>& data, GLsizei* outCount) {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
    const GLsizei stride = static_cast<GLsizei>(kStride * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
    glBindVertexArray(0);
    *outCount = static_cast<GLsizei>(data.size() / kStride);
}

void pushVertTp(std::vector<float>& v, float px, float py, float pz, float nx, float ny, float nz) {
    v.push_back(px);
    v.push_back(py);
    v.push_back(pz);
    v.push_back(nx);
    v.push_back(ny);
    v.push_back(nz);
}

void pushTriTp(std::vector<float>& v, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2,
    float z2, float nx, float ny, float nz) {
    pushVertTp(v, x0, y0, z0, nx, ny, nz);
    pushVertTp(v, x1, y1, z1, nx, ny, nz);
    pushVertTp(v, x2, y2, z2, nx, ny, nz);
}

void addCuboidTp(std::vector<float>& v, float cx, float cy, float cz, float w, float h, float d) {
    const float hx = w * 0.5f;
    const float hy = h * 0.5f;
    const float hz = d * 0.5f;
    const float x0 = cx - hx;
    const float x1 = cx + hx;
    const float y0 = cy - hy;
    const float y1 = cy + hy;
    const float z0 = cz - hz;
    const float z1 = cz + hz;
    pushTriTp(v, x0, y0, z1, x1, y0, z1, x1, y1, z1, 0, 0, 1);
    pushTriTp(v, x0, y0, z1, x1, y1, z1, x0, y1, z1, 0, 0, 1);
    pushTriTp(v, x1, y0, z0, x0, y0, z0, x0, y1, z0, 0, 0, -1);
    pushTriTp(v, x1, y0, z0, x0, y1, z0, x1, y1, z0, 0, 0, -1);
    pushTriTp(v, x1, y0, z1, x1, y0, z0, x1, y1, z0, 1, 0, 0);
    pushTriTp(v, x1, y0, z1, x1, y1, z0, x1, y1, z1, 1, 0, 0);
    pushTriTp(v, x0, y0, z0, x0, y0, z1, x0, y1, z1, -1, 0, 0);
    pushTriTp(v, x0, y0, z0, x0, y1, z1, x0, y1, z0, -1, 0, 0);
    pushTriTp(v, x0, y1, z0, x1, y1, z0, x1, y1, z1, 0, 1, 0);
    pushTriTp(v, x0, y1, z0, x1, y1, z1, x0, y1, z1, 0, 1, 0);
    pushTriTp(v, x0, y0, z1, x0, y0, z0, x1, y0, z0, 0, -1, 0);
    pushTriTp(v, x0, y0, z1, x1, y0, z0, x1, y0, z1, 0, -1, 0);
}

void addFrustumY(std::vector<float>& v, float cx, float y0, float cz, float rBottom, float rTop, float height,
    int slices, float rcol, float gcol, float bcol) {
    const int n = std::max(6, slices);
    const float y1 = y0 + height;
    for (int i = 0; i < n; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(n) * (2.0f * PI);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(n) * (2.0f * PI);
        const float xb0 = cx + std::cos(t0) * rBottom;
        const float zb0 = cz + std::sin(t0) * rBottom;
        const float xb1 = cx + std::cos(t1) * rBottom;
        const float zb1 = cz + std::sin(t1) * rBottom;
        const float xt0 = cx + std::cos(t0) * rTop;
        const float zt0 = cz + std::sin(t0) * rTop;
        const float xt1 = cx + std::cos(t1) * rTop;
        const float zt1 = cz + std::sin(t1) * rTop;
        Vec3 u = {xt0 - xb0, y1 - y0, zt0 - zb0};
        Vec3 w = {xb1 - xb0, 0, zb1 - zb0};
        Vec3 nrm = normalizeVec(crossVec(u, w));
        pushTri(v, xb0, y0, zb0, xb1, y0, zb1, xt1, y1, zt1, nrm.x, nrm.y, nrm.z, rcol, gcol, bcol);
        u = {xt1 - xb0, y1 - y0, zt1 - zb0};
        w = {xt0 - xb0, y1 - y0, zt0 - zb0};
        nrm = normalizeVec(crossVec(u, w));
        pushTri(v, xb0, y0, zb0, xt1, y1, zt1, xt0, y1, zt0, nrm.x, nrm.y, nrm.z, rcol, gcol, bcol);
    }
}

void addFrustumTree(std::vector<float>& v, float x, float y0, float z, int seed) {
    const float trunkH = 2.8f + static_cast<float>((seed * 17) % 5) * 0.28f;
    const float tr = 0.13f + static_cast<float>((seed * 13) % 5) * 0.03f;
    addCylinderY(v, x, y0, z, tr, trunkH, 9, 0.42f, 0.22f, 0.10f);
    float y = y0 + trunkH;
    float rb = 1.28f + static_cast<float>((seed * 7) % 7) * 0.16f;
    for (int k = 0; k < 4; ++k) {
        const float segH = 0.82f + static_cast<float>((seed + k * 3) % 5) * 0.24f;
        const float taper = 0.52f - static_cast<float>(k) * 0.09f;
        float rt = rb * std::max(0.22f, taper);
        const float gr = 0.06f + static_cast<float>(k) * 0.035f;
        const float gg = 0.38f - static_cast<float>(k) * 0.055f;
        const float gb = 0.11f + static_cast<float>(k) * 0.018f;
        addFrustumY(v, x, y, z, rb, rt, segH, 10, gr, gg, gb);
        y += segH;
        rb = rt;
    }
}

void pushRoadVert(std::vector<float>& v, float x, float y, float z, float nx, float ny, float nz, float u, float tv) {
    v.push_back(x);
    v.push_back(y);
    v.push_back(z);
    v.push_back(nx);
    v.push_back(ny);
    v.push_back(nz);
    v.push_back(u);
    v.push_back(tv);
}

void pushRoadTri(std::vector<float>& v, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2,
    float z2, float nx, float ny, float nz) {
    pushRoadVert(v, x0, y0, z0, nx, ny, nz, x0 * 0.14f, z0 * 0.14f);
    pushRoadVert(v, x1, y1, z1, nx, ny, nz, x1 * 0.14f, z1 * 0.14f);
    pushRoadVert(v, x2, y2, z2, nx, ny, nz, x2 * 0.14f, z2 * 0.14f);
}

void addTerrainRoadPatchRoad(std::vector<float>& v, const Scene& scene, float x0, float x1, float z0, float z1, int splits) {
    if (x1 <= x0 || z1 <= z0) {
        return;
    }
    const int n = std::max(3, splits);
    const float yOff = 0.055f;
    for (int j = 0; j < n; ++j) {
        const float zA = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zB = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        for (int i = 0; i < n; ++i) {
            const float xA = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            const float xB = x0 + (x1 - x0) * static_cast<float>(i + 1) / static_cast<float>(n);
            const float h00 = scene.terrainHeight(xA, zA) + yOff;
            const float h10 = scene.terrainHeight(xB, zA) + yOff;
            const float h11 = scene.terrainHeight(xB, zB) + yOff;
            const float h01 = scene.terrainHeight(xA, zB) + yOff;
            Vec3 n1 = triNormal(xA, h00, zA, xB, h10, zA, xB, h11, zB);
            pushRoadTri(v, xA, h00, zA, xB, h10, zA, xB, h11, zB, n1.x, n1.y, n1.z);
            Vec3 n2 = triNormal(xA, h00, zA, xB, h11, zB, xA, h01, zB);
            pushRoadTri(v, xA, h00, zA, xB, h11, zB, xA, h01, zB, n2.x, n2.y, n2.z);
        }
    }
}

void addRoadGridRectRoad(std::vector<float>& v, float x0, float x1, float z0, float z1, float y, int splits) {
    const int n = std::max(2, splits);
    for (int j = 0; j < n; ++j) {
        const float za = z0 + (z1 - z0) * static_cast<float>(j) / static_cast<float>(n);
        const float zb = z0 + (z1 - z0) * static_cast<float>(j + 1) / static_cast<float>(n);
        for (int i = 0; i < n; ++i) {
            const float xa = x0 + (x1 - x0) * static_cast<float>(i) / static_cast<float>(n);
            const float xb = x0 + (x1 - x0) * static_cast<float>(i + 1) / static_cast<float>(n);
            pushRoadTri(v, xa, y, za, xb, y, za, xb, y, zb, 0, 1, 0);
            pushRoadTri(v, xa, y, za, xb, y, zb, xa, y, zb, 0, 1, 0);
        }
    }
}

void uploadTriplanarMesh(GLuint& vao, GLuint& vbo, const std::vector<float>& data, GLsizei* outCount) {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
    const GLsizei stride = static_cast<GLsizei>(kStrideTp * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    *outCount = static_cast<GLsizei>(data.size() / kStrideTp);
}

void uploadRoadMesh(GLuint& vao, GLuint& vbo, const std::vector<float>& data, GLsizei* outCount) {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
    const GLsizei stride = static_cast<GLsizei>(kStrideRoad * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
    glBindVertexArray(0);
    *outCount = static_cast<GLsizei>(data.size() / kStrideRoad);
}

bool tryLoadShaderPair(ShaderProgram& prog, const std::string& dir, const char* baseName) {
    const std::string v = dir + "/" + baseName + ".vert";
    const std::string f = dir + "/" + baseName + ".frag";
    return prog.loadFromFiles(v, f);
}

}  // namespace

static GLuint loadTextureStem(const char* stem) {
    char buf[12][280];
    const char* paths[12];
    int pn = 0;
    const char* prefixes[] = {"textures/", "../textures/", "../../BusStandSimulator/textures/"};
    const char* exts[] = {".jpg", ".png"};
    for (const char* pre : prefixes) {
        for (const char* ext : exts) {
            if (pn >= 12) {
                break;
            }
            std::snprintf(buf[pn], sizeof(buf[pn]), "%s%s%s", pre, stem, ext);
            paths[pn] = buf[pn];
            ++pn;
        }
    }
    return loadTextureFromSearchPaths(paths, pn);
}

static GLuint makeWhiteTexture() {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    const unsigned char px[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return t;
}

static constexpr float kMathPi = 3.14159265358979323846f;

Renderer::~Renderer() {
    destroyGpu();
}

static void bindPhongFogSpots(GLuint prog, const float* camPos, const float* ga, const DirectionalLight& sun, int nPt,
    const PointLight* pts, int nSpot, const Spotlight* spots, const AppRenderSettings& settings) {
    applyLightingUniforms(prog, camPos, ga, sun, nPt, pts, settings.specularScale);
    applyFogUniforms(prog, settings.fogDensity, settings.fogColor);
    applySpotlights(prog, settings.spotLightsEnabled ? nSpot : 0, spots);
}

static DirectionalLight makeSun(const AppRenderSettings& s) {
    const float night = s.nightMode ? 0.18f : 1.0f;
    DirectionalLight sun{};
    const float az = s.sunAzimuthDeg * (kMathPi / 180.0f);
    const float el = s.sunElevationDeg * (kMathPi / 180.0f);
    const float ce = std::cos(el);
    sun.dir[0] = ce * std::sin(az);
    sun.dir[1] = std::sin(el);
    sun.dir[2] = ce * std::cos(az);
    sun.ambient[0] = 0.22f * s.sunAmbientScale * night;
    sun.ambient[1] = 0.22f * s.sunAmbientScale * night;
    sun.ambient[2] = (s.nightMode ? 0.32f : 0.25f) * s.sunAmbientScale * night;
    sun.diffuse[0] = 0.95f * s.sunDiffuseScale * night;
    sun.diffuse[1] = 0.93f * s.sunDiffuseScale * night;
    sun.diffuse[2] = (s.nightMode ? 0.55f : 0.85f) * s.sunDiffuseScale * night;
    sun.specular[0] = 0.75f * s.sunDiffuseScale * night;
    sun.specular[1] = 0.72f * s.sunDiffuseScale * night;
    sun.specular[2] = 0.65f * s.sunDiffuseScale * night;
    return sun;
}

void Renderer::destroyGpu() {
    auto delV = [](GLuint& vao, GLuint& vbo) {
        if (vbo) {
            glDeleteBuffers(1, &vbo);
        }
        if (vao) {
            glDeleteVertexArrays(1, &vao);
        }
        vao = vbo = 0;
    };
    delV(m_terrainVao, m_terrainVbo);
    delV(m_staticVao, m_staticVbo);
    delV(m_roadVao, m_roadVbo);
    delV(m_tpStationVao, m_tpStationVbo);
    delV(m_tpCityVao, m_tpCityVbo);
    delV(m_tpTowerVao, m_tpTowerVbo);
    delV(m_waterVao, m_waterVbo);
    delV(m_interiorVao, m_interiorVbo);
    m_terrainVertexCount = m_staticVertexCount = m_roadVertexCount = m_tpStationVertexCount = m_tpCityVertexCount =
        m_tpTowerVertexCount = m_waterVertexCount = m_interiorVertexCount = 0;
    const GLuint texs[] = {m_grassTex, m_roadTex, m_buildingTex, m_buildingCityTex, m_buildingTowerTex, m_pathTex,
        m_groundDetailTex, m_flagTex, m_lampTex, m_chairTex, m_npcTex, m_waterTex, m_whiteTex};
    for (GLuint t : texs) {
        if (t) {
            glDeleteTextures(1, &t);
        }
    }
    m_grassTex = m_roadTex = m_buildingTex = m_buildingCityTex = m_buildingTowerTex = m_pathTex = m_groundDetailTex =
        m_flagTex = m_lampTex = m_chairTex = m_npcTex = m_waterTex = m_whiteTex = 0;
    m_hasGrassTex = m_hasRoadTex = m_hasBuildingTex = m_hasBuildingCityTex = m_hasBuildingTowerTex = m_hasPathTex =
        m_hasGroundDetailTex = m_hasFlagTex = m_hasLampTex = m_hasChairTex = m_hasNpcTex = m_hasWaterTex = false;
}

void Renderer::loadTextures() {
    m_whiteTex = makeWhiteTexture();
    m_grassTex = loadTextureStem("terrain_grass");
    if (m_grassTex == 0) {
        m_grassTex = loadTextureStem("grass");
    }
    if (m_grassTex == 0) {
        m_grassTex = loadTextureStem("normal-ground");
    }
    m_hasGrassTex = (m_grassTex != 0);
    m_roadTex = loadTextureStem("road_asphalt");
    if (m_roadTex == 0) {
        m_roadTex = loadTextureStem("road");
    }
    if (m_roadTex == 0) {
        m_roadTex = loadTextureStem("concrete");
    }
    m_hasRoadTex = (m_roadTex != 0);
    m_buildingTex = loadTextureStem("building");
    if (m_buildingTex == 0) {
        m_buildingTex = loadTextureStem("wall_brick");
    }
    m_hasBuildingTex = (m_buildingTex != 0);
    m_buildingCityTex = loadTextureStem("old-building");
    if (m_buildingCityTex == 0) {
        m_buildingCityTex = m_buildingTex;
    }
    m_hasBuildingCityTex = (m_buildingCityTex != 0);
    m_buildingTowerTex = loadTextureStem("HighRiseResidential0139_1_350");
    if (m_buildingTowerTex == 0) {
        m_buildingTowerTex = loadTextureStem("highrise");
    }
    if (m_buildingTowerTex == 0) {
        m_buildingTowerTex = m_buildingCityTex;
    }
    m_hasBuildingTowerTex = (m_buildingTowerTex != 0);
    m_pathTex = loadTextureStem("concrete");
    m_hasPathTex = (m_pathTex != 0);
    m_groundDetailTex = loadTextureStem("flowers");
    if (m_groundDetailTex == 0) {
        m_groundDetailTex = loadTextureStem("normal-ground");
    }
    m_hasGroundDetailTex = (m_groundDetailTex != 0);
    m_flagTex = loadTextureStem("flowers");
    m_hasFlagTex = (m_flagTex != 0);
    m_lampTex = loadTextureStem("lamp");
    m_hasLampTex = (m_lampTex != 0);
    m_chairTex = loadTextureStem("chair");
    m_hasChairTex = (m_chairTex != 0);
    m_npcTex = loadTextureStem("npc");
    m_hasNpcTex = (m_npcTex != 0);
    m_waterTex = loadTextureStem("water");
    m_hasWaterTex = (m_waterTex != 0);
}

int Renderer::fillSpotlights(const Scene& scene, Spotlight* out) {
    int n = 0;
    const float ci = std::cos(10.0f * kMathPi / 180.0f);
    const float co = std::cos(34.0f * kMathPi / 180.0f);
    auto add = [&](float px, float py, float pz) {
        if (n >= kMaxSpotlights) {
            return;
        }
        Spotlight& s = out[n++];
        s.position[0] = px;
        s.position[1] = py;
        s.position[2] = pz;
        s.direction[0] = 0.0f;
        s.direction[1] = -1.0f;
        s.direction[2] = 0.0f;
        s.cosInner = ci;
        s.cosOuter = co;
        s.diffuse[0] = 0.95f;
        s.diffuse[1] = 0.88f;
        s.diffuse[2] = 0.62f;
        s.specular[0] = 0.55f;
        s.specular[1] = 0.5f;
        s.specular[2] = 0.38f;
        s.constant = 1.0f;
        s.linear = 0.028f;
        s.quadratic = 0.085f;
    };
    for (int i = -4; i <= 4; ++i) {
        const float z = static_cast<float>(i) * 38.0f;
        for (float sx : {-118.0f, 118.0f}) {
            const float th = scene.terrainHeight(sx, z);
            add(sx, th + 8.25f, z);
        }
    }
    const float pp[] = {-34.0f, -12.0f, 12.0f, 34.0f};
    for (float x : pp) {
        add(x, 7.25f, 24.5f);
    }
    return n;
}

void Renderer::buildTerrain(const Scene& scene) {
    std::vector<float> mesh;
    const float half = kTerrainHalfExtent;
    const float minX = -half;
    const float maxX = half;
    const float minZ = -half;
    const float maxZ = half;
    const float step = 3.2f;
    const float grassSpecTintR = 0.06f;
    const float grassSpecTintG = 0.09f;
    const float grassSpecTintB = 0.05f;

    for (float x = minX; x < maxX; x += step) {
        for (float z = minZ; z < maxZ; z += step) {
            const float x0 = x;
            const float x1 = x + step;
            const float z0 = z;
            const float z1 = z + step;
            const float h00 = scene.terrainHeight(x0, z0);
            const float h10 = scene.terrainHeight(x1, z0);
            const float h11 = scene.terrainHeight(x1, z1);
            const float h01 = scene.terrainHeight(x0, z1);
            Vec3 n0 = scene.terrainNormal(x0, z0);
            Vec3 n1 = scene.terrainNormal(x1, z0);
            Vec3 n2 = scene.terrainNormal(x1, z1);
            Vec3 n3 = scene.terrainNormal(x0, z1);
            float r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3;
            scene.terrainGrassColor(x0, z0, h00, n0, &r0, &g0, &b0);
            scene.terrainGrassColor(x1, z0, h10, n1, &r1, &g1, &b1);
            scene.terrainGrassColor(x1, z1, h11, n2, &r2, &g2, &b2);
            scene.terrainGrassColor(x0, z1, h01, n3, &r3, &g3, &b3);
            r0 += grassSpecTintR * 0.08f;
            g0 += grassSpecTintG * 0.08f;
            b0 += grassSpecTintB * 0.08f;
            r1 += grassSpecTintR * 0.08f;
            g1 += grassSpecTintG * 0.08f;
            b1 += grassSpecTintB * 0.08f;
            Vec3 tn = triNormal(x0, h00, z0, x1, h10, z0, x1, h11, z1);
            pushTri(mesh, x0, h00, z0, x1, h10, z0, x1, h11, z1, tn.x, tn.y, tn.z, (r0 + r1 + r2) / 3.0f,
                (g0 + g1 + g2) / 3.0f, (b0 + b1 + b2) / 3.0f);
            tn = triNormal(x0, h00, z0, x1, h11, z1, x0, h01, z1);
            pushTri(mesh, x0, h00, z0, x1, h11, z1, x0, h01, z1, tn.x, tn.y, tn.z, (r0 + r2 + r3) / 3.0f,
                (g0 + g2 + g3) / 3.0f, (b0 + b2 + b3) / 3.0f);
        }
    }
    uploadColoredMesh(m_terrainVao, m_terrainVbo, mesh, &m_terrainVertexCount);
}

void Renderer::buildRoadMesh(const Scene& scene) {
    std::vector<float> mesh;
    const float roadY = 0.038f;
    const float he = kTerrainHalfExtent;
    addRoadGridRectRoad(mesh, -40.0f, 40.0f, -24.0f, 24.0f, roadY, 14);
    addRoadGridRectRoad(mesh, -95.0f, -40.0f, -8.0f, 8.0f, roadY, 12);
    addRoadGridRectRoad(mesh, 40.0f, 95.0f, -8.0f, 8.0f, roadY, 12);

    addTerrainRoadPatchRoad(mesh, scene, -he + 4.0f, he - 4.0f, -118.0f, -112.0f, 24);
    addTerrainRoadPatchRoad(mesh, scene, -he + 4.0f, he - 4.0f, 112.0f, 118.0f, 24);
    addTerrainRoadPatchRoad(mesh, scene, -118.0f, -112.0f, -he + 4.0f, he - 4.0f, 26);
    addTerrainRoadPatchRoad(mesh, scene, 112.0f, 118.0f, -he + 4.0f, he - 4.0f, 26);

    addTerrainRoadPatchRoad(mesh, scene, 55.0f, he - 8.0f, -6.0f, 6.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, -he + 8.0f, -55.0f, -6.0f, 6.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, -6.0f, 6.0f, 55.0f, he - 8.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, -6.0f, 6.0f, -he + 8.0f, -55.0f, 18);

    addTerrainRoadPatchRoad(mesh, scene, -he + 10.0f, he - 10.0f, -102.0f, -98.0f, 16);
    addTerrainRoadPatchRoad(mesh, scene, -he + 10.0f, he - 10.0f, 98.0f, 102.0f, 16);
    addTerrainRoadPatchRoad(mesh, scene, -150.0f, 150.0f, -42.0f, -38.0f, 20);
    addTerrainRoadPatchRoad(mesh, scene, -150.0f, 150.0f, 38.0f, 42.0f, 20);
    addTerrainRoadPatchRoad(mesh, scene, -42.0f, -38.0f, -160.0f, 160.0f, 22);
    addTerrainRoadPatchRoad(mesh, scene, 38.0f, 42.0f, -160.0f, 160.0f, 22);
    addTerrainRoadPatchRoad(mesh, scene, -90.0f, 90.0f, -65.0f, -60.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, -90.0f, 90.0f, 60.0f, 65.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, -65.0f, -60.0f, -90.0f, 90.0f, 18);
    addTerrainRoadPatchRoad(mesh, scene, 60.0f, 65.0f, -90.0f, 90.0f, 18);

    uploadRoadMesh(m_roadVao, m_roadVbo, mesh, &m_roadVertexCount);
}

void Renderer::buildTriplanarMesh(const Scene& scene) {
    std::vector<float> meshStation;
    std::vector<float> meshCity;
    std::vector<float> meshTower;
    for (int i = 0; i < 4; ++i) {
        const float px = -22.0f + i * 14.0f;
        addCuboidTp(meshStation, px, 0.35f, 30.0f, 10.0f, 0.7f, 8.0f);
        addCuboidTp(meshStation, px, 3.3f, 30.0f, 11.0f, 0.35f, 8.6f);
        for (int s = -1; s <= 1; s += 2) {
            addCuboidTp(meshStation, px + s * 3.2f, 0.55f, 32.5f, 0.55f, 0.65f, 0.55f);
        }
    }
    addCuboidTp(meshStation, 0.0f, 4.0f, -40.0f, 44.0f, 8.0f, 20.0f);
    addCuboidTp(meshStation, 0.0f, 8.7f, -40.0f, 46.0f, 1.0f, 21.0f);
    for (int i = 0; i < 5; ++i) {
        addCuboidTp(meshStation, -13.0f + i * 6.5f, 4.0f, -31.8f, 3.4f, 4.0f, 0.35f);
    }

    addCuboidTp(meshStation, 0.0f, 2.5f, -10.0f, 28.0f, 5.0f, 14.0f);
    addCuboidTp(meshStation, 0.0f, 5.3f, -10.0f, 29.0f, 0.8f, 15.0f);
    for (int i = 0; i < 3; ++i) {
        addCuboidTp(meshStation, -7.5f + i * 7.5f, 1.2f, -6.4f, 5.5f, 2.4f, 0.5f);
        addCuboidTp(meshStation, -7.5f + i * 7.5f, 2.5f, -5.8f, 2.4f, 1.2f, 0.2f);
    }
    addCuboidTp(meshStation, -8.8f, 1.6f, -13.2f, 0.45f, 3.2f, 7.0f);
    addCuboidTp(meshStation, 0.0f, 1.6f, -13.2f, 0.45f, 3.2f, 7.0f);
    addCuboidTp(meshStation, 8.8f, 1.6f, -13.2f, 0.45f, 3.2f, 7.0f);

    auto placeBuildingTp = [&](std::vector<float>& dest, float cx, float cz, int id) {
        if (std::fabs(cx) < kStationFlatRadius + 5.0f && std::fabs(cz) < kStationFlatRadius + 5.0f) {
            return;
        }
        const int hKey = hash2(static_cast<int>(cx * 2.0f), static_cast<int>(cz * 2.0f)) ^ id;
        const float h = 10.0f + static_cast<float>((std::abs(hKey) % 38));
        const float w = 5.4f + static_cast<float>((std::abs(hKey / 7) % 7));
        const float d = 5.4f + static_cast<float>((std::abs(hKey / 13) % 7));
        const float y0 = scene.terrainHeight(cx, cz);
        addCuboidTp(dest, cx, y0 + h * 0.5f, cz, w, h, d);
    };

    const float he = kTerrainHalfExtent;
    for (int i = -22; i <= 22; ++i) {
        const float x = static_cast<float>(i) * 9.0f;
        placeBuildingTp(meshCity, x, -(he - 14.0f), i);
        placeBuildingTp(meshCity, x, (he - 14.0f), i + 1000);
    }
    for (int j = -18; j <= 18; ++j) {
        const float z = static_cast<float>(j) * 9.0f;
        placeBuildingTp(meshCity, -(he - 14.0f), z, j + 2000);
        placeBuildingTp(meshCity, (he - 14.0f), z, j + 3000);
    }
    for (int t = 0; t < 48; ++t) {
        const int hk = hash2(t, 909);
        const float span = he - 25.0f;
        const float cx = (std::abs(hk % 280)) / 280.0f * (2.0f * span) - span;
        const float cz = (std::abs((hk / 280) % 280)) / 280.0f * (2.0f * span) - span;
        if (std::fabs(cx) < kStationFlatRadius + 12.0f && std::fabs(cz) < kStationFlatRadius + 12.0f) {
            continue;
        }
        const float y0 = scene.terrainHeight(cx, cz);
        const float h = 18.0f + static_cast<float>((std::abs(hk) % 50));
        const float foot = 8.8f + static_cast<float>((std::abs(hk / 3) % 5)) * 0.55f;
        addCuboidTp(meshTower, cx, y0 + h * 0.5f, cz, foot, h, foot);
    }

    uploadTriplanarMesh(m_tpStationVao, m_tpStationVbo, meshStation, &m_tpStationVertexCount);
    uploadTriplanarMesh(m_tpCityVao, m_tpCityVbo, meshCity, &m_tpCityVertexCount);
    uploadTriplanarMesh(m_tpTowerVao, m_tpTowerVbo, meshTower, &m_tpTowerVertexCount);
}

void Renderer::buildColoredStatic(const Scene& scene) {
    std::vector<float> mesh;
    for (int i = -4; i <= 4; ++i) {
        const float z = static_cast<float>(i) * 38.0f;
        for (float x : {-118.0f, 118.0f}) {
            const float y0 = scene.terrainHeight(x, z);
            addCylinderY(mesh, x, y0, z, 0.22f, 8.0f, 10, 0.30f, 0.30f, 0.34f);
            addSphere(mesh, x, y0 + 8.0f, z, 0.42f, 12, 8, 1.0f, 0.93f, 0.62f);
        }
    }
    const float platformPoles[] = {-34.0f, -12.0f, 12.0f, 34.0f};
    for (float x : platformPoles) {
        addCylinderY(mesh, x, 0.0f, 24.5f, 0.25f, 7.0f, 10, 0.35f, 0.35f, 0.38f);
        addSphere(mesh, x, 7.0f, 24.5f, 0.45f, 10, 6, 0.97f, 0.92f, 0.55f);
    }

    const float he = kTerrainHalfExtent;
    for (int i = 0; i < 28; ++i) {
        const float x = -he + 25.0f + static_cast<float>(i % 7) * 11.0f;
        const float z = -he + 30.0f + static_cast<float>(i / 7) * 14.0f;
        const float y = scene.terrainHeight(x, z);
        addFrustumTree(mesh, x, y, z, i * 17 + 3);
    }
    for (int i = 0; i < 18; ++i) {
        const float x = he - 35.0f - static_cast<float>(i % 3) * 9.0f;
        const float z = -he + 40.0f + static_cast<float>(i / 3) * 12.0f;
        const float y = scene.terrainHeight(x, z);
        addFrustumTree(mesh, x, y, z, i * 31 + 11);
    }
    for (int i = -14; i <= 14; ++i) {
        const float z = static_cast<float>(i) * 12.0f;
        const float xl = -he + 22.0f;
        const float xr = he - 22.0f;
        const float yl = scene.terrainHeight(xl, z);
        const float yr = scene.terrainHeight(xr, z);
        addFrustumTree(mesh, xl, yl, z, i * 19 + 5);
        addFrustumTree(mesh, xr, yr, z, i * 23 + 9);
    }

    uploadColoredMesh(m_staticVao, m_staticVbo, mesh, &m_staticVertexCount);
}

void Renderer::buildWater(const Scene& scene) {
    (void)scene;
    std::vector<float> mesh;
    const float wy = -0.22f;
    const float cx = 63.0f;
    const float cz = 62.0f;
    const float rx = 17.5f;
    const float rz = 15.5f;
    const int nx = 36;
    const int nz = 32;
    for (int j = 0; j < nz; ++j) {
        for (int i = 0; i < nx; ++i) {
            const float x0 = cx - rx + (2.0f * rx) * static_cast<float>(i) / static_cast<float>(nx);
            const float x1 = cx - rx + (2.0f * rx) * static_cast<float>(i + 1) / static_cast<float>(nx);
            const float z0 = cz - rz + (2.0f * rz) * static_cast<float>(j) / static_cast<float>(nz);
            const float z1 = cz - rz + (2.0f * rz) * static_cast<float>(j + 1) / static_cast<float>(nz);
            const float nx = 0.0f;
            const float ny = 1.0f;
            const float nz = 0.0f;
            mesh.insert(mesh.end(), {x0, wy, z0, nx, ny, nz, x1, wy, z0, nx, ny, nz, x1, wy, z1, nx, ny, nz});
            mesh.insert(mesh.end(), {x0, wy, z0, nx, ny, nz, x1, wy, z1, nx, ny, nz, x0, wy, z1, nx, ny, nz});
        }
    }
    if (m_waterVao == 0) {
        glGenVertexArrays(1, &m_waterVao);
    }
    if (m_waterVbo == 0) {
        glGenBuffers(1, &m_waterVbo);
    }
    glBindVertexArray(m_waterVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_waterVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.size() * sizeof(float)), mesh.data(), GL_STATIC_DRAW);
    constexpr int kWStride = 6;
    const GLsizei stride = static_cast<GLsizei>(kWStride * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    m_waterVertexCount = static_cast<GLsizei>(mesh.size() / static_cast<size_t>(kWStride));
}

void Renderer::buildInterior() {
    std::vector<float> mesh;
    pushTri(mesh, -7.0f, 0.0f, -5.0f, 7.0f, 0.0f, -5.0f, 7.0f, 0.0f, 5.0f, 0, 1, 0, 0.42f, 0.39f, 0.35f);
    pushTri(mesh, -7.0f, 0.0f, -5.0f, 7.0f, 0.0f, 5.0f, -7.0f, 0.0f, 5.0f, 0, 1, 0, 0.42f, 0.39f, 0.35f);
    pushTri(mesh, -7.0f, 0.0f, -5.0f, -7.0f, 4.0f, -5.0f, 7.0f, 4.0f, -5.0f, 0, 0, 1, 0.78f, 0.76f, 0.72f);
    pushTri(mesh, -7.0f, 0.0f, -5.0f, 7.0f, 4.0f, -5.0f, 7.0f, 0.0f, -5.0f, 0, 0, 1, 0.78f, 0.76f, 0.72f);
    pushTri(mesh, -7.0f, 0.0f, -5.0f, -7.0f, 0.0f, 5.0f, -7.0f, 4.0f, 5.0f, -1, 0, 0, 0.75f, 0.73f, 0.7f);
    pushTri(mesh, -7.0f, 0.0f, -5.0f, -7.0f, 4.0f, 5.0f, -7.0f, 4.0f, -5.0f, -1, 0, 0, 0.75f, 0.73f, 0.7f);
    pushTri(mesh, 7.0f, 0.0f, -5.0f, 7.0f, 0.0f, 5.0f, 7.0f, 4.0f, 5.0f, 1, 0, 0, 0.75f, 0.73f, 0.7f);
    pushTri(mesh, 7.0f, 0.0f, -5.0f, 7.0f, 4.0f, 5.0f, 7.0f, 4.0f, -5.0f, 1, 0, 0, 0.75f, 0.73f, 0.7f);
    addCuboid(mesh, 0.0f, 0.55f, 1.2f, 12.0f, 1.1f, 1.6f, 0.32f, 0.2f, 0.12f);
    addCuboid(mesh, 0.0f, 1.25f, -1.0f, 0.5f, 1.9f, 0.45f, 0.5f, 0.38f, 0.32f);
    for (int i = 0; i < 6; ++i) {
        const float px = -4.5f + static_cast<float>(i) * 1.65f;
        addCuboid(mesh, px, 0.9f, 3.4f, 0.48f, 1.75f, 0.38f, 0.15f + 0.07f * static_cast<float>(i % 3), 0.32f, 0.55f);
    }
    uploadColoredMesh(m_interiorVao, m_interiorVbo, mesh, &m_interiorVertexCount);
}

bool Renderer::init(const std::string& shaderRoot, const Scene& scene) {
    destroyGpu();
    m_shaderRoot = shaderRoot;
    const char* candidates[] = {shaderRoot.c_str(), "./shaders/", "../shaders/", "../../BusStandSimulator/shaders/"};
    std::string loadedFrom;
    for (const char* c : candidates) {
        std::string d(c);
        if (tryLoadShaderPair(m_standard, d, "standard") && tryLoadShaderPair(m_terrain, d, "terrain") &&
            tryLoadShaderPair(m_water, d, "water") && tryLoadShaderPair(m_triplanar, d, "triplanar") &&
            tryLoadShaderPair(m_stall, d, "stall")) {
            loadedFrom = d;
            break;
        }
    }
    if (loadedFrom.empty()) {
        std::fprintf(stderr, "Renderer: failed to load shaders\n");
        return false;
    }
    loadTextures();
    buildTerrain(scene);
    buildRoadMesh(scene);
    buildTriplanarMesh(scene);
    buildColoredStatic(scene);
    buildWater(scene);
    buildInterior();
    return m_terrainVertexCount > 0 && m_roadVertexCount > 0 && m_tpStationVertexCount > 0 && m_tpCityVertexCount > 0 &&
        m_tpTowerVertexCount > 0;
}

void Renderer::drawTexturedQuad(GLuint program, const float* view, const float* proj, const float* model, float nx,
    float ny, float nz, const float* verts12, float u0, float v0, float u1, float v1, GLuint tex, float fallbackR,
    float fallbackG, float fallbackB, std::vector<float>& scratch) {
    scratch.clear();
    const float verts[6][8] = {
        {verts12[0], verts12[1], verts12[2], nx, ny, nz, u0, v0},
        {verts12[3], verts12[4], verts12[5], nx, ny, nz, u1, v0},
        {verts12[6], verts12[7], verts12[8], nx, ny, nz, u1, v1},
        {verts12[0], verts12[1], verts12[2], nx, ny, nz, u0, v0},
        {verts12[6], verts12[7], verts12[8], nx, ny, nz, u1, v1},
        {verts12[9], verts12[10], verts12[11], nx, ny, nz, u0, v1},
    };
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 8; ++j) {
            scratch.push_back(verts[i][j]);
        }
    }
    static GLuint vao = 0;
    static GLuint vbo = 0;
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(scratch.size() * sizeof(float)), scratch.data(),
        GL_STREAM_DRAW);
    const GLsizei stride = static_cast<GLsizei>(8 * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));

    glUseProgram(program);
    GLint locM = glGetUniformLocation(program, "uModel");
    GLint locV = glGetUniformLocation(program, "uView");
    GLint locP = glGetUniformLocation(program, "uProjection");
    glUniformMatrix4fv(locM, 1, GL_FALSE, model);
    glUniformMatrix4fv(locV, 1, GL_FALSE, view);
    glUniformMatrix4fv(locP, 1, GL_FALSE, proj);

    const bool useTex = (tex != 0);
    glUniform1f(glGetUniformLocation(program, "uUseTexture"), useTex ? 1.0f : 0.0f);
    glUniform3f(glGetUniformLocation(program, "uBaseColor"), fallbackR, fallbackG, fallbackB);
    glUniform1f(glGetUniformLocation(program, "uShininess"), 48.0f);
    glUniform1f(glGetUniformLocation(program, "uSpecularStrength"), 0.35f);
    if (useTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(program, "uTex"), 0);
    }
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::drawBuses(const Scene& scene, const float* view, const float* proj, const float* cameraPos,
    const AppRenderSettings& settings) {
    GLuint prog = m_standard.id();

    PointLight pts[2];
    pts[0].position[0] = 0;
    pts[0].position[1] = 12;
    pts[0].position[2] = 0;
    pts[0].ambient[0] = pts[0].ambient[1] = pts[0].ambient[2] = 0.03f;
    pts[0].diffuse[0] = 0.45f;
    pts[0].diffuse[1] = 0.43f;
    pts[0].diffuse[2] = 0.35f;
    pts[0].specular[0] = 0.28f;
    pts[0].specular[1] = 0.25f;
    pts[0].specular[2] = 0.20f;
    pts[0].constant = 1.0f;
    pts[0].linear = 0.03f;
    pts[0].quadratic = 0.004f;
    pts[1] = pts[0];
    pts[1].position[0] = 40;
    pts[1].position[1] = 20;
    pts[1].position[2] = 10;

    Spotlight spots[kMaxSpotlights];
    const int nSpot = fillSpotlights(scene, spots);
    DirectionalLight sun = makeSun(settings);
    float ga[3] = {0.26f * settings.globalAmbientScale, 0.26f * settings.globalAmbientScale,
        0.30f * settings.globalAmbientScale};
    const int nPt = settings.pointLightsEnabled ? 2 : 0;
    bindPhongFogSpots(prog, cameraPos, ga, sun, nPt, pts, nSpot, spots, settings);

    GLint locM = glGetUniformLocation(prog, "uModel");
    GLint locV = glGetUniformLocation(prog, "uView");
    GLint locP = glGetUniformLocation(prog, "uProjection");
    glUseProgram(prog);
    glUniformMatrix4fv(locV, 1, GL_FALSE, view);
    glUniformMatrix4fv(locP, 1, GL_FALSE, proj);

    std::vector<float> scratch;
    scratch.reserve(256);

    const unsigned int texRight =
        Bus::windowTextureId() != 0 ? Bus::windowTextureId() : (Bus::bodyTextureId() != 0 ? Bus::bodyTextureId() : 0);
    const unsigned int texLeft = texRight;
    const unsigned int texFront =
        Bus::frontTextureId() != 0 ? Bus::frontTextureId() : (Bus::bodyTextureId() != 0 ? Bus::bodyTextureId() : 0);
    const unsigned int texRear =
        Bus::backTextureId() != 0 ? Bus::backTextureId() : (Bus::bodyTextureId() != 0 ? Bus::bodyTextureId() : 0);
    const unsigned int texTop = Bus::bodyTextureId();
    const unsigned int texBottom = Bus::bodyTextureId();

    for (const Bus& bus : scene.getBuses()) {
        const Vec3 pos = bus.getPosition();
        const float heading = bus.getHeadingDegrees();
        const float L = bus.getLength();
        const float W = bus.getWidth();
        const float H = bus.getHeight();
        const float hx = L * 0.5f;
        const float hy = H * 0.46f;
        const float hz = W * 0.5f;

        float T[16], R[16], S[16], M[16], tmp[16];
        mat4Translate(pos.x, 0.9f, pos.z, T);
        mat4RotateY(heading * (kMathPi / 180.0f), R);
        mat4Scale(1, 1, 1, S);
        mat4Mul(R, S, tmp);
        mat4Mul(T, tmp, M);

        if (!Bus::hasAnyBusTexture()) {
            glUseProgram(prog);
            glUniformMatrix4fv(locM, 1, GL_FALSE, M);
            glUniformMatrix4fv(locV, 1, GL_FALSE, view);
            glUniformMatrix4fv(locP, 1, GL_FALSE, proj);
            glUniform1f(glGetUniformLocation(prog, "uUseTexture"), 0.0f);
            glUniform3f(glGetUniformLocation(prog, "uBaseColor"), 0.06f, 0.06f, 0.08f);
            glUniform1f(glGetUniformLocation(prog, "uShininess"), 32.0f);
            glUniform1f(glGetUniformLocation(prog, "uSpecularStrength"), 0.2f);
            addCuboid(scratch, 0, 0, 0, L, H * 0.92f, W, 0.06f, 0.06f, 0.08f);
            static GLuint busVao = 0, busVbo = 0;
            if (busVao == 0) {
                glGenVertexArrays(1, &busVao);
                glGenBuffers(1, &busVbo);
            }
            glBindVertexArray(busVao);
            glBindBuffer(GL_ARRAY_BUFFER, busVbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(scratch.size() * sizeof(float)), scratch.data(),
                GL_STREAM_DRAW);
            const GLsizei stride = static_cast<GLsizei>(kStride * sizeof(float));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(scratch.size() / kStride));
            glBindVertexArray(0);
            scratch.clear();
            continue;
        }

        float v[12];
        auto face = [&](unsigned int tex, float fr, float fg, float fb, float nx, float ny, float nz, float u0, float tv0,
                        float u1, float tv1) {
            drawTexturedQuad(prog, view, proj, M, nx, ny, nz, v, u0, tv0, u1, tv1, tex, fr, fg, fb, scratch);
        };

        v[0] = -hx;
        v[1] = -hy;
        v[2] = hz;
        v[3] = hx;
        v[4] = -hy;
        v[5] = hz;
        v[6] = hx;
        v[7] = hy;
        v[8] = hz;
        v[9] = -hx;
        v[10] = hy;
        v[11] = hz;
        face(texRight, 0.12f, 0.12f, 0.14f, 0, 0, 1, 0, 0, 2, 1);

        v[0] = -hx;
        v[1] = -hy;
        v[2] = -hz;
        v[3] = hx;
        v[4] = -hy;
        v[5] = -hz;
        v[6] = hx;
        v[7] = hy;
        v[8] = -hz;
        v[9] = -hx;
        v[10] = hy;
        v[11] = -hz;
        face(texLeft, 0.12f, 0.12f, 0.14f, 0, 0, -1, 2, 0, 0, 1);

        v[0] = hx;
        v[1] = -hy;
        v[2] = -hz;
        v[3] = hx;
        v[4] = -hy;
        v[5] = hz;
        v[6] = hx;
        v[7] = hy;
        v[8] = hz;
        v[9] = hx;
        v[10] = hy;
        v[11] = -hz;
        face(texFront, 0.14f, 0.14f, 0.16f, 1, 0, 0, 0, 0, 1, 1);

        v[0] = -hx;
        v[1] = -hy;
        v[2] = hz;
        v[3] = -hx;
        v[4] = -hy;
        v[5] = -hz;
        v[6] = -hx;
        v[7] = hy;
        v[8] = -hz;
        v[9] = -hx;
        v[10] = hy;
        v[11] = hz;
        face(texRear, 0.10f, 0.10f, 0.12f, -1, 0, 0, 1, 0, 0, 1);

        v[0] = -hx;
        v[1] = hy;
        v[2] = -hz;
        v[3] = hx;
        v[4] = hy;
        v[5] = -hz;
        v[6] = hx;
        v[7] = hy;
        v[8] = hz;
        v[9] = -hx;
        v[10] = hy;
        v[11] = hz;
        face(texTop, 0.16f, 0.16f, 0.18f, 0, 1, 0, 0, 0, 2, 2);

        v[0] = -hx;
        v[1] = -hy;
        v[2] = hz;
        v[3] = hx;
        v[4] = -hy;
        v[5] = hz;
        v[6] = hx;
        v[7] = -hy;
        v[8] = -hz;
        v[9] = -hx;
        v[10] = -hy;
        v[11] = -hz;
        face(texBottom, 0.05f, 0.05f, 0.06f, 0, -1, 0, 0, 0, 2, 2);

        const float ridge = hy + 0.11f;
        float rv[12] = {-L * 0.495f, ridge, hz * 0.96f, L * 0.495f, ridge, hz * 0.96f, L * 0.495f, ridge, -hz * 0.96f,
            -L * 0.495f, ridge, -hz * 0.96f};
        if (texTop != 0) {
            drawTexturedQuad(prog, view, proj, M, 0, 1, 0, rv, 0, 0, 3.2f, 2.2f, texTop, 0.12f, 0.55f, 0.22f, scratch);
        } else {
            float G[16];
            mat4Translate(pos.x, 0.9f + H * 0.48f, pos.z, T);
            mat4RotateY(heading * (kMathPi / 180.0f), R);
            mat4Mul(T, R, G);
            glUseProgram(prog);
            glUniformMatrix4fv(locM, 1, GL_FALSE, G);
            glUniformMatrix4fv(locV, 1, GL_FALSE, view);
            glUniformMatrix4fv(locP, 1, GL_FALSE, proj);
            glUniform1f(glGetUniformLocation(prog, "uUseTexture"), 0.0f);
            glUniform3f(glGetUniformLocation(prog, "uBaseColor"), 0.04f, 0.78f, 0.32f);
            scratch.clear();
            addCuboid(scratch, 0, 0, 0, L * 0.99f, 0.22f, W * 0.98f, 0.04f, 0.78f, 0.32f);
            static GLuint gVao = 0, gVbo = 0;
            if (gVao == 0) {
                glGenVertexArrays(1, &gVao);
                glGenBuffers(1, &gVbo);
            }
            glBindVertexArray(gVao);
            glBindBuffer(GL_ARRAY_BUFFER, gVbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(scratch.size() * sizeof(float)), scratch.data(),
                GL_STREAM_DRAW);
            const GLsizei stride2 = static_cast<GLsizei>(kStride * sizeof(float));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride2, reinterpret_cast<void*>(0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride2, reinterpret_cast<void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride2, reinterpret_cast<void*>(6 * sizeof(float)));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(scratch.size() / kStride));
            glBindVertexArray(0);
        }
    }
}

void Renderer::drawInteriorScene(float timeSec, const float* view, const float* proj, const float* camPos,
    const AppRenderSettings& settings) {
    GLuint stallP = m_stall.id();
    glUseProgram(stallP);
    glUniformMatrix4fv(glGetUniformLocation(stallP, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(stallP, "uProjection"), 1, GL_FALSE, proj);
    glUniform3f(glGetUniformLocation(stallP, "uLightPos"), 1.2f, 3.6f, 1.5f);
    glUniform1f(glGetUniformLocation(stallP, "uTime"), timeSec);
    float model[16];
    mat4Identity(model);
    glUniformMatrix4fv(glGetUniformLocation(stallP, "uModel"), 1, GL_FALSE, model);
    glBindVertexArray(m_interiorVao);
    glDrawArrays(GL_TRIANGLES, 0, m_interiorVertexCount);
    glBindVertexArray(0);

    if (m_hasNpcTex) {
        GLuint prog = m_standard.id();
        PointLight pts[2];
        pts[0].position[0] = 1.0f;
        pts[0].position[1] = 3.5f;
        pts[0].position[2] = 2.0f;
        pts[0].ambient[0] = pts[0].ambient[1] = pts[0].ambient[2] = 0.08f;
        pts[0].diffuse[0] = 0.55f;
        pts[0].diffuse[1] = 0.52f;
        pts[0].diffuse[2] = 0.48f;
        pts[0].specular[0] = 0.25f;
        pts[0].specular[1] = 0.23f;
        pts[0].specular[2] = 0.2f;
        pts[0].constant = 1.0f;
        pts[0].linear = 0.12f;
        pts[0].quadratic = 0.06f;
        pts[1] = pts[0];
        pts[1].position[0] = -2.0f;
        pts[1].position[2] = -1.0f;
        DirectionalLight sun{{0.3f, 1.0f, 0.2f}, {0.18f, 0.18f, 0.2f}, {0.35f, 0.34f, 0.32f}, {0.2f, 0.2f, 0.18f}};
        float ga[3] = {0.28f * settings.globalAmbientScale, 0.27f * settings.globalAmbientScale,
            0.26f * settings.globalAmbientScale};
        Spotlight spots[kMaxSpotlights];
        bindPhongFogSpots(prog, camPos, ga, sun, 2, pts, 0, spots, settings);

        std::vector<float> scratch;
        scratch.reserve(128);
        GLint locM = glGetUniformLocation(prog, "uModel");
        GLint locV = glGetUniformLocation(prog, "uView");
        GLint locP = glGetUniformLocation(prog, "uProjection");
        glUniformMatrix4fv(locV, 1, GL_FALSE, view);
        glUniformMatrix4fv(locP, 1, GL_FALSE, proj);

        auto npc = [&](float px, float pz, float w, float h) {
            float T[16], M[16];
            mat4Translate(px, 0.0f, pz, T);
            mat4Mul(T, model, M);
            float vv[12] = {-w * 0.5f, 0, 0, w * 0.5f, 0, 0, w * 0.5f, h, 0, -w * 0.5f, h, 0};
            drawTexturedQuad(prog, view, proj, M, 0, 0, 1, vv, 0, 0, 1, 1, m_npcTex, 0.55f, 0.5f, 0.48f, scratch);
        };
        npc(-3.2f, 0.8f, 0.85f, 1.75f);
        npc(-1.0f, 0.5f, 0.8f, 1.7f);
        npc(1.4f, 1.0f, 0.82f, 1.72f);
        npc(3.6f, 0.6f, 0.78f, 1.68f);
    }
}

void Renderer::drawOutdoor(const Scene& scene, const CameraController& camera, const Bus& driverBus, int fbWidth,
    int fbHeight, float timeSec, const AppRenderSettings& settings) {
    const float aspect = (fbHeight > 0) ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f;
    float view[16];
    float proj[16];
    camera.getViewMatrix(driverBus, view);
    mat4Perspective(60.0f, aspect, 0.1f, 520.0f, proj);

    float camPos[3];
    camera.getCameraWorldPosition(driverBus, camPos);

    float model[16];
    mat4Identity(model);

    PointLight pts[2];
    pts[0].position[0] = 0;
    pts[0].position[1] = 12;
    pts[0].position[2] = 0;
    pts[0].ambient[0] = pts[0].ambient[1] = pts[0].ambient[2] = 0.03f;
    pts[0].diffuse[0] = 0.45f;
    pts[0].diffuse[1] = 0.43f;
    pts[0].diffuse[2] = 0.35f;
    pts[0].specular[0] = 0.28f;
    pts[0].specular[1] = 0.25f;
    pts[0].specular[2] = 0.20f;
    pts[0].constant = 1.0f;
    pts[0].linear = 0.03f;
    pts[0].quadratic = 0.004f;
    pts[1] = pts[0];
    pts[1].position[0] = 40;
    pts[1].position[1] = 20;
    pts[1].position[2] = 10;

    Spotlight spots[kMaxSpotlights];
    const int nSpot = fillSpotlights(scene, spots);
    DirectionalLight sun = makeSun(settings);
    float ga[3] = {0.26f * settings.globalAmbientScale, 0.26f * settings.globalAmbientScale,
        0.30f * settings.globalAmbientScale};
    const int nPt = settings.pointLightsEnabled ? 2 : 0;

    GLuint terrainProg = m_terrain.id();
    bindPhongFogSpots(terrainProg, camPos, ga, sun, nPt, pts, nSpot, spots, settings);
    glUseProgram(terrainProg);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uModel"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(terrainProg, "uShininess"), 24.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uSpecularStrength"), 0.18f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUseGrassTex"), m_hasGrassTex ? 1.0f : 0.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUsePathTex"), m_hasPathTex ? 1.0f : 0.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUseGroundDetailTex"), m_hasGroundDetailTex ? 1.0f : 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hasGrassTex ? m_grassTex : m_whiteTex);
    glUniform1i(glGetUniformLocation(terrainProg, "uGrassTex"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_hasPathTex ? m_pathTex : m_whiteTex);
    glUniform1i(glGetUniformLocation(terrainProg, "uPathTex"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_hasGroundDetailTex ? m_groundDetailTex : m_whiteTex);
    glUniform1i(glGetUniformLocation(terrainProg, "uGroundDetailTex"), 2);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_terrainVao);
    glDrawArrays(GL_TRIANGLES, 0, m_terrainVertexCount);
    glBindVertexArray(0);

    bindPhongFogSpots(terrainProg, camPos, ga, sun, nPt, pts, nSpot, spots, settings);
    glUseProgram(terrainProg);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uModel"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(terrainProg, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(terrainProg, "uShininess"), 36.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uSpecularStrength"), 0.22f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUseGrassTex"), 0.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUsePathTex"), m_hasPathTex ? 1.0f : 0.0f);
    glUniform1f(glGetUniformLocation(terrainProg, "uUseGroundDetailTex"), m_hasGroundDetailTex ? 1.0f : 0.0f);
    glBindVertexArray(m_staticVao);
    glDrawArrays(GL_TRIANGLES, 0, m_staticVertexCount);
    glBindVertexArray(0);

    GLuint stdP = m_standard.id();
    bindPhongFogSpots(stdP, camPos, ga, sun, nPt, pts, nSpot, spots, settings);
    glUseProgram(stdP);
    glUniformMatrix4fv(glGetUniformLocation(stdP, "uModel"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(stdP, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(stdP, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(stdP, "uShininess"), 28.0f);
    glUniform1f(glGetUniformLocation(stdP, "uSpecularStrength"), 0.2f);
    glUniform1f(glGetUniformLocation(stdP, "uUseTexture"), m_hasRoadTex ? 1.0f : 0.0f);
    glUniform3f(glGetUniformLocation(stdP, "uBaseColor"), 0.12f, 0.12f, 0.13f);
    if (m_hasRoadTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_roadTex);
        glUniform1i(glGetUniformLocation(stdP, "uTex"), 0);
    }
    glBindVertexArray(m_roadVao);
    glDrawArrays(GL_TRIANGLES, 0, m_roadVertexCount);
    glBindVertexArray(0);

    GLuint tpP = m_triplanar.id();
    auto drawTriplanarGroup = [&](GLuint tex, bool hasTex, float scale, float tintR, float tintG, float tintB) {
        bindPhongFogSpots(tpP, camPos, ga, sun, nPt, pts, nSpot, spots, settings);
        glUseProgram(tpP);
        glUniformMatrix4fv(glGetUniformLocation(tpP, "uModel"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(tpP, "uView"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(tpP, "uProjection"), 1, GL_FALSE, proj);
        glUniform1f(glGetUniformLocation(tpP, "uTexScale"), scale);
        glUniform3f(glGetUniformLocation(tpP, "uTint"), tintR, tintG, tintB);
        glUniform1f(glGetUniformLocation(tpP, "uShininess"), 22.0f);
        glUniform1f(glGetUniformLocation(tpP, "uSpecularStrength"), 0.16f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hasTex ? tex : m_whiteTex);
        glUniform1i(glGetUniformLocation(tpP, "uTex"), 0);
    };
    drawTriplanarGroup(m_buildingTex, m_hasBuildingTex, 0.10f, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(m_tpStationVao);
    glDrawArrays(GL_TRIANGLES, 0, m_tpStationVertexCount);
    glBindVertexArray(0);

    drawTriplanarGroup(m_buildingCityTex, m_hasBuildingCityTex, 0.11f, 1.02f, 0.98f, 0.95f);
    glBindVertexArray(m_tpCityVao);
    glDrawArrays(GL_TRIANGLES, 0, m_tpCityVertexCount);
    glBindVertexArray(0);

    drawTriplanarGroup(m_buildingTowerTex, m_hasBuildingTowerTex, 0.065f, 0.95f, 0.97f, 1.02f);
    glBindVertexArray(m_tpTowerVao);
    glDrawArrays(GL_TRIANGLES, 0, m_tpTowerVertexCount);
    glBindVertexArray(0);

    drawWindBanner(scene, view, proj, camPos, timeSec, settings);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint wprog = m_water.id();
    glUseProgram(wprog);
    glUniformMatrix4fv(glGetUniformLocation(wprog, "uModel"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(wprog, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(wprog, "uProjection"), 1, GL_FALSE, proj);
    glUniform3fv(glGetUniformLocation(wprog, "uCameraPos"), 1, camPos);
    applyFogUniforms(wprog, settings.fogDensity, settings.fogColor);
    glUniform3fv(glGetUniformLocation(wprog, "uLightDir"), 1, sun.dir);
    glUniform1f(glGetUniformLocation(wprog, "uTime"), timeSec);
    GLint uLakeC = glGetUniformLocation(wprog, "uLakeCenterXZ");
    if (uLakeC >= 0) {
        glUniform2f(uLakeC, 63.0f, 62.0f);
    }
    GLint uLakeF = glGetUniformLocation(wprog, "uLakeFadeScale");
    if (uLakeF >= 0) {
        glUniform1f(uLakeF, 1.0f / 17.5f);
    }
    glUniform1f(glGetUniformLocation(wprog, "uUseWaterTex"), m_hasWaterTex ? 1.0f : 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hasWaterTex ? m_waterTex : m_whiteTex);
    glUniform1i(glGetUniformLocation(wprog, "uWaterTex"), 0);
    glBindVertexArray(m_waterVao);
    glDrawArrays(GL_TRIANGLES, 0, m_waterVertexCount);
    glBindVertexArray(0);
    glDisable(GL_BLEND);

    drawBuses(scene, view, proj, camPos, settings);
}

void Renderer::drawWindBanner(const Scene& scene, const float* view, const float* proj, const float* camPos,
    float timeSec, const AppRenderSettings& settings) {
    if (!m_hasFlagTex) {
        return;
    }
    GLuint prog = m_standard.id();
    PointLight pts[2];
    pts[0].position[0] = 0;
    pts[0].position[1] = 12;
    pts[0].position[2] = 0;
    pts[0].ambient[0] = pts[0].ambient[1] = pts[0].ambient[2] = 0.03f;
    pts[0].diffuse[0] = 0.45f;
    pts[0].diffuse[1] = 0.43f;
    pts[0].diffuse[2] = 0.35f;
    pts[0].specular[0] = 0.28f;
    pts[0].specular[1] = 0.25f;
    pts[0].specular[2] = 0.20f;
    pts[0].constant = 1.0f;
    pts[0].linear = 0.03f;
    pts[0].quadratic = 0.004f;
    pts[1] = pts[0];
    pts[1].position[0] = 40;
    pts[1].position[1] = 20;
    pts[1].position[2] = 10;
    Spotlight spots[kMaxSpotlights];
    const int nSpot = fillSpotlights(scene, spots);
    DirectionalLight sun = makeSun(settings);
    float ga[3] = {0.26f * settings.globalAmbientScale, 0.26f * settings.globalAmbientScale,
        0.30f * settings.globalAmbientScale};
    const int nPt = settings.pointLightsEnabled ? 2 : 0;
    bindPhongFogSpots(prog, camPos, ga, sun, nPt, pts, nSpot, spots, settings);
    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uView"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uProjection"), 1, GL_FALSE, proj);
    glUniform1f(glGetUniformLocation(prog, "uShininess"), 26.0f);
    glUniform1f(glGetUniformLocation(prog, "uSpecularStrength"), 0.2f);

    const float poleX = 28.5f;
    const float poleZ = -36.0f;
    const float baseY = scene.terrainHeight(poleX, poleZ);
    const float swayRad = (11.0f + 22.0f * std::sin(timeSec * 2.25f)) * (kMathPi / 180.0f);
    const float flap = 0.2f * std::sin(timeSec * 5.85f);

    float T[16], R[16], M[16];
    mat4Translate(poleX, baseY + 3.5f, poleZ, T);
    mat4RotateY(swayRad, R);
    mat4Mul(T, R, M);

    std::vector<float> scratch;
    float vf[12] = {0, -0.2f, 0, 0, 2.6f, 0, 0, 2.6f, 3.25f + flap, 0, -0.2f, 3.25f};
    drawTexturedQuad(prog, view, proj, M, 1, 0, 0, vf, 0, 0, 1, 1, m_flagTex, 0.85f, 0.78f, 0.55f, scratch);
    float vb[12] = {0, -0.2f, 0, 0, -0.2f, 3.25f, 0, 2.6f, 3.25f + flap, 0, 2.6f, 0};
    drawTexturedQuad(prog, view, proj, M, -1, 0, 0, vb, 1, 0, 0, 1, m_flagTex, 0.85f, 0.78f, 0.55f, scratch);
}

void Renderer::draw(const Scene& scene, const CameraController& camera, const Bus& driverBus, int fbWidth, int fbHeight,
    float timeSec, WorldMode worldMode, const AppRenderSettings& settings) {
    if (worldMode == WorldMode::TicketOffice) {
        const float aspect = (fbHeight > 0) ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f;
        float view[16];
        float proj[16];
        camera.getViewMatrix(driverBus, view);
        mat4Perspective(55.0f, aspect, 0.08f, 80.0f, proj);
        float camPos[3];
        camera.getCameraWorldPosition(driverBus, camPos);
        drawInteriorScene(timeSec, view, proj, camPos, settings);
        return;
    }
    drawOutdoor(scene, camera, driverBus, fbWidth, fbHeight, timeSec, settings);
}
