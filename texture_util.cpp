#include "texture_util.h"

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

bool tryLoad(const char* path, GLuint* outId) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return false;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    stbi_image_free(pixels);
    *outId = tex;
    return true;
}

}  // namespace

GLuint loadTextureFromImageFile(const char* path) {
    GLuint id = 0;
    if (path != nullptr && tryLoad(path, &id)) {
        return id;
    }
    return 0;
}

GLuint loadTextureFromSearchPaths(const char* const* paths, int pathCount) {
    for (int i = 0; i < pathCount; ++i) {
      GLuint id = loadTextureFromImageFile(paths[i]);
      if (id != 0) {
          return id;
      }
    }
    return 0;
}
