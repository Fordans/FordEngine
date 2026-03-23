#include "FDE/pch.hpp"
#include "FDE/Asset/MeshImporter.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cctype>
#include <cfloat>
#include <glm/glm.hpp>
#include <string_view>

namespace FDE
{

namespace
{

struct Vertex
{
    float px, py, pz;
    float r, g, b;
    float u, v;
};

glm::mat4 AiMatrix4x4ToGlm(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
}

std::string LowerExtension(const std::filesystem::path& path)
{
    std::string e = path.extension().string();
    for (char& c : e)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return e;
}

std::string NormalizeAssimpHint(std::string_view hint)
{
    if (hint.empty())
        return ".obj";
    std::string h(hint);
    for (char& c : h)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (h.front() != '.')
        h.insert(h.begin(), '.');
    return h;
}

unsigned ImportFlagsForExtension(std::string_view extWithDot)
{
    unsigned flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
                     | aiProcess_GenSmoothNormals | aiProcess_ForceGenNormals | aiProcess_FlipUVs;
    if (extWithDot == ".3ds")
        flags |= aiProcess_FlipWindingOrder;
    return flags;
}

bool MeshUsesVertexColors(const aiMesh* mesh)
{
    if (!mesh->HasVertexColors(0) || mesh->mNumVertices == 0)
        return false;
    glm::vec3 cmin(FLT_MAX);
    glm::vec3 cmax(-FLT_MAX);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiColor4D& c = mesh->mColors[0][i];
        const glm::vec3 rgb(c.r, c.g, c.b);
        cmin = glm::min(cmin, rgb);
        cmax = glm::max(cmax, rgb);
    }
    const glm::vec3 span = cmax - cmin;
    const float spread = glm::max(span.x, glm::max(span.y, span.z));
    return spread > 0.08f;
}

void AppendMesh(const aiMesh* mesh, const glm::mat4& world, const glm::vec3& lightDir, const glm::vec3& baseColor,
                std::vector<Vertex>& outVerts, std::vector<uint32_t>& outIndices, glm::vec3& bmin, glm::vec3& bmax)
{
    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    const bool useVertexColors = MeshUsesVertexColors(mesh);

    const glm::mat3 world3(world);
    glm::mat3 normalMat(1.f);
    const float det = glm::determinant(world3);
    if (std::abs(det) > 1e-12f)
        normalMat = glm::transpose(glm::inverse(world3));

    outVerts.resize(outVerts.size() + mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex& v = outVerts[base + i];
        const aiVector3D& p = mesh->mVertices[i];
        const glm::vec4 posW = world * glm::vec4(p.x, p.y, p.z, 1.f);
        v.px = posW.x;
        v.py = posW.y;
        v.pz = posW.z;

        bmin = glm::min(bmin, glm::vec3(v.px, v.py, v.pz));
        bmax = glm::max(bmax, glm::vec3(v.px, v.py, v.pz));

        if (useVertexColors)
        {
            const aiColor4D& c = mesh->mColors[0][i];
            v.r = c.r;
            v.g = c.g;
            v.b = c.b;
        }
        else if (mesh->HasNormals())
        {
            const aiVector3D& n = mesh->mNormals[i];
            glm::vec3 N = normalMat * glm::vec3(n.x, n.y, n.z);
            if (glm::dot(N, N) > 1e-10f)
                N = glm::normalize(N);
            const float ndl = glm::max(0.f, glm::dot(N, lightDir));
            const float shade = 0.22f + 0.78f * ndl;
            v.r = baseColor.r * shade;
            v.g = baseColor.g * shade;
            v.b = baseColor.b * shade;
        }
        else
        {
            v.r = baseColor.r;
            v.g = baseColor.g;
            v.b = baseColor.b;
        }

        if (mesh->HasTextureCoords(0))
        {
            const aiVector3D& t = mesh->mTextureCoords[0][i];
            v.u = t.x;
            v.v = t.y;
        }
        else
        {
            v.u = 0.f;
            v.v = 0.f;
        }
    }

    for (unsigned f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices != 3u)
            continue;
        outIndices.push_back(base + face.mIndices[0]);
        outIndices.push_back(base + face.mIndices[1]);
        outIndices.push_back(base + face.mIndices[2]);
    }
}

