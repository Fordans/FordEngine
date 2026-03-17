#pragma once

#include "FDE/Export.hpp"
#include "FDE/Renderer/Shader.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace FDE
{

class FDE_API OpenGLShader : public Shader
{
  public:
    OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~OpenGLShader() override;

    void Bind() const override;
    void Unbind() const override;

    void SetMat4(const std::string& name, const glm::mat4& value) override;
    void SetVec4(const std::string& name, const glm::vec4& value) override;
    void SetVec3(const std::string& name, const glm::vec3& value) override;
    void SetFloat(const std::string& name, float value) override;
    void SetInt(const std::string& name, int value) override;

  private:
    int GetUniformLocation(const std::string& name);

    unsigned int m_id = 0;
    std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace FDE
