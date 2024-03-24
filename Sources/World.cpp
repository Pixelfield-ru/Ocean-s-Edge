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
                        chunk->m_blocks.get(bx, by, bz) = { };
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
    
    // std::vector<float> blocksPositions;
    
    for(const auto& p : tmpOccupiedIndices)
    {
        if(!m_lastOccupiedIndices.contains(p) && !m_freeChunksEntities.empty())
        {
            auto fIt = m_freeChunksEntities.begin();
            
            const auto& chunk = *fIt;
            
            lvec2 chunkIdx = p;
            
            const glm::vec3 chunkPosition = { chunkIdx.x * Settings::s_chunksSize.x * 2, 0, chunkIdx.y * Settings::s_chunksSize.z * 2 };
            
            SGCore::PerlinNoise perlinNoise;
            // todo: make flexible settings
            perlinNoise.generate((lvec2 { chunkIdx.x, chunkIdx.y } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 },
                                 { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);
            
            const long endX = Settings::s_chunksSize.x;
            const long endZ = Settings::s_chunksSize.z;
            
            chunk->m_vertices.clear();
            chunk->m_indices.clear();
            chunk->m_currentIndex = 0;
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    float rawY = perlinNoise.m_map.get(x, z);
                    float by = std::floor(((rawY * 50)) / (Settings::s_blockHalfSize.y * 2.0)) * Settings::s_blockHalfSize.y * 2.0;
                    
                    const long endY = Settings::s_chunksSize.y / 2 + by;
                    
                    for(long y = 0; y < endY; ++y)
                    {
                        auto& blockData = chunk->m_blocks.get(x, y, z);
                        
                        glm::vec3 blockPos = { chunkPosition.x + x * Settings::s_blockHalfSize.x * 2,
                                              by,
                                              chunkPosition.z + z * Settings::s_blockHalfSize.z * 2 };
                        
                        // blockData.m_type = BlocksTypes::OEB_MUD_WITH_GRASS;
                        
                        // blocksPositions.push_back(blockData.m_position.x);
                        // blocksPositions.push_back(blockData.m_position.y);
                        // blocksPositions.push_back(blockData.m_position.z);
                        
                        if(y == endY - 1)
                        {
                            addBlockTopSideVertices(blockPos, chunk);
                        }
                        if(z == 0)
                        {
                            addBlockBackSideVertices(blockPos, chunk);
                        }
                        if(z == endZ - 1)
                        {
                            addBlockFaceSideVertices(blockPos, chunk);
                        }
                        if(x == 0)
                        {
                            addBlockLeftSideVertices(blockPos, chunk);
                        }
                        if(x == endX - 1)
                        {
                            addBlockRightSideVertices(blockPos, chunk);
                        }
                    }
                }
            }
            
            chunk->m_needsSubData = true;
            
            // std::cout << "polygons count : " << (chunk->m_polygons.size() / 3) << std::endl;
            
            m_lastOccupiedIndices.emplace(p, chunk);
            
            fIt = m_freeChunksEntities.erase(fIt);
        }
    }
}

void OceansEdge::World::addBlockTopSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::addBlockBottomSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::addBlockFaceSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::addBlockBackSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::addBlockLeftSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x - Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::addBlockRightSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    glm::vec3 ld = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 lt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z - Settings::s_blockHalfSize.z };
    glm::vec3 rt = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y + Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    glm::vec3 rd = { blockPos.x + Settings::s_blockHalfSize.x, blockPos.y - Settings::s_blockHalfSize.y, blockPos.z + Settings::s_blockHalfSize.z };
    
    chunk->m_vertices.push_back(ld.x);
    chunk->m_vertices.push_back(ld.y);
    chunk->m_vertices.push_back(ld.z);
    
    chunk->m_vertices.push_back(lt.x);
    chunk->m_vertices.push_back(lt.y);
    chunk->m_vertices.push_back(lt.z);
    
    chunk->m_vertices.push_back(rt.x);
    chunk->m_vertices.push_back(rt.y);
    chunk->m_vertices.push_back(rt.z);
    
    chunk->m_vertices.push_back(rd.x);
    chunk->m_vertices.push_back(rd.y);
    chunk->m_vertices.push_back(rd.z);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 6;
}

void OceansEdge::World::render(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    for(const auto& chunk : m_chunks)
    {
        chunk.second->render(scene);
    }
}