void ProcessNode(const aiScene* scene, const aiNode* node, const glm::mat4& parentWorld, const glm::vec3& lightDir,
                 const glm::vec3& baseColor, std::vector<Vertex>& outVerts, std::vector<uint32_t>& outIndices,
                 glm::vec3& bmin, glm::vec3& bmax)
{
    const glm::mat4 world = parentWorld * AiMatrix4x4ToGlm(node->mTransformation);
    for (unsigned i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh && mesh->mNumVertices > 0)
            AppendMesh(mesh, world, lightDir, baseColor, outVerts, outIndices, bmin, bmax);
    }
    for (unsigned c = 0; c < node->mNumChildren; ++c)
        ProcessNode(scene, node->mChildren[c], world, lightDir, baseColor, outVerts, outIndices, bmin, bmax);
}

bool BuildMeshFromScene(const aiScene* scene, const char* logPathOrNull, std::shared_ptr<VertexArray>& outVertexArray,
                        glm::vec3* outLocalMin, glm::vec3* outLocalMax)
{
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        return false;

    const glm::vec3 lightDir = glm::normalize(glm::vec3(0.45f, 0.85f, 0.35f));
    const glm::vec3 baseColor(0.72f, 0.70f, 0.68f);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 bmin(FLT_MAX);
    glm::vec3 bmax(-FLT_MAX);

    ProcessNode(scene, scene->mRootNode, glm::mat4(1.f), lightDir, baseColor, vertices, indices, bmin, bmax);

    if (vertices.empty() || indices.empty())
    {
        if (logPathOrNull)
            FDE_LOG_CLIENT_ERROR("Assimp: no mesh geometry in {}", logPathOrNull);
        else
            FDE_LOG_CLIENT_ERROR("Assimp: no mesh geometry in memory buffer");
        return false;
    }

    if (outLocalMin)
        *outLocalMin = bmin;
    if (outLocalMax)
        *outLocalMax = bmax;

    auto vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(Vertex));
    BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Color"},
                           {ShaderDataType::Float2, "a_TexCoord"}};
    outVertexArray = VertexArray::Create();
    if (!vbo || !outVertexArray)
        return false;
    outVertexArray->AddVertexBuffer(vbo, layout);
    outVertexArray->SetIndexBuffer(indices.data(), static_cast<uint32_t>(indices.size()));
    return true;
}

} // namespace

bool MeshImporter::LoadSceneMeshesMerged(const std::filesystem::path& path, std::shared_ptr<VertexArray>& outVertexArray,
                                         glm::vec3* outLocalMin, glm::vec3* outLocalMax)
{
    outVertexArray.reset();

    Assimp::Importer importer;
    const std::string ext = LowerExtension(path);
    const unsigned flags = ImportFlagsForExtension(ext);

    const aiScene* scene = importer.ReadFile(path.string(), flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        FDE_LOG_CLIENT_ERROR("Assimp: {}", importer.GetErrorString());
        return false;
    }

    return BuildMeshFromScene(scene, path.string().c_str(), outVertexArray, outLocalMin, outLocalMax);
}

bool MeshImporter::LoadSceneMeshesMergedFromMemory(const void* data, size_t dataSize, std::string_view formatHint,
                                                   std::shared_ptr<VertexArray>& outVertexArray, glm::vec3* outLocalMin,
                                                   glm::vec3* outLocalMax)
{
    outVertexArray.reset();
    if (!data || dataSize == 0)
        return false;

    const std::string hint = NormalizeAssimpHint(formatHint);
    Assimp::Importer importer;
    const unsigned flags = ImportFlagsForExtension(hint);

    const aiScene* scene = importer.ReadFileFromMemory(data, dataSize, flags, hint.c_str());
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        FDE_LOG_CLIENT_ERROR("Assimp (memory): {}", importer.GetErrorString());
        return false;
    }

    return BuildMeshFromScene(scene, nullptr, outVertexArray, outLocalMin, outLocalMax);
}

} // namespace FDE
