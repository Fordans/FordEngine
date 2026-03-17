#pragma once

#include "FDE/Export.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace FDE
{

/// Abstract interface for shader programs.
/// Use Shader::Create() to obtain a backend-specific implementation.
class FDE_API Shader
{
  public:
    virtual ~Shader() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;
    virtual void SetVec4(const std::string& name, const glm::vec4& value) = 0;
    virtual void SetVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetInt(const std::string& name, int value) = 0;

    /// Factory: creates a shader for the active graphics API.
    static std::unique_ptr<Shader> Create(const std::string& vertexSrc, const std::string& fragmentSrc);

  protected:
    Shader() = default;
};

} // namespace FDE
