#include "shader_program.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

ShaderProgram::~ShaderProgram() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

std::string ShaderProgram::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "Shader file not found: " << path << "\n";
        return {};
    }
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

GLuint ShaderProgram::compile(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

bool ShaderProgram::loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
    const std::string vs = readFile(vertPath);
    const std::string fs = readFile(fragPath);
    if (vs.empty() || fs.empty()) {
        return false;
    }
    GLuint v = compile(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compile(GL_FRAGMENT_SHADER, fs.c_str());
    if (v == 0 || f == 0) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return false;
    }
    m_program = glCreateProgram();
    glAttachShader(m_program, v);
    glAttachShader(m_program, f);
    glLinkProgram(m_program);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint linked = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

GLint ShaderProgram::uniformLocation(const char* name) const {
    return glGetUniformLocation(m_program, name);
}
