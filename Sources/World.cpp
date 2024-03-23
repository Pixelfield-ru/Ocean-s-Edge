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
#include "World.h"
#include "BlocksTypes.h"
#include "Chunk.h"
#include "BlockData.h"

void OceansEdge::World::prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    auto& registry = scene->getECSRegistry();
    
    const size_t totalSurfaceBlocksInChunkCnt = Settings::s_chunksSize.x * Settings::s_chunksSize.z * 2;
    
    size_t curChunk = 0;
    size_t blockIdx = 0;
    for(long x = -Settings::s_drawingRange / 2; x < Settings::s_drawingRange / 2; ++x)
    {
        for(long y = -Settings::s_drawingRange / 2; y < Settings::s_drawingRange / 2; ++y)
        {
            auto chunk = SGCore::MakeRef<Chunk>();
            m_chunks[{ x, y }] = chunk;
            
            chunk->m_blocks = Chunk::blocks_container_t(Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z);
            // chunk->m_blocks = new BlockData**[Settings::s_chunksSize.x];
            
            /*auto& chunkBatch = registry.emplace<SGCore::Batch>(chunkEntity, scene,
                                                               totalSurfaceBlocksInChunkCnt * 36,
                                                               totalSurfaceBlocksInChunkCnt);*/
            
            for(std::uint32_t bx = 0; bx < Settings::s_chunksSize.x; ++bx)
            {
                // chunk->m_blocks[bx] = new BlockData*[Settings::s_chunksSize.y];
                
                for(std::uint32_t by = 0; by < Settings::s_chunksSize.y; ++by)
                {
                    // chunk->m_blocks[bx][by] = new BlockData[Settings::s_chunksSize.z];
                    for(std::uint32_t bz = 0; bz < Settings::s_chunksSize.z; ++bz)
                    {
                        //
                        //  chunk->m_blocks[bx][by][bz] = { };
                        // chunk->m_blocks[{ bx, by, bz }] = SGCore::MakeRef<BlockData>();
                    }
                }
                
                /*auto blockEntity = BlocksTypes::getBlockTypeMeta(BlocksTypes::OEB_MUD_WITH_GRASS)
                        .m_meshData->addOnScene(scene, SG_LAYER_OPAQUE_NAME);
                registry.emplace<SGCore::DisableMeshGeometryPass>(blockEntity);
                registry.remove<SGCore::Ref<SGCore::OctreeCullableInfo>>(blockEntity);
                registry.remove<SGCore::Ref<SGCore::CullableMesh>>(blockEntity);*/
                
                /*chunkEntityChunk.m_blocks[{ bx, bz }] = blockEntity;
                
                chunkBatch.addEntity(blockEntity);
                
                SGCore::EntityBaseInfo& blockEntityBaseInfo = registry.get<SGCore::EntityBaseInfo>(blockEntity);
                blockEntityBaseInfo.m_parent = chunkEntity;*/
            }
            
            // m_chunksEntities[curChunk] = chunkEntity;
            ++curChunk;
            m_freeChunksEntities.insert(chunk);
            // m_lastOccupiedIndices[{ x, y }] = chunkEntity;
        }
    }
    
    std::cout << "created" << std::endl;
}

