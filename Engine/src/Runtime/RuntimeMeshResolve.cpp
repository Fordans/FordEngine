#include "FDE/pch.hpp"
#include "FDE/Runtime/RuntimeMeshResolve.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Scene/World.hpp"

namespace FDE
{

namespace
{

void CreateTriangleMesh(std::shared_ptr<VertexArray>& outVAO)
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,
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

void ResolvePendingMeshes(World* world, AssetManager* assets)
{
    if (!world)
        return;
    AssetManager fallback;
    AssetManager* resolve3d = assets ? assets : &fallback;

    for (const std::string& name : world->GetSceneNames())
    {
        Scene* scene = world->GetScene(name);
        if (!scene)
            continue;
        auto view = scene->GetRegistry().view<Mesh2DComponent>();
        for (auto entity : view)
        {
            auto& mesh = view.get<Mesh2DComponent>(entity);
            if (mesh.vertexArray && mesh.vertexArray->GetIndexCount() > 0)
                continue;
            if (assets)
                assets->ResolveMesh2D(mesh);
            if (!mesh.vertexArray && mesh.meshAsset == "builtin:triangle")
            {
                std::shared_ptr<VertexArray> va;
                CreateTriangleMesh(va);
                mesh.vertexArray = va;
            }
        }

        Scene3D* scene3d = world->GetScene3D(name);
        if (!scene3d)
            continue;
        auto view3d = scene3d->GetRegistry().view<Mesh3DComponent>();
        for (auto entity : view3d)
        {
            auto& mesh = view3d.get<Mesh3DComponent>(entity);
            if (mesh.vertexArray && mesh.vertexArray->GetIndexCount() > 0)
                continue;
            resolve3d->ResolveMesh3D(mesh);
        }
    }
}

} // namespace FDE
