#include "FDE/pch.hpp"
#include "FDE/Asset/MeshImporter.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace FDE
{

bool MeshImporter::LoadFirstMesh(const std::filesystem::path& path, std::shared_ptr<VertexArray>& outVertexArray)
{
    outVertexArray.reset();

    Assimp::Importer importer;
    const unsigned flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals
                           | aiProcess_ImproveCacheLocality | aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(path.string(), flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        FDE_LOG_CLIENT_ERROR("Assimp: {}", importer.GetErrorString());
        return false;
    }

    const aiMesh* mesh = nullptr;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i)
    {
        if (scene->mMeshes[i] && scene->mMeshes[i]->mNumVertices > 0)
        {
            mesh = scene->mMeshes[i];
            break;
        }
    }
    if (!mesh)
    {
        FDE_LOG_CLIENT_ERROR("Assimp: no mesh in {}", path.string());
        return false;
    }

    struct Vertex
    {
        float px, py, pz;
        float r, g, b;
    };

    std::vector<Vertex> vertices(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& p = mesh->mVertices[i];
        vertices[i].px = p.x;
        vertices[i].py = p.y;
        vertices[i].pz = p.z;
        if (mesh->HasVertexColors(0))
        {
            const aiColor4D& c = mesh->mColors[0][i];
            vertices[i].r = c.r;
            vertices[i].g = c.g;
            vertices[i].b = c.b;
        }
        else
        {
            vertices[i].r = 1.0f;
            vertices[i].g = 1.0f;
            vertices[i].b = 1.0f;
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3u);
    for (unsigned f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices != 3u)
            continue;
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }
    if (indices.empty())
    {
        FDE_LOG_CLIENT_ERROR("Assimp: no triangle indices in {}", path.string());
        return false;
    }

    auto vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(Vertex));
    BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Color"}};
    outVertexArray = VertexArray::Create();
    if (!vbo || !outVertexArray)
        return false;
    outVertexArray->AddVertexBuffer(vbo, layout);
    outVertexArray->SetIndexBuffer(indices.data(), static_cast<uint32_t>(indices.size()));
    return true;
}

} // namespace FDE
