//
// Created by ilya on 16.03.24.
//

#include <SGUtils/Noise/PerlinNoise.h>
#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/Render/DisableMeshGeometryPass.h>
#include <SGCore/Render/SpacePartitioning/OctreeCullableInfo.h>
#include <SGCore/Render/SpacePartitioning/CullableMesh.h>
#include <SGCore/Transformations/Transform.h>
#include <SGCore/Scene/Layer.h>
#include <SGCore/Render/Batching/Batch.h>
#include <SGCore/Scene/EntityBaseInfo.h>
#include "ChunksManager.h"
#include "BlocksTypes.h"

void OceansEdge::ChunksManager::buildChunksGrid
(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed)
{
    auto& registry = scene->getECSRegistry();
    
    ulvec2 playerChunk = { std::floor(playerPosition.x / (Settings::s_chunksSize.x * 2.0f)), std::floor(playerPosition.z / (Settings::s_chunksSize.z * 2.0f)) };
    
    std::unordered_set<ulvec2, SGCore::MathUtils::GLMVectorHash<ulvec2>> chunksIndices;
    
    for(size_t x = 0; x < Settings::s_drawingRange; ++x)
    {
        for(size_t y = 0; y < Settings::s_drawingRange; ++y)
        {
            chunksIndices.insert({ x, y });
        }
    }
    
    decltype(m_chunks) newChunks;
    
    for(const auto& v : chunksIndices)
    {
        if(m_chunks.contains(v))
        {
            newChunks[v] = m_chunks[v];
        }
        else
        {
            newChunks[v] = registry.create();
            
            // create new valid chunk
            // todo:
            
            SGCore::PerlinNoise perlinNoise;
            // todo: make flexible settings
            perlinNoise.generate(v * ulvec2 { Settings::s_chunksSize.x, Settings::s_chunksSize.z }, { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 6, 0.6f);
        }
    }
    
    // delete not valid chunks
    for(auto& p : m_chunks)
    {
        if(!newChunks.contains(p.first))
        {
            registry.destroy(p.second);
        }
    }
    
    m_chunks = newChunks;
    
    SGCore::PerlinNoise perlinNoise;
    // todo: make flexible settings
    perlinNoise.generate({ Settings::s_chunksSize.x * Settings::s_drawingRange, Settings::s_chunksSize.z * Settings::s_drawingRange}, 6, 0.6f);
    
    const size_t totalSurfaceBlocksInChunkCnt = Settings::s_chunksSize.x * Settings::s_chunksSize.z * 2;
    
    for(size_t cx = 0; cx < Settings::s_drawingRange; ++cx)
    {
        for(size_t cz = 0; cz < Settings::s_drawingRange; ++cz)
        {
            entt::entity chunkEntity = registry.create();
            
            SGCore::Batch& chunkBatch = registry.emplace<SGCore::Batch>(chunkEntity, scene,
                                                                        totalSurfaceBlocksInChunkCnt * 36,
                                                                        totalSurfaceBlocksInChunkCnt);
            auto chunkTransform = registry.emplace<SGCore::Ref<SGCore::Transform>>(chunkEntity, SGCore::MakeRef<SGCore::Transform>());
            
            chunkTransform->m_ownTransform.m_position.x = cx * Settings::s_chunksSize.x * 2;
            chunkTransform->m_ownTransform.m_position.z = cz * Settings::s_chunksSize.z * 2;
            
            for(std::uint32_t x = 0; x < Settings::s_chunksSize.x; ++x)
            {
                for(std::uint32_t z = 0; z < Settings::s_chunksSize.z; ++z)
                {
                    float rawY = perlinNoise.m_map.get(cx * Settings::s_chunksSize.x + x, cz * Settings::s_chunksSize.z + z);
                    float y = std::floor(((rawY * 50)) / 2.0f) * 2.0f;
                    
                    entt::entity blockEntity = BlocksTypes::getBlockTypeMeta(BlocksTypes::OEB_MUD_WITH_GRASS
                    ).m_meshData->addOnScene(scene, SG_LAYER_OPAQUE_NAME);
                    registry.emplace<SGCore::DisableMeshGeometryPass>(blockEntity);
                    registry.remove<SGCore::Ref<SGCore::OctreeCullableInfo>>(blockEntity);
                    registry.remove<SGCore::Ref<SGCore::CullableMesh>>(blockEntity);
                    
                    chunkBatch.addEntity(blockEntity);
                    
                    auto blockTransform = registry.get<SGCore::Ref<SGCore::Transform>>(
                            blockEntity
                    );
                    
                    SGCore::EntityBaseInfo& blockEntityBaseInfo = registry.get<SGCore::EntityBaseInfo>(blockEntity);
                    blockEntityBaseInfo.m_parent = chunkEntity;
                    // blockTransform->m_ownTransform.m_position.x = cx * GameGlobals::s_chunksSize.x * 2 + (float) x * 2;
                    blockTransform->m_ownTransform.m_position.x = (float) x * 2;
                    blockTransform->m_ownTransform.m_position.y = y;
                    // blockTransform->m_ownTransform.m_position.y = 0;
                    // blockTransform->m_ownTransform.m_position.z = cz * GameGlobals::s_chunksSize.z * 2 + (float) z * 2;
                    blockTransform->m_ownTransform.m_position.z = (float) z * 2;
                    // blockTransform->m_ownTransform.m_position.z = 0.0;
                    
                }
            }
        }
    }
}

std::unordered_map<OceansEdge::ulvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<OceansEdge::ulvec2>>& OceansEdge::ChunksManager::getChunks() noexcept
{
    return m_chunks;
}
