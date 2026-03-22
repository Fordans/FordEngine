#include "FDE/pch.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Asset/AssetId.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/RenderCommand.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include <glad/glad.h>
#include <json11.hpp>
#include <fstream>
#include "stb_image.h"

namespace FDE
{

namespace
{

ShaderDataType ParseLayoutType(const std::string& s)
{
    if (s == "Float")
        return ShaderDataType::Float;
    if (s == "Float2")
        return ShaderDataType::Float2;
    if (s == "Float3")
        return ShaderDataType::Float3;
    if (s == "Float4")
        return ShaderDataType::Float4;
    return ShaderDataType::Float3;
}

void CreateBuiltinTriangle(std::shared_ptr<VertexArray>& outVAO)
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    uint32_t indices[] = {0, 1, 2};
    auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
    BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Color"}};
    outVAO = VertexArray::Create();
    if (outVAO && vbo)
    {
        outVAO->AddVertexBuffer(vbo, layout);
        outVAO->SetIndexBuffer(indices, 3);
    }
}

} // namespace

Texture2DResource::~Texture2DResource()
{
    if (glTextureId != 0)
    {
        GLuint t = static_cast<GLuint>(glTextureId);
        glDeleteTextures(1, &t);
        glTextureId = 0;
    }
}

AssetManager::AssetManager() = default;

AssetManager::~AssetManager()
{
    Shutdown();
}

void AssetManager::Initialize(const std::string& projectRoot)
{
    Shutdown();
    m_projectRoot = projectRoot;
    m_loadQueue = std::make_unique<AssetLoadQueue>();
    std::string err;
    if (!AssetRegistry::LoadFromProject(projectRoot, m_registry, err))
        FDE_LOG_CLIENT_WARN("Asset registry load: {}", err);
}

void AssetManager::SetActivePack(const std::string& absoluteFdepackPath)
{
    m_pack = std::make_unique<AssetPackReader>();
    std::string err;
    if (!m_pack->OpenFile(absoluteFdepackPath, err))
    {
        FDE_LOG_CLIENT_WARN("Failed to open fdepack {}: {}", absoluteFdepackPath, err);
        m_pack.reset();
    }
}

void AssetManager::Shutdown()
{
    if (m_loadQueue)
        m_loadQueue->Shutdown();
    m_loadQueue.reset();
    m_meshCache.clear();
    m_shaderCache.clear();
    m_textureCache.clear();
    m_pack.reset();
    m_registry = AssetRegistry{};
    m_projectRoot.clear();
}

void AssetManager::ProcessAsyncUploads()
{
    if (m_loadQueue)
        m_loadQueue->ProcessMainThread();
}

std::string AssetManager::ResolveBuiltAbsolute(const AssetRecord& rec) const
{
    if (rec.builtRelativePath.empty())
        return {};
    return FileSystem::ResolveProjectPath(rec.builtRelativePath);
}

const AssetRecord* AssetManager::ResolveRecordFromMeshString(const std::string& meshAsset) const
{
    if (auto id = AssetId::Parse(meshAsset))
        return m_registry.FindByGuid(*id);
    std::string norm = NormalizeLogicalPath(meshAsset);
    if (norm.size() >= 7 && norm.compare(0, 7, "Assets/") == 0)
        return m_registry.FindByLogicalPath(norm);
    return nullptr;
}

std::shared_ptr<VertexArray> AssetManager::LoadMesh2DFromBlob(const std::vector<uint8_t>& bytes)
{
    std::string content(bytes.begin(), bytes.end());
    std::string err;
    json11::Json json = json11::Json::parse(content, err);
    if (!err.empty() || !json.is_object())
    {
        FDE_LOG_CLIENT_ERROR("fdemesh JSON parse failed: {}", err);
        return nullptr;
    }

    const auto& o = json.object_items();
    auto itV = o.find("vertices");
    auto itI = o.find("indices");
    auto itL = o.find("layout");
    if (itV == o.end() || !itV->second.is_array() || itI == o.end() || !itI->second.is_array() ||
        itL == o.end() || !itL->second.is_array())
    {
        FDE_LOG_CLIENT_ERROR("fdemesh missing vertices/indices/layout");
        return nullptr;
    }

    std::vector<float> verts;
    for (const json11::Json& v : itV->second.array_items())
    {
        if (v.is_number())
            verts.push_back(static_cast<float>(v.number_value()));
    }

    std::vector<uint32_t> indices;
    for (const json11::Json& v : itI->second.array_items())
    {
        if (v.is_number())
            indices.push_back(static_cast<uint32_t>(v.number_value()));
    }

    std::vector<BufferElement> elems;
    for (const json11::Json& el : itL->second.array_items())
    {
        if (!el.is_object())
            continue;
        const auto& lo = el.object_items();
        auto itN = lo.find("name");
        auto itT = lo.find("type");
        if (itN == lo.end() || itT == lo.end())
            continue;
        elems.emplace_back(ParseLayoutType(itT->second.string_value()), itN->second.string_value());
    }
    if (elems.empty())
    {
        FDE_LOG_CLIENT_ERROR("fdemesh empty layout");
        return nullptr;
    }

    BufferLayout layout(elems);
    uint32_t stride = layout.GetStride();
    if (stride == 0 || (verts.size() * sizeof(float)) % stride != 0)
    {
        FDE_LOG_CLIENT_ERROR("fdemesh vertex size mismatch");
        return nullptr;
    }

    auto vbo = VertexBuffer::Create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(float)));
    auto vao = VertexArray::Create();
    if (!vao || !vbo)
        return nullptr;
    vao->AddVertexBuffer(vbo, layout);
    vao->SetIndexBuffer(indices.data(), static_cast<uint32_t>(indices.size()));
    return vao;
}

