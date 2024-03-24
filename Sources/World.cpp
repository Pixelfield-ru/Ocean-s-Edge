//
// Created by ilya on 16.03.24.
//

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
                        chunk->m_blocks[bx][by][bz] = { };
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
    
    lvec2 playerChunk = { std::floor(playerPosition.x / (Settings::s_chunksSize.x)), std::floor(playerPosition.z / (Settings::s_chunksSize.z)) };
    
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
            
            auto chunk = *fIt;
            
            lvec2 chunkIdx = p;
            
            const glm::vec3 chunkPosition = { chunkIdx.x * Settings::s_chunksSize.x, 0, chunkIdx.y * Settings::s_chunksSize.z };
            chunk->m_position = { chunkPosition.x, 0, chunkPosition.z };
            
            // todo: make flexible settings
            /*perlinNoise.generate((lvec2 { chunkIdx.x, chunkIdx.y } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 },
                                 { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);*/
            
            const long endX = Settings::s_chunksSize.x;
            const long endZ = Settings::s_chunksSize.z;
            
            chunk->m_vertices.clear();
            chunk->m_indices.clear();
            chunk->m_currentIndex = 0;
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    float rawY = m_perlinNoise.octave2D_01((chunkPosition.x + x) * 0.03, (chunkPosition.z + z) * 0.03, 1);
                    float by = std::floor(rawY * 50);
                    
                    const long endY = std::min<long>(std::ceil(Settings::s_chunksSize.y / 2) + by, Settings::s_chunksSize.y);
                    
                    for(long y = 0; y < endY; ++y)
                    {
                        auto& blockData = chunk->m_blocks[x][y][z];
                        
                        blockData.m_type = BlocksTypes::OEB_MUD_WITH_GRASS;
                    }
                }
            }
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    float rawY = m_perlinNoise.octave2D_01((chunkPosition.x + x) * 0.03, (chunkPosition.z + z) * 0.03, 1);
                    float by = std::floor(rawY * 50);
                    
                    const long endY = std::min<long>(std::ceil(Settings::s_chunksSize.y / 2) + by, Settings::s_chunksSize.y);
                    
                    for(long y = 0; y < endY; ++y)
                    {
                        long px = std::min(endX - 1, x + 1);
                        long mx = std::max<long>(0, x - 1);
                        long pz = std::min(endZ - 1, z + 1);
                        long mz = std::max<long>(0, z - 1);
                        long py = std::min(endY - 1, y + 1);
                        long my = std::max<long>(0, y - 1);
                        
                        vec3_8 blockPos = { x, y, z };
                        
                        const auto& pxBD = chunk->m_blocks[px][y][z];
                        const auto& mxBD = chunk->m_blocks[mx][y][z];
                        const auto& pyBD = chunk->m_blocks[x][py][z];
                        const auto& myBD = chunk->m_blocks[x][my][z];
                        const auto& pzBD = chunk->m_blocks[x][y][pz];
                        const auto& mzBD = chunk->m_blocks[x][y][mz];
                        
                        if(pyBD.m_type == BlocksTypes::OEB_AIR || y == endY - 1)
                        {
                            addBlockTopSideVertices(blockPos, chunk);
                        }
                        if(pxBD.m_type == BlocksTypes::OEB_AIR || x == endX - 1 )
                        {
                            addBlockRightSideVertices(blockPos, chunk);
                        }
                        if(mxBD.m_type == BlocksTypes::OEB_AIR || x == 0)
                        {
                            addBlockLeftSideVertices(blockPos, chunk);
                        }
                        if(pzBD.m_type == BlocksTypes::OEB_AIR || z == endZ - 1)
                        {
                            addBlockFaceSideVertices(blockPos, chunk);
                        }
                        if(mzBD.m_type == BlocksTypes::OEB_AIR || z == 0)
                        {
                            addBlockBackSideVertices(blockPos, chunk);
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

void OceansEdge::World::addBlockTopSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x, blockPos.y + 1, blockPos.z };
    vec3_8 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rd = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    
    // std::cout << std::to_string(ld.x) << ", " << std::to_string(ld.y) << ", " << std::to_string(ld.z) << std::endl;
    
    int face = 0;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBottomSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x, blockPos.y, blockPos.z };
    vec3_8 lt = { blockPos.x, blockPos.y, blockPos.z + 1 };
    vec3_8 rt = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    vec3_8 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 1;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockFaceSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x, blockPos.y, blockPos.z + 1 };
    vec3_8 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    int face = 4;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBackSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x, blockPos.y, blockPos.z };
    vec3_8 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    vec3_8 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    vec3_8 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 5;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockLeftSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x, blockPos.y, blockPos.z };
    vec3_8 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    vec3_8 rt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rd = { blockPos.x, blockPos.y, blockPos.z + 1 };
    
    int face = 3;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockRightSideVertices(const vec3_8& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept
{
    vec3_8 ld = { blockPos.x + 1, blockPos.y, blockPos.z };
    vec3_8 lt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    vec3_8 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    vec3_8 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    int face = 2;
    
    int ldData = 0;
    ldData |= ldData | ld.x;
    ldData |= ldData | (ld.y << 6);
    ldData |= ldData | (ld.z << 12);
    ldData |= ldData | (face << 18);
    
    int ltData = 0;
    ltData |= ltData | lt.x;
    ltData |= ltData | (lt.y << 6);
    ltData |= ltData | (lt.z << 12);
    ltData |= ltData | (face << 18);
    
    int rtData = 0;
    rtData |= rtData | rt.x;
    rtData |= rtData | (rt.y << 6);
    rtData |= rtData | (rt.z << 12);
    rtData |= rtData | (face << 18);
    
    int rdData = 0;
    rdData |= rdData | rd.x;
    rdData |= rdData | (rd.y << 6);
    rdData |= rdData | (rd.z << 12);
    rdData |= rdData | (face << 18);
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rdData);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::render(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    for(const auto& chunk : m_chunks)
    {
        if(!chunk.second) continue;
        chunk.second->render(scene);
    }
}
