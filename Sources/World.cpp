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
#include <BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h>
#include <SGCore/Physics/PhysicsWorld3D.h>
#include <SGCore/Render/RenderingBase.h>
#include <SGCore/Render/Camera3D.h>
#include <SGCore/Transformations/Controllable3D.h>
#include <SGCore/Render/DebugDraw.h>
#include <SGCore/Input/InputManager.h>

#include "World.h"
#include "BlocksTypes.h"
#include "Chunk.h"
#include "BlockData.h"
#include "OEPhysicalEntity.h"
#include "Player/LocalPlayer.h"

void OceansEdge::World::prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    m_chunkTmpBlocks = flat_array<std::uint16_t, 3>(Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z);
    m_chunkTmpBlocks = flat_array<std::uint16_t, 3>(Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z);
    m_chunkTmpYMaximums = flat_array<long, 2>(Settings::s_chunksSize.x, Settings::s_chunksSize.z);
    
    auto& registry = scene->getECSRegistry();
    
    // ========================================================================
    
    m_playerEntity = registry.create();
    SGCore::EntityBaseInfo& cameraBaseInfo = registry.emplace<SGCore::EntityBaseInfo>(m_playerEntity);
    cameraBaseInfo.setRawName("SGMainCamera");
    registry.emplace<LocalPlayer>(m_playerEntity);
    registry.emplace<SGCore::Ref<OEPhysicalEntity>>(m_playerEntity);
    
    auto playerTransform = registry.emplace<SGCore::Ref<SGCore::Transform>>(m_playerEntity,
                                                                            SGCore::MakeRef<SGCore::Transform>());
    playerTransform->m_ownTransform.m_position = { 0.0, 500, 0 };
    
    auto& cameraEntityCamera = registry.emplace<SGCore::Ref<SGCore::Camera3D>>(m_playerEntity,
                                                                               SGCore::MakeRef<SGCore::Camera3D>());
    SGCore::Controllable3D& cameraEntityControllable = registry.emplace<SGCore::Controllable3D>(m_playerEntity);
    auto& cameraRenderingBase = registry.emplace<SGCore::Ref<SGCore::RenderingBase>>(m_playerEntity,
                                                                                     SGCore::MakeRef<SGCore::RenderingBase>());
    
    // ========================================================================
    
    
    const size_t totalSurfaceBlocksInChunkCnt = Settings::s_chunksSize.x * Settings::s_chunksSize.z * 2;
    
    size_t curChunk = 0;
    size_t blockIdx = 0;
    for(long x = -Settings::s_drawingRange; x < Settings::s_drawingRange; ++x)
    {
        for(long y = -Settings::s_drawingRange; y < Settings::s_drawingRange; ++y)
        {
            auto chunk = SGCore::MakeRef<Chunk>();
            m_chunks[{ x, y }] = chunk;
            
            ++curChunk;
            m_freeChunksEntities.insert(chunk);
        }
    }
    
    SGCore::InputManager::addInputListener(m_playerInputListener);
    
    std::cout << "created" << std::endl;
}

