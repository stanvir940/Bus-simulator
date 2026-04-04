#pragma once

#include <GL/glew.h>
#include <string>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);
    void use() const { glUseProgram(m_program); }
    GLuint id() const { return m_program; }

    GLint uniformLocation(const char* name) const;

private:
    GLuint m_program = 0;
    static std::string readFile(const std::string& path);
    static GLuint compile(GLenum type, const char* src);
};
