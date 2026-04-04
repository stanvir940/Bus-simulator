#pragma once

#include <cmath>
#include <cstring>

// Column-major 4x4 matrices (OpenGL convention).
inline void mat4Identity(float* m) {
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

inline void mat4Mul(const float* a, const float* b, float* out) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            out[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] + a[2 * 4 + r] * b[c * 4 + 2] +
                             a[3 * 4 + r] * b[c * 4 + 3];
        }
    }
}

inline void mat4Perspective(float fovYDeg, float aspect, float zNear, float zFar, float* m) {
    const float rad = fovYDeg * 3.14159265f / 180.0f;
    const float f = 1.0f / std::tan(rad * 0.5f);
    std::memset(m, 0, 64);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
}

inline void mat4LookAt(float eyeX, float eyeY, float eyeZ, float cx, float cy, float cz, float ux, float uy, float uz,
    float* m) {
    float fx = cx - eyeX, fy = cy - eyeY, fz = cz - eyeZ;
    float len = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (len > 1e-6f) {
        fx /= len;
        fy /= len;
        fz /= len;
    }
    float sx = fy * uz - fz * uy;
    float sy = fz * ux - fx * uz;
    float sz = fx * uy - fy * ux;
    len = std::sqrt(sx * sx + sy * sy + sz * sz);
    if (len > 1e-6f) {
        sx /= len;
        sy /= len;
        sz /= len;
    }
    float ux2 = sy * fz - sz * fy;
    float uy2 = sz * fx - sx * fz;
    float uz2 = sx * fy - sy * fx;
    m[0] = sx;
    m[1] = ux2;
    m[2] = -fx;
    m[3] = 0.0f;
    m[4] = sy;
    m[5] = uy2;
    m[6] = -fy;
    m[7] = 0.0f;
    m[8] = sz;
    m[9] = uz2;
    m[10] = -fz;
    m[11] = 0.0f;
    m[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
    m[13] = -(ux2 * eyeX + uy2 * eyeY + uz2 * eyeZ);
    m[14] = -(-fx * eyeX - fy * eyeY - fz * eyeZ);
    m[15] = 1.0f;
}

inline void mat4Translate(float x, float y, float z, float* m) {
    mat4Identity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

inline void mat4Scale(float x, float y, float z, float* m) {
    mat4Identity(m);
    m[0] = x;
    m[5] = y;
    m[10] = z;
}

inline void mat4RotateY(float rad, float* m) {
    mat4Identity(m);
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    m[0] = c;
    m[8] = s;
    m[2] = -s;
    m[10] = c;
}

inline void mat4ComposeTRS(float tx, float ty, float tz, float rotYRad, float sx, float sy, float sz, float* out) {
    float t[16], r[16], s[16], tmp[16];
    mat4Translate(tx, ty, tz, t);
    mat4RotateY(rotYRad, r);
    mat4Scale(sx, sy, sz, s);
    mat4Mul(r, s, tmp);
    mat4Mul(t, tmp, out);
}