void OceansEdge::World::buildChunksGrid
(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed)
{
    ivec2_64 playerChunk = { std::floor(playerPosition.x / (Settings::s_chunksSize.x)), std::floor(playerPosition.z / (Settings::s_chunksSize.z)) };
    
    std::unordered_set<ivec2_64, SGCore::MathUtils::GLMVectorHash<ivec2_64>> tmpOccupiedIndices;
    
    size_t curEntityIdx = 0;
    for(long cx = playerChunk.x - Settings::s_drawingRange; cx < playerChunk.x + Settings::s_drawingRange; ++cx)
    {
        for(long cy = playerChunk.y - Settings::s_drawingRange; cy < playerChunk.y + Settings::s_drawingRange; ++cy)
        {
            tmpOccupiedIndices.insert(ivec2_64 { cx, cy });
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
    
   /* auto bIt = m_visibleBlocksTypes.begin();
    while(bIt != m_visibleBlocksTypes.end())
    {
        if(tmpOccupiedIndices.contains(bIt->first))
        {
            ++bIt;
        }
        else
        {
            bIt = m_visibleBlocksTypes.erase(bIt);
        }
    }*/
    
    // std::vector<float> blocksPositions;
    
    for(const auto& p : tmpOccupiedIndices)
    {
        if(!m_lastOccupiedIndices.contains(p) && !m_freeChunksEntities.empty())
        {
            auto fIt = m_freeChunksEntities.begin();
            
            auto chunk = *fIt;
            
            if(!chunk) continue;

            if(chunk->m_needsSubData) continue;

            ivec2_64 chunkIdx = p;
            
            m_visibleBlocksTypes[chunkIdx].clear();
            
            const glm::vec3 chunkPosition = { chunkIdx.x * Settings::s_chunksSize.x, 0, chunkIdx.y * Settings::s_chunksSize.z };
            chunk->m_position = { chunkPosition.x, 0, chunkPosition.z };
            chunk->m_aabb.m_min = chunkPosition;
            chunk->m_aabb.m_max = chunkPosition + glm::vec3 { Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z };
            
            // todo: make flexible settings
            /*perlinNoise.generate((lvec2 { chunkIdx.x, chunkIdx.y } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 },
                                 { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);*/
            
            const long endX = Settings::s_chunksSize.x;
            // const long endY = Settings::s_chunksSize.y;
            const long endZ = Settings::s_chunksSize.z;
            
            chunk->m_vertices.clear();
            chunk->m_indices.clear();
            chunk->m_currentIndex = 0;
            
            /*if(m_yDirDistribution(m_yDirDistributionRange) == 0)
            {
                m_yDir += 0.0015;
            }
            else
            {
                m_yDir -= 0.0015;
            }*/
            
            auto foundChangedChunkIt = m_changedBlocks.find(chunkIdx);
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    double rawY = m_perlinNoise.octave2D_11((chunkPosition.x + (float) x) * 0.01, (chunkPosition.z + (float) z) * 0.01, 2);
                    double by = rawY * 50; // * interpolate(chunkPosition.x + (float) x, chunkPosition.z + (float) z, rawY) * 0.01;
                    
                    const long endY = std::clamp<long>(std::ceil(Settings::s_chunksSize.y / 2 + by), 1, Settings::s_chunksSize.y - 1);
                    
                    for(long y = 0; y < Settings::s_chunksSize.y; ++y)
                    {
                        auto& blockData = m_chunkTmpBlocks[x][y][z]; // chunk->m_blocks[x][y][z];
                        
                        if(foundChangedChunkIt != m_changedBlocks.end())
                        {
                            auto findChangedBlock = foundChangedChunkIt->second.find({ x, y, z });
                            if(findChangedBlock != foundChangedChunkIt->second.end())
                            {
                                blockData = findChangedBlock->second;
                                std::cout << "sdfsfdfdfgdf" << std::endl;
                                continue;
                            }
                        }
                        
                        if(y < endY)
                        {
                            blockData = BlocksTypes::OEB_MUD_WITH_GRASS;
                        }
                        else
                        {
                            blockData = BlocksTypes::OEB_AIR;
                        }
                    }
                }
            }
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    long px = std::min(endX - 1, x + 1);
                    long mx = std::max<long>(0, x - 1);
                    long pz = std::min(endZ - 1, z + 1);
                    long mz = std::max<long>(0, z - 1);
                    
                    for(long y = 0; y < Settings::s_chunksSize.y; ++y)
                    {
                        if(m_chunkTmpBlocks[x][y][z] == BlocksTypes::OEB_AIR)
                        {
                            continue;
                        }
                        
                        long py = std::min<long>(Settings::s_chunksSize.y - 1, y + 1);
                        long my = std::max<long>(0, y - 1);
                        
                        ivec3_32 blockPos = { x, y, z };
                        
                        const auto& pxBD = m_chunkTmpBlocks[px][y][z];
                        const auto& mxBD = m_chunkTmpBlocks[mx][y][z];
                        const auto& pyBD = m_chunkTmpBlocks[x][py][z];
                        const auto& myBD = m_chunkTmpBlocks[x][my][z];
                        const auto& pzBD = m_chunkTmpBlocks[x][y][pz];
                        const auto& mzBD = m_chunkTmpBlocks[x][y][mz];
                        
                        bool isBlockVisible = false;
                        
                        if(pyBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockTopSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        if(myBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockBottomSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        if(pxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockRightSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        if(mxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockLeftSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        if(pzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockFaceSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        if(mzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockBackSideVertices(blockPos, chunk, nullptr);
                            isBlockVisible = true;
                        }
                        
                        if(isBlockVisible)
                        {
                            BlockData blockData;
                            blockData.m_type = m_chunkTmpBlocks[x][y][z];
                            blockData.m_indices = { x, y, z };
                            m_visibleBlocksTypes[chunkIdx].emplace_back(blockData);
                        }
                    }
                }
            }

            chunk->m_needsSubData = true;
            
            m_lastOccupiedIndices.emplace(p, chunk);
            
            fIt = m_freeChunksEntities.erase(fIt);
        }
    }
    
    // =============================
    
    auto playerTransform = scene->getECSRegistry().get<SGCore::Ref<SGCore::Transform>>(m_playerEntity);
    auto debugDraw = scene->getSystem<SGCore::DebugDraw>();
    
    // std::cout << playerTransform->m_ownTransform.m_forward << std::endl;
    
    debugDraw->drawLine(playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_position +
                        playerTransform->m_ownTransform.m_forward * 10, { 1.0, 1.0, 1.0, 1.0 });
    
    bool lmbPressed = m_playerInputListener->mouseButtonPressed(SGCore::MouseButton::MOUSE_BUTTON_LEFT);
    
    if(lmbPressed)
    {
        ivec2_64 foundNearestBlockChunk;
        BlockData* foundNearestBlockData = nullptr;
        float nearestDistance = std::numeric_limits<float>::max();
        
        for(auto& p : m_visibleBlocksTypes)
        {
            if(!m_lastOccupiedIndices[p.first]) continue;
            
            // td::cout << "sddf" << std::endl;
            
            glm::vec3 chunkPos = m_lastOccupiedIndices[p.first]->m_position;
            
            for(auto& blockData : p.second)
            {
                glm::vec3 blockMin =
                        chunkPos + glm::vec3 { blockData.m_indices.x, blockData.m_indices.y, blockData.m_indices.z };
                glm::vec3 blockMax = blockMin + glm::vec3 { 1, 1, 1 };
                
                float foundLength = 0;
                
                bool intersected = SGCore::MathUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin,
                        blockMax,
                        7,
                        foundLength);
                
                if(intersected)
                {
                    if(foundLength < nearestDistance)
                    {
                        foundNearestBlockChunk = p.first;
                        foundNearestBlockData = &blockData;
                        nearestDistance = foundLength;
                    }
                }
                
                // if(blo)
            }
        }
        
        if(foundNearestBlockData)
        {
            m_changedBlocks[foundNearestBlockChunk][foundNearestBlockData->m_indices] = BlocksTypes::OEB_AIR;
            m_freeChunksEntities.insert(m_lastOccupiedIndices[foundNearestBlockChunk]);
            m_lastOccupiedIndices.erase(foundNearestBlockChunk);
        }
    }
    
    // =============================
}

void OceansEdge::World::addBlockTopSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    
    int face = 0;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBottomSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 1;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockFaceSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z + 1 };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    //                          13  12  11  10   9   8   7   6   5   4   3   2   1   0
    // | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
    
    int face = 4;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBackSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 5;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockLeftSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x, blockPos.y, blockPos.z + 1 };
    
    // std::cout << std::to_string(rt.x) << ", " << std::to_string(rt.y) << ", " << std::to_string(rt.z) << std::endl;
    
    int face = 3;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 1);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    chunk->m_indices.push_back(chunk->m_currentIndex + 0);
    chunk->m_indices.push_back(chunk->m_currentIndex + 3);
    chunk->m_indices.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockRightSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x + 1, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    int face = 2;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    
    chunk->m_vertices.push_back(ldData);
    chunk->m_vertices.push_back(ld.y);
    
    chunk->m_vertices.push_back(ltData);
    chunk->m_vertices.push_back(lt.y);
    
    chunk->m_vertices.push_back(rtData);
    chunk->m_vertices.push_back(rt.y);
    
    chunk->m_vertices.push_back(rdData);
    chunk->m_vertices.push_back(rd.y);
    
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

SGCore::entity_t OceansEdge::World::getPlayerEntity() const noexcept
{
    return m_playerEntity;
}