std::shared_ptr<VertexArray> AssetManager::LoadMesh2DFromBuilt(const AssetRecord& rec)
{
    std::string absPath = ResolveBuiltAbsolute(rec);
    if (absPath.empty())
        return nullptr;

    if (m_pack)
    {
        for (size_t i = 0; i < m_pack->GetIndex().size(); ++i)
        {
            if (m_pack->GetIndex()[i].guid == rec.guid)
            {
                std::vector<uint8_t> blob;
                if (m_pack->ReadBlob(i, blob))
                    return LoadMesh2DFromBlob(blob);
            }
        }
    }

    std::ifstream file(absPath, std::ios::binary);
    if (!file)
    {
        FDE_LOG_CLIENT_ERROR("Failed to open mesh built file: {}", absPath);
        return nullptr;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return LoadMesh2DFromBlob(bytes);
}

bool AssetManager::ResolveMesh2D(Mesh2DComponent& mesh)
{
    if (mesh.vertexArray && mesh.vertexArray->GetIndexCount() > 0)
        return true;

    if (mesh.meshAsset.empty())
        return false;

    if (mesh.meshAsset == "builtin:triangle")
    {
        std::shared_ptr<VertexArray> va;
        CreateBuiltinTriangle(va);
        mesh.vertexArray = va;
        return true;
    }

    const AssetRecord* rec = ResolveRecordFromMeshString(mesh.meshAsset);
    if (!rec || rec->type != AssetType::Mesh2D)
    {
        FDE_LOG_CLIENT_WARN("Mesh asset not found or wrong type: {}", mesh.meshAsset);
        return false;
    }

    std::string key = rec->guid.str();
    auto it = m_meshCache.find(key);
    if (it != m_meshCache.end())
    {
        mesh.vertexArray = it->second;
        return true;
    }

    auto va = LoadMesh2DFromBuilt(*rec);
    if (!va)
        return false;
    m_meshCache[key] = va;
    mesh.vertexArray = va;
    return true;
}

std::shared_ptr<Shader> AssetManager::LoadShader(const AssetId& id)
{
    if (!id.IsValid())
        return nullptr;
    std::string key = id.str();
    auto it = m_shaderCache.find(key);
    if (it != m_shaderCache.end())
        return it->second;

    const AssetRecord* rec = m_registry.FindByGuid(id);
    if (!rec || rec->type != AssetType::Shader)
        return nullptr;

    std::string absPath = ResolveBuiltAbsolute(*rec);
    std::string jsonStr;
    if (!absPath.empty())
    {
        std::ifstream file(absPath);
        if (file)
        {
            std::stringstream ss;
            ss << file.rdbuf();
            jsonStr = ss.str();
        }
    }
    if (jsonStr.empty() && m_pack)
    {
        for (size_t i = 0; i < m_pack->GetIndex().size(); ++i)
        {
            if (m_pack->GetIndex()[i].guid == id)
            {
                std::vector<uint8_t> blob;
                if (m_pack->ReadBlob(i, blob))
                    jsonStr.assign(reinterpret_cast<const char*>(blob.data()), blob.size());
                break;
            }
        }
    }

    if (jsonStr.empty())
        return nullptr;

    std::string err;
    json11::Json j = json11::Json::parse(jsonStr, err);
    if (!err.empty() || !j.is_object())
        return nullptr;
    const auto& o = j.object_items();
    auto itV = o.find("vertex");
    auto itF = o.find("fragment");
    if (itV == o.end() || itF == o.end() || !itV->second.is_string() || !itF->second.is_string())
        return nullptr;

    auto shader = Shader::Create(itV->second.string_value(), itF->second.string_value());
    if (!shader)
        return nullptr;
    auto shared = std::shared_ptr<Shader>(shader.release());
    m_shaderCache[key] = shared;
    return shared;
}

std::shared_ptr<Texture2DResource> AssetManager::LoadTexture2D(const AssetId& id)
{
    if (!id.IsValid())
        return nullptr;
    std::string key = id.str();
    auto it = m_textureCache.find(key);
    if (it != m_textureCache.end())
        return it->second;

    const AssetRecord* rec = m_registry.FindByGuid(id);
    if (!rec || rec->type != AssetType::Texture2D)
        return nullptr;

    std::vector<uint8_t> pixelsData;
    int w = 0, h = 0, channels = 0;

    std::string absPath = ResolveBuiltAbsolute(*rec);
    unsigned char* pixels = nullptr;
    if (!absPath.empty() && std::filesystem::exists(absPath))
        pixels = stbi_load(absPath.c_str(), &w, &h, &channels, 4);

    if (!pixels && m_pack)
    {
        for (size_t i = 0; i < m_pack->GetIndex().size(); ++i)
        {
            if (m_pack->GetIndex()[i].guid == id)
            {
                std::vector<uint8_t> blob;
                if (m_pack->ReadBlob(i, blob))
                    pixels = stbi_load_from_memory(blob.data(), static_cast<int>(blob.size()), &w, &h, &channels, 4);
                break;
            }
        }
    }

    if (!pixels)
        return nullptr;

    auto tex = std::make_shared<Texture2DResource>();
    glGenTextures(1, &tex->glTextureId);
    glBindTexture(GL_TEXTURE_2D, tex->glTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
    tex->width = w;
    tex->height = h;

    m_textureCache[key] = tex;
    return tex;
}

} // namespace FDE