void OceansEdge::World::buildChunksGrid
(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed)
{
    auto& registry = scene->getECSRegistry();
    
    lvec2 playerChunk = { std::floor(playerPosition.x / (Settings::s_chunksSize.x * 2.0f)), std::floor(playerPosition.z / (Settings::s_chunksSize.z * 2.0f)) };
    
    std::unordered_set<lvec2, SGCore::MathUtils::GLMVectorHash<lvec2>> tmpOccupiedIndices;
    
    size_t curEntityIdx = 0;
    for(long cx = playerChunk.x - Settings::s_drawingRange / 2; cx < playerChunk.x + Settings::s_drawingRange / 2; ++cx)
    {
        for(long cy = playerChunk.y - Settings::s_drawingRange / 2; cy < playerChunk.y + Settings::s_drawingRange / 2; ++cy)
        {
            tmpOccupiedIndices.insert(lvec2 { cx, cy });
        }
    }
    
    /*auto tmp = m_lastOccupiedIndices;
    m_lastOccupiedIndices.clear();*/
    
    auto ocIt = m_lastOccupiedIndices.begin();
    while(ocIt != m_lastOccupiedIndices.end())
    {
        if(tmpOccupiedIndices.contains(ocIt->first))
        {
            m_freeChunksEntities.erase(ocIt->second);
            ++ocIt;
        }
        else
        {
            m_freeChunksEntities.insert(ocIt->second);
            ocIt = m_lastOccupiedIndices.erase(ocIt);
        }
    }
    
    for(const auto& p : tmpOccupiedIndices)
    {
        if(!m_lastOccupiedIndices.contains(p) && !m_freeChunksEntities.empty())
        {
            auto fIt = m_freeChunksEntities.begin();
            
            const auto& chunk = *fIt;
            
            lvec2 chunkIdx = p;
            
            // chunkTransform->m_ownTransform.m_position.x = (chunkIdx.x * Settings::s_chunksSize.x * 2);
            // chunkTransform->m_ownTransform.m_position.z = (chunkIdx.y * Settings::s_chunksSize.z * 2);
            
            SGCore::PerlinNoise perlinNoise;
            // todo: make flexible settings
            perlinNoise.generate((lvec2 { chunkIdx.x, chunkIdx.y } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 },
                                 { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);
            // perlinNoise.generate({ Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);
            
            for(long x = 0; x < Settings::s_chunksSize.x; ++x)
            {
                for(long z = 0; z < Settings::s_chunksSize.z; ++z)
                {
                    float rawY = perlinNoise.m_map.get(x, z);
                    float by = std::floor(((rawY * 50)) / 2.0f) * 2.0f;
                    
                    for(long y = 0; y < Settings::s_chunksSize.y / 2 + by; ++y)
                    {
                        const auto& blockData = chunk->m_blocks.get(x, y, z);
                        
                        /*blockData->m_position.x = x * 2;
                        blockData->m_position.y = by;
                        blockData->m_position.z = z * 2;*/
                    }
                }
            }
            
            m_lastOccupiedIndices.emplace(p, chunk);
            
            fIt = m_freeChunksEntities.erase(fIt);
        }
    }
    
    /*long curChunkBitFlag = 1;
    size_t curChunk = 0;
    for(long cx = playerChunk.x - Settings::s_drawingRange / 2; cx < playerChunk.x + Settings::s_drawingRange / 2; ++cx)
    {
        for(long cy = playerChunk.y - Settings::s_drawingRange / 2; cy < playerChunk.y + Settings::s_drawingRange / 2; ++cy)
        {
            const size_t hashedChunkIndices = SGCore::MathUtils::hashVector(lvec2 { cx, cy });
            
            if(!m_chunksHashes.contains(hashedChunkIndices))
            {
                // std::cout << "m_chunksMask: " << m_chunksMask.m_flags << std::endl;
                
                const auto& chunkEntity = m_chunksEntities[curChunk];
                
                auto chunkTransform = registry.get<SGCore::Ref<SGCore::Transform>>(chunkEntity);
                Chunk& chunkEntityChunk = registry.get<Chunk>(chunkEntity);
                
                chunkTransform->m_ownTransform.m_position.x = (cx * Settings::s_chunksSize.x * 2);
                chunkTransform->m_ownTransform.m_position.z = (cy * Settings::s_chunksSize.z * 2);
                
                SGCore::PerlinNoise perlinNoise;
                // todo: make flexible settings
                perlinNoise.generate((lvec2 { cx, cy } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 }, { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);
                // perlinNoise.generate({ Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);
                
                for(long x = 0; x < Settings::s_chunksSize.x; ++x)
                {
                    for(long z = 0; z < Settings::s_chunksSize.z; ++z)
                    {
                        float rawY = perlinNoise.m_map.get(x, z);
                        float y = std::floor(((rawY * 50)) / 2.0f) * 2.0f;
                        
                        const auto& blockEntity = chunkEntityChunk.m_blocks[{ x, z }];
                        
                        auto blockTransform = registry.get<SGCore::Ref<SGCore::Transform>>(blockEntity);
                        
                        blockTransform->m_ownTransform.m_position.x = x * 2;
                        blockTransform->m_ownTransform.m_position.y = y;
                        blockTransform->m_ownTransform.m_position.z = z * 2;
                    }
                }
                
                m_chunksHashes.insert(hashedChunkIndices);
            }
            
            // curChunkBitFlag >>= 1;
            ++curChunk;
        }
    }*/
    
    // m_lastPlayerChunk = playerChunk;
    
    /*SGCore::PerlinNoise perlinNoise;
    // todo: make flexible settings
    perlinNoise.generate({ Settings::s_chunksSize.x * Settings::s_drawingRange, Settings::s_chunksSize.z * Settings::s_drawingRange}, 6, 0.6f);
    
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
    }*/
}