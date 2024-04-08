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

#include "World.h"
#include "BlocksTypes.h"
#include "Chunk.h"
#include "BlockData.h"
#include "OEPhysicalEntity.h"

void OceansEdge::World::prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    m_chunkTmpBlocks = flat_array<std::uint16_t, 3>(Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z);
    m_chunkTmpYMaximums = flat_array<long, 2>(Settings::s_chunksSize.x, Settings::s_chunksSize.z);
    
    auto& registry = scene->getECSRegistry();
    
    const size_t totalSurfaceBlocksInChunkCnt = Settings::s_chunksSize.x * Settings::s_chunksSize.z * 2;
    
    std::random_device dev;
    m_yDirDistributionRange = std::mt19937(dev());
    m_yDirDistribution = std::uniform_int_distribution<std::mt19937::result_type>(0, 1);
    
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
    
    std::cout << "created" << std::endl;
}

void OceansEdge::World::buildChunksGrid
(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed)
{
    lvec2 playerChunk = { std::floor(playerPosition.x / (Settings::s_chunksSize.x)), std::floor(playerPosition.z / (Settings::s_chunksSize.z)) };
    
    std::unordered_set<lvec2, SGCore::MathUtils::GLMVectorHash<lvec2>> tmpOccupiedIndices;
    
    size_t curEntityIdx = 0;
    for(long cx = playerChunk.x - Settings::s_drawingRange; cx < playerChunk.x + Settings::s_drawingRange; ++cx)
    {
        for(long cy = playerChunk.y - Settings::s_drawingRange; cy < playerChunk.y + Settings::s_drawingRange; ++cy)
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

            if(chunk->m_needsSubData) continue;

            lvec2 chunkIdx = p;
            
            const glm::vec3 chunkPosition = { chunkIdx.x * Settings::s_chunksSize.x, 0, chunkIdx.y * Settings::s_chunksSize.z };
            chunk->m_position = { chunkPosition.x, 0, chunkPosition.z };
            chunk->m_aabb.m_min = chunkPosition;
            chunk->m_aabb.m_max = chunkPosition + glm::vec3 { Settings::s_chunksSize.x, Settings::s_chunksSize.y, Settings::s_chunksSize.z };
            
            // ==================================================================
            auto oeEntitiesView = scene->getECSRegistry().view<SGCore::Ref<OEPhysicalEntity>, SGCore::Ref<SGCore::Transform>>();
            oeEntitiesView.each([&chunk](const SGCore::entity_t& oeEntity, auto&, const SGCore::Ref<SGCore::Transform>& oeEntityTransform) {
                if(oeEntityTransform->m_finalTransform.m_aabb.isOverlappedBy(chunk->m_aabb))
                {
                    chunk->m_overlappedPhysicalEntities.insert(oeEntity);
                }
                else
                {
                    chunk->m_overlappedPhysicalEntities.erase(oeEntity);
                }
            });
            
            auto foundPhysicalChunkIt = std::find_if(m_occupiedPhysicalChunks.begin(), m_occupiedPhysicalChunks.end(), [&chunk](const SGCore::Ref<PhysicalChunk>& physicalChunk) {
                return physicalChunk->m_chunk == chunk.get();
            });
            
            SGCore::Ref<PhysicalChunk> physicalChunk;
            
            if(!chunk->m_overlappedPhysicalEntities.empty())
            {
                if(foundPhysicalChunkIt == m_occupiedPhysicalChunks.end())
                {
                    physicalChunk = m_freePhysicalChunks.top();
                    if(!physicalChunk)
                    {
                        physicalChunk = SGCore::MakeRef<PhysicalChunk>();
                        physicalChunk->m_entity = scene->getECSRegistry().create();
                        physicalChunk->m_rigidbody3D = scene->getECSRegistry().emplace<SGCore::Ref<SGCore::Rigidbody3D>>(
                                physicalChunk->m_entity, SGCore::MakeRef<SGCore::Rigidbody3D>(scene->getSystem<SGCore::PhysicsWorld3D>()));
                        scene->getECSRegistry().emplace<SGCore::Ref<SGCore::Transform>>(physicalChunk->m_entity,
                                SGCore::MakeRef<SGCore::Transform>());
                    }
                    else
                    {
                        physicalChunk->m_isColliderFormed = false;
                        physicalChunk->m_colliderVertices.clear();
                        m_freePhysicalChunks.pop();
                    }
                    
                    m_occupiedPhysicalChunks.push_back(physicalChunk);
                }
                else
                {
                    physicalChunk = *foundPhysicalChunkIt;
                }
                
                physicalChunk->m_chunk = chunk.get();
            }
            else
            {
                if(foundPhysicalChunkIt != m_occupiedPhysicalChunks.end())
                {
                    physicalChunk = *foundPhysicalChunkIt;
                    
                    physicalChunk->m_isColliderFormed = false;
                    physicalChunk->m_colliderVertices.clear();
                    m_occupiedPhysicalChunks.erase(foundPhysicalChunkIt);
                    m_freePhysicalChunks.push(*foundPhysicalChunkIt);
                }
            }
            
            // ==================================================================
            
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
            
            for(long x = 0; x < endX; ++x)
            {
                for(long z = 0; z < endZ; ++z)
                {
                    double rawY = m_perlinNoise.octave2D_11((chunkPosition.x + (float) x) * 0.01, (chunkPosition.z + (float) z) * 0.01, 2);
                    double by = rawY * 50; // * interpolate(chunkPosition.x + (float) x, chunkPosition.z + (float) z, rawY) * 0.01;
                    
                    const long endY = std::clamp<long>(std::ceil(Settings::s_chunksSize.y / 2 + by), 1, Settings::s_chunksSize.y - 1);
                    
                    m_chunkTmpYMaximums[x][z] = endY;
                    
                    for(long y = 0; y < Settings::s_chunksSize.y; ++y)
                    {
                        auto& blockData = m_chunkTmpBlocks[x][y][z]; // chunk->m_blocks[x][y][z];
                        
                        if(y < endY)
                        {
                            blockData = BlocksTypes::OEB_MUD_WITH_GRASS;
                        }
                        else
                        {
                            m_chunkTmpBlocks[x][y][z] = BlocksTypes::OEB_AIR;
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
                    
                    const long curY = m_chunkTmpYMaximums[x][z];
                    const long pxMaxY = m_chunkTmpYMaximums[px][z];
                    const long mxMaxY = m_chunkTmpYMaximums[mx][z];
                    const long pzMaxY = m_chunkTmpYMaximums[x][pz];
                    const long mzMaxY = m_chunkTmpYMaximums[x][mz];
                    
                    long minMaxY = pxMaxY;
                    minMaxY = std::min(std::min(std::min(minMaxY, mxMaxY), pzMaxY), mzMaxY);
                    if(minMaxY > curY)
                    {
                        minMaxY = curY;
                    }
                    
                    --minMaxY;
                    
                    for(long y = minMaxY; y < curY; ++y)
                    {
                        long py = std::min(curY, y + 1);
                        long my = std::max<long>(0, y - 1);
                        
                        ivec3_32 blockPos = { x, y, z };
                        
                        const auto& pxBD = m_chunkTmpBlocks[px][y][z];
                        const auto& mxBD = m_chunkTmpBlocks[mx][y][z];
                        const auto& pyBD = m_chunkTmpBlocks[x][py][z];
                        const auto& myBD = m_chunkTmpBlocks[x][my][z];
                        const auto& pzBD = m_chunkTmpBlocks[x][y][pz];
                        const auto& mzBD = m_chunkTmpBlocks[x][y][mz];
                        
                        if(pyBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockTopSideVertices(blockPos, chunk, physicalChunk);
                        }
                        if(pxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockRightSideVertices(blockPos, chunk, physicalChunk);
                        }
                        if(mxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockLeftSideVertices(blockPos, chunk, physicalChunk);
                        }
                        if(pzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockFaceSideVertices(blockPos, chunk, physicalChunk);
                        }
                        if(mzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockBackSideVertices(blockPos, chunk, physicalChunk);
                        }
                    }
                }
            }

            chunk->m_needsSubData = true;
            if(physicalChunk && physicalChunk->m_chunk == chunk.get() && !physicalChunk->m_isColliderFormed)
            {
                static SGCore::Threading::WorkerSingletonGuard physicalChunkWorkerGuard = SGCore::Threading::MakeWorkerSingletonGuard();
                auto physicalChunkWorker = scene->getSystem<SGCore::PhysicsWorld3D>()->getThread()->createWorker(physicalChunkWorkerGuard);
                physicalChunkWorker->setOnExecuteCallback([](SGCore::Ref<PhysicalChunk> physChunk, SGCore::Ref<Chunk> c) {
                    auto chunkRigidbody3D = physChunk->m_rigidbody3D;
                    
                    chunkRigidbody3D->removeFromWorld();
                    
                    physChunk->m_physicalTriangleMesh = SGCore::IMeshData::generatePhysicalMesh(c->m_vertices, c->m_indices);
                    auto shape = SGCore::MakeRef<btConvexTriangleMeshShape>(physChunk->m_physicalTriangleMesh.get(), true);
                    chunkRigidbody3D->setShape(shape);
                    
                    chunkRigidbody3D->m_bodyFlags.removeFlag(btCollisionObject::CF_STATIC_OBJECT);
                    chunkRigidbody3D->m_bodyFlags.addFlag(btCollisionObject::CF_DYNAMIC_OBJECT);
                    chunkRigidbody3D->m_body->setRestitution(0.9);
                    btScalar mass = 100.0f;
                    btVector3 inertia(0, 0, 0);
                    chunkRigidbody3D->m_body->getCollisionShape()->calculateLocalInertia(mass, inertia);
                    chunkRigidbody3D->m_body->setMassProps(mass, inertia);
                    chunkRigidbody3D->updateFlags();
                    chunkRigidbody3D->reAddToWorld();
                    
                    physChunk->m_isColliderFormed = true;
                    
                    std::cout << "dfdfdf" << std::endl;
                }, physicalChunk, chunk);
                
                scene->getSystem<SGCore::PhysicsWorld3D>()->getThread()->addWorker(physicalChunkWorker);
            }
            
            m_lastOccupiedIndices.emplace(p, chunk);
            
            fIt = m_freeChunksEntities.erase(fIt);
        }
    }
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
