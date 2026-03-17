#pragma once

#include "FDE/Export.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace FDE
{

class FDE_API Shader
{
  public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void Bind() const;
    void Unbind() const;

    void SetMat4(const std::string& name, const glm::mat4& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);

    unsigned int GetId() const { return m_id; }

  private:
    int GetUniformLocation(const std::string& name);

    unsigned int m_id = 0;
    std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace FDE
