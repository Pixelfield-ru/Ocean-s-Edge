//
// Created by ilya on 24.03.24.
//
#include "Chunk.h"
#include "Resources.h"
#include "GameMain.h"

#include <SGCore/Graphics/API/IVertexArray.h>
#include <SGCore/Graphics/API/IVertexBuffer.h>
#include <SGCore/Graphics/API/IIndexBuffer.h>
#include <SGCore/Graphics/API/IRenderer.h>
#include <SGCore/Graphics/API/IFrameBuffer.h>
#include <SGCore/Graphics/API/IVertexBufferLayout.h>
#include <SGCore/Main/CoreMain.h>
#include <SGCore/Render/RenderPipelinesManager.h>
#include <SGCore/Memory/AssetManager.h>
#include <SGCore/Scene/Scene.h>
#include <SGCore/Render/Camera3D.h>
#include <SGCore/Transformations/Transform.h>
#include <SGCore/Render/Atmosphere/AtmosphereUpdater.h>

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
        
        m_positionsVertexBuffer->setUsage(SGG_DYNAMIC)->create(m_maxVerticesCount * 2 * sizeof(int))->bind();
        
        bufferLayout->reset()
                ->addAttribute(Ref<IVertexAttribute>(
                        bufferLayout->createVertexAttribute(0,
                                                            "vertexData",
                                                            SGG_INT2,
                                                            (size_t) 0))
                )
                ->prepare()->enableAttributes();
        
        m_indicesBuffer = Ref<IIndexBuffer>(CoreMain::getRenderer()->createIndexBuffer());
        m_indicesBuffer->setUsage(SGG_DYNAMIC)->create(m_maxIndicesCount * sizeof(int));
    }
    
    m_renderInfo.m_useIndices = true;
    m_renderInfo.m_enableFacesCulling = false;
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
    
    size_t verticesCount = 0;
    size_t indicesCount = 0;
    
    {
        std::lock_guard buffersGuard(m_buffersChangeMutex);
        
        verticesCount = std::min<size_t>(m_vertices.size() / 2, m_maxVerticesCount);
        indicesCount = std::min<size_t>(m_indices.size(), m_maxIndicesCount);
    }
    
    subPassShader->bind();
    
    LayeredFrameReceiver& playerCameraReceiver = GameMain::getCurrentWorldScene()->getECSRegistry()->get<LayeredFrameReceiver>(
            GameMain::getCurrentWorld()->getPlayerEntity());
    playerCameraReceiver.getLayer(
            "chunks_layer")->m_frameBuffer->getAttachment(SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT5)->bind(0);
    subPassShader->useTextureBlock("SSAOLayerColor", 0);
    
    // m_vertexArray->bind();
    
    subPassShader->useUniformBuffer(CoreMain::getRenderer()->m_viewMatricesBuffer);
    
    auto atmosphereUpdater = scene->getSystem<AtmosphereUpdater>();
    
    if(atmosphereUpdater)
    {
        subPassShader->useUniformBuffer(atmosphereUpdater->m_uniformBuffer);
    }
    
    glm::vec3 chunkPos;
    chunkPos.x = m_position.x;
    chunkPos.y = m_position.y;
    chunkPos.z = m_position.z;
    
    subPassShader->useVectorf("u_chunkPosition", chunkPos);
    Resources::getBlocksAtlas()->bind(1);
    subPassShader->useTextureBlock("u_blocksAtlas", 1);
    
    CoreMain::getRenderer()->renderArray(m_vertexArray, m_renderInfo,
                                         verticesCount,
                                         indicesCount);
    
    if(m_needsSubData)
    {
        std::lock_guard buffersGuard(m_buffersChangeMutex);
        
        if(m_vertices.size() >= verticesCount * 2)
        {
            m_positionsVertexBuffer->bind();
            m_positionsVertexBuffer->subData(m_vertices.data(), verticesCount * 2, 0);
        }
        
        if(m_indices.size() >= indicesCount)
        {
            m_indicesBuffer->bind();
            m_indicesBuffer->subData(m_indices.data(), indicesCount, 0);
        }
    }
    
    m_needsSubData = false;
}

void OceansEdge::Chunk::onRenderPipelineSet() noexcept
{
    auto renderPipeline = SGCore::RenderPipelinesManager::getCurrentRenderPipeline();
    
    if(!renderPipeline) return;
    
    // std::cout << Settings::getShadersPaths()["ChunkShader"].m_GLSL4RealizationPath << std::endl;
    
    m_shader->addSubPassShadersAndCompile(SGCore::AssetManager::loadAsset<SGCore::TextFileAsset>(
            Settings::getShadersPaths()["ChunkShader"].getCurrentRealization()));
}

bool OceansEdge::Chunk::isBuffersHaveFreeSpace() noexcept
{
    std::lock_guard buffersGuard(m_buffersChangeMutex);
    
    bool val = m_vertices.size() / 2 + 4 <= m_maxVerticesCount &&
               m_indices.size() <= m_maxIndicesCount;
    
    
    return val;
}

