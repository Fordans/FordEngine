#include "FDE/Renderer/Shader.hpp"
#include "FDE/Core/Log.hpp"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace FDE
{

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetShaderInfoLog(id, length, &length, log.data());
        FDE_LOG_CLIENT_ERROR("Shader compilation failed: {}", log);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    if (!vertexShader || !fragmentShader)
    {
        if (vertexShader)
            glDeleteShader(vertexShader);
        if (fragmentShader)
            glDeleteShader(fragmentShader);
        return;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShader);
    glAttachShader(m_id, fragmentShader);
    glLinkProgram(m_id);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        int length;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '\0');
        glGetProgramInfoLog(m_id, length, &length, log.data());
        FDE_LOG_CLIENT_ERROR("Shader link failed: {}", log);
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

Shader::~Shader()
{
    if (m_id)
    {
        glDeleteProgram(m_id);
    }
}

void Shader::Bind() const
{
    if (m_id)
        glUseProgram(m_id);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

int Shader::GetUniformLocation(const std::string& name)
{
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end())
        return m_uniformLocationCache[name];

    int location = glGetUniformLocation(m_id, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value)
{
    int location = GetUniformLocation(name);
    if (location >= 0)
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value)
{
    int location = GetUniformLocation(name);
    if (location >= 0)
        glUniform4f(location, value.x, value.y, value.z, value.w);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value)
{
    int location = GetUniformLocation(name);
    if (location >= 0)
        glUniform3f(location, value.x, value.y, value.z);
}

void Shader::SetFloat(const std::string& name, float value)
{
    int location = GetUniformLocation(name);
    if (location >= 0)
        glUniform1f(location, value);
}

void Shader::SetInt(const std::string& name, int value)
{
    int location = GetUniformLocation(name);
    if (location >= 0)
        glUniform1i(location, value);
}

} // namespace FDE
