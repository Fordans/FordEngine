#pragma once

#include "FDE/Export.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace FDE
{

enum class ShaderDataType
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    Mat3,
    Mat4
};

inline uint32_t ShaderDataTypeSize(ShaderDataType type)
{
    switch (type)
    {
    case ShaderDataType::Float:
        return 4;
    case ShaderDataType::Float2:
        return 8;
    case ShaderDataType::Float3:
        return 12;
    case ShaderDataType::Float4:
        return 16;
    case ShaderDataType::Int:
        return 4;
    case ShaderDataType::Int2:
        return 8;
    case ShaderDataType::Int3:
        return 12;
    case ShaderDataType::Int4:
        return 16;
    case ShaderDataType::Mat3:
        return 36;
    case ShaderDataType::Mat4:
        return 64;
    default:
        return 0;
    }
}

struct BufferElement
{
    std::string name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;

    BufferElement() = default;
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
        : name(name), type(type), size(ShaderDataTypeSize(type)), offset(0), normalized(normalized)
    {
    }

    uint32_t GetComponentCount() const
    {
        switch (type)
        {
        case ShaderDataType::Float:
        case ShaderDataType::Int:
            return 1;
        case ShaderDataType::Float2:
        case ShaderDataType::Int2:
            return 2;
        case ShaderDataType::Float3:
        case ShaderDataType::Int3:
            return 3;
        case ShaderDataType::Float4:
        case ShaderDataType::Int4:
            return 4;
        case ShaderDataType::Mat3:
            return 3 * 3;
        case ShaderDataType::Mat4:
            return 4 * 4;
        default:
            return 0;
        }
    }
};

class FDE_API VertexBuffer
{
  public:
    VertexBuffer(const void* data, size_t size);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    void Bind() const;
    void Unbind() const;

    void SetData(const void* data, size_t size);

    uint32_t GetId() const { return m_id; }

  private:
    uint32_t m_id = 0;
};

} // namespace FDE
