//
// Created by ilya on 24.03.24.
//
#include "Chunk.h"

#include <SGCore/Graphics/API/IVertexArray.h>
#include <SGCore/Graphics/API/IVertexBuffer.h>
#include <SGCore/Graphics/API/IIndexBuffer.h>
#include <SGCore/Graphics/API/IRenderer.h>
#include <SGCore/Graphics/API/IVertexBufferLayout.h>
#include <SGCore/Main/CoreMain.h>
#include <SGCore/Render/RenderPipelinesManager.h>
#include <SGCore/Memory/AssetManager.h>
#include <SGCore/Scene/Scene.h>
#include <SGCore/Render/Camera3D.h>
#include <SGCore/Transformations/Transform.h>

OceansEdge::Chunk::Chunk()
{
    using namespace SGCore;
    
    m_vertexArray = Ref<IVertexArray>(CoreMain::getRenderer()->createVertexArray());
    m_vertexArray->create()->bind();
    
    {
        Ref<IVertexBufferLayout> bufferLayout = Ref<IVertexBufferLayout>(
                CoreMain::getRenderer()->createVertexBufferLayout());
        
        m_positionsVertexBuffer = Ref<IVertexBuffer>(
                CoreMain::getRenderer()->createVertexBuffer()
        );
        
        m_positionsVertexBuffer->setUsage(SGG_STATIC)->create(m_maxVerticesCount * 3 * sizeof(m_maxVerticesCount))->bind();
        
        bufferLayout->reset()
                ->addAttribute(Ref<IVertexAttribute>(
                        bufferLayout->createVertexAttribute(0,
                                                            "vertexPosition",
                                                            SGG_FLOAT3,
                                                            (size_t) 0))
                )
                ->prepare()->enableAttributes();
        
        m_indicesBuffer = Ref<IIndexBuffer>(CoreMain::getRenderer()->createIndexBuffer());
        m_indicesBuffer->setUsage(SGG_DYNAMIC)->create(m_maxIndicesCount * sizeof(m_maxIndicesCount));
    }
    
    m_renderInfo.m_useIndices = true;
    m_renderInfo.m_enableFacesCulling = true;
    m_renderInfo.m_drawMode = SGDrawMode::SGG_TRIANGLES;
    
    RenderPipelinesManager::subscribeToRenderPipelineSetEvent(m_onRenderPipelineSetEventListener);
    
    m_shader = MakeRef<IShader>();
    
    onRenderPipelineSet();
}

void OceansEdge::Chunk::render(const SGCore::Ref<SGCore::Scene>& scene)
{
    using namespace SGCore;
    
    auto& registry = scene->getECSRegistry();
    
    if(!m_shader) return;
    
    auto subPassShader = m_shader->getSubPassShader("ChunksPass");
    
    if(!subPassShader) return;
    
    size_t indicesCount = std::min<size_t>(m_indices.size(), m_maxIndicesCount);
    size_t verticesCount = std::min<size_t>(m_vertices.size(), m_maxVerticesCount * 3) / 3;
    
    subPassShader->bind();
    
    if(m_needsSubData && m_vertices.size() >= verticesCount * 3)
    {
        m_positionsVertexBuffer->bind();
        m_positionsVertexBuffer->subData(m_vertices.data(), verticesCount * 3, 0);
    }
    
    if(m_needsSubData && m_indices.size() >= indicesCount)
    {
        m_indicesBuffer->bind();
        m_indicesBuffer->subData(m_indices.data(), indicesCount, 0);
    }
    
    auto camerasView = registry.view<Ref<Camera3D>, Ref<RenderingBase>, Ref<Transform>>();
    
    camerasView.each([&verticesCount, &indicesCount, &subPassShader, this](Ref<Camera3D> camera3D,
            Ref<RenderingBase> renderingBase, Ref<Transform> transform) {
        CoreMain::getRenderer()->prepareUniformBuffers(renderingBase, transform);
        
        subPassShader->useUniformBuffer(CoreMain::getRenderer()->m_viewMatricesBuffer);
        
        // std::cout << "verticesCount : " << verticesCount << ", indicesCount : " << indicesCount << std::endl;
        
        CoreMain::getRenderer()->renderArray(m_vertexArray, m_renderInfo,
                                             verticesCount,
                                             indicesCount);
    });
    
    m_needsSubData = false;
}

void OceansEdge::Chunk::onRenderPipelineSet() noexcept
{
    using namespace SGCore;
    
    auto renderPipeline = RenderPipelinesManager::getCurrentRenderPipeline();
    
    if(!renderPipeline) return;
    
    // std::cout << Settings::getShadersPaths()["ChunkShader"].m_GLSL4RealizationPath << std::endl;
    
    m_shader->addSubPassShadersAndCompile(AssetManager::loadAsset<FileAsset>(
            Settings::getShadersPaths()["ChunkShader"].getCurrentRealization()));
}

