#pragma once

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Loads an RGBA PNG/JPG (common formats supported by stb_image) into a 2D texture.
// Returns 0 on failure. Caller should not delete; texture lives for app lifetime.
GLuint loadTextureFromImageFile(const char* path);

// Tries paths in order; returns first successful texture id, or 0 if all fail.
GLuint loadTextureFromSearchPaths(const char* const* paths, int pathCount);
