//
// Created by ilya on 16.03.24.
//

#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/Render/DisableMeshGeometryPass.h>
#include <SGCore/Render/SpacePartitioning/OctreeCullable.h>
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
#include <SGUtils/Math/MathPrimitivesUtils.h>
#include <SGCore/Audio/AudioSource.h>
#include <SGCore/Render/LayeredFrameReceiver.h>
#include <SGCore/Graphics/API/IFrameBuffer.h>

#include "World.h"
#include "BlocksTypes.h"
#include "Chunk.h"
#include "BlockData.h"
#include "OEPhysicalEntity.h"
#include "Player/LocalPlayer.h"
#include "Resources.h"

void OceansEdge::World::prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    m_audioEntitiesPool = SGCore::EntitiesPool(scene->getECSRegistry());

    m_chunkTmpBlocks = flat_array<std::uint16_t, 3>(Settings::m_chunksSize.x, Settings::m_chunksSize.y, Settings::m_chunksSize.z);
    
    auto& registry = scene->getECSRegistry();
    
    // ========================================================================
    
    m_playerEntity = registry->create();
    SGCore::EntityBaseInfo& cameraBaseInfo = registry->emplace<SGCore::EntityBaseInfo>(m_playerEntity);
    cameraBaseInfo.setRawName("SGMainCamera");
    registry->emplace<LocalPlayer>(m_playerEntity);
    registry->emplace<SGCore::Ref<OEPhysicalEntity>>(m_playerEntity);
    
    auto playerTransform = registry->emplace<SGCore::Ref<SGCore::Transform>>(m_playerEntity,
                                                                            SGCore::MakeRef<SGCore::Transform>());
    playerTransform->m_ownTransform.m_position = { 0.0, 500, 0 };
    
    auto& cameraEntityCamera = registry->emplace<SGCore::Ref<SGCore::Camera3D>>(m_playerEntity,
                                                                               SGCore::MakeRef<SGCore::Camera3D>());
    SGCore::Controllable3D& cameraEntityControllable = registry->emplace<SGCore::Controllable3D>(m_playerEntity);
    auto& cameraRenderingBase = registry->emplace<SGCore::Ref<SGCore::RenderingBase>>(m_playerEntity,
                                                                                     SGCore::MakeRef<SGCore::RenderingBase>());
    
    auto& layeredFrameReceiver = registry->emplace<SGCore::LayeredFrameReceiver>(m_playerEntity);
    
    
    // ========================================================================
    
    
    const size_t totalSurfaceBlocksInChunkCnt = Settings::m_chunksSize.x * Settings::m_chunksSize.z * 2;
    
    size_t curChunk = 0;
    size_t blockIdx = 0;
    for(long x = -Settings::m_drawingRange; x < Settings::m_drawingRange; ++x)
    {
        for(long y = -Settings::m_drawingRange; y < Settings::m_drawingRange; ++y)
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
    std::lock_guard worldSaveLock(m_saveWorldMutex);
    
    ivec2_64 playerChunk = {std::floor(playerPosition.x / (Settings::m_chunksSize.x)), std::floor(playerPosition.z / (Settings::m_chunksSize.z)) };
    
    std::unordered_set<ivec2_64, SGCore::MathUtils::GLMVectorHash<ivec2_64>> tmpOccupiedIndices;
    
    size_t curEntityIdx = 0;
    for(long cx = playerChunk.x - Settings::m_drawingRange; cx < playerChunk.x + Settings::m_drawingRange; ++cx)
    {
        for(long cy = playerChunk.y - Settings::m_drawingRange; cy < playerChunk.y + Settings::m_drawingRange; ++cy)
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

    auto t0 = SGCore::now();

    for(const auto& p : tmpOccupiedIndices)
    {
        if(!m_lastOccupiedIndices.contains(p) && !m_freeChunksEntities.empty())
        {
            auto fIt = m_freeChunksEntities.begin();
            
            auto chunk = *fIt;
            
            m_currentChunkIndicesCopy.clear();
            m_currentChunkVerticesCopy.clear();
            
            if(!chunk) continue;

            // if(chunk->m_needsSubData) continue;

            ivec2_64 chunkIdx = p;
            
            m_visibleBlocksTypes[chunkIdx].clear();
            
            const glm::vec3 chunkPosition = { chunkIdx.x * Settings::m_chunksSize.x, 0, chunkIdx.y * Settings::m_chunksSize.z };
            chunk->m_position.x = chunkPosition.x;
            chunk->m_position.y = 0;
            chunk->m_position.z = chunkPosition.z;
            
            chunk->m_aabb.m_min = chunkPosition;
            chunk->m_aabb.m_max = chunkPosition + glm::vec3 {Settings::m_chunksSize.x, Settings::m_chunksSize.y, Settings::m_chunksSize.z };
            
            // todo: make flexible settings
            /*perlinNoise.generate((lvec2 { chunkIdx.x, chunkIdx.y } + lvec2 { Settings::s_drawingRange / 2, Settings::s_drawingRange / 2 }) * lvec2 { Settings::s_chunksSize.x * 2, Settings::s_chunksSize.z * 2 },
                                 { Settings::s_chunksSize.x, Settings::s_chunksSize.z}, 3, 0.6f);*/
            
            const long endX = Settings::m_chunksSize.x;
            // const long endY = Settings::s_chunksSize.y;
            const long endZ = Settings::m_chunksSize.z;
            
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
                    double thisRawY = m_perlinNoise.octave2D_11((chunkPosition.x + (float) x) * 0.01, (chunkPosition.z + (float) z) * 0.01, 2);
                    const long thisEndY = std::clamp<long>(std::ceil(Settings::m_chunksSize.y / 2 + thisRawY * 50), 1, Settings::m_chunksSize.y - 1);
                    
                    for(long y = 0; y < Settings::m_chunksSize.y; ++y)
                    {
                        auto& blockType = m_chunkTmpBlocks[x][y][z]; // chunk->m_blocks[x][y][z];
                        
                        if(foundChangedChunkIt != m_changedBlocks.end())
                        {
                            auto findChangedBlock = foundChangedChunkIt->second.find({ x, y, z });
                            if(findChangedBlock != foundChangedChunkIt->second.end())
                            {
                                blockType = findChangedBlock->second;
                                continue;
                            }
                        }
                        
                        bool isYLowerThanEnd = y < thisEndY;
                        
                        if(isYLowerThanEnd)
                        {
                            blockType = BlocksTypes::OEB_MUD_WITH_GRASS;
                        }
                        else
                        {
                            blockType = BlocksTypes::OEB_AIR;
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
                    
                    for(long y = 0; y < Settings::m_chunksSize.y; ++y)
                    {
                        std::uint16_t blockType = m_chunkTmpBlocks[x][y][z];
                        
                        if(blockType == BlocksTypes::OEB_AIR)
                        {
                            continue;
                        }
                        
                        long py = std::min<long>(Settings::m_chunksSize.y - 1, y + 1);
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
                            addBlockTopSideVertices(blockPos, chunk, nullptr, blockType);
                            isBlockVisible = true;
                        }
                        if(myBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockBottomSideVertices(blockPos, chunk, nullptr, blockType);
                            isBlockVisible = true;
                        }
                        if(pxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockRightSideVertices(blockPos, chunk, nullptr, blockType);
                            isBlockVisible = true;
                        }
                        if(mxBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockLeftSideVertices(blockPos, chunk, nullptr, blockType);
                            isBlockVisible = true;
                        }
                        if(pzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockFaceSideVertices(blockPos, chunk, nullptr, blockType);
                            isBlockVisible = true;
                        }
                        if(mzBD == BlocksTypes::OEB_AIR)
                        {
                            addBlockBackSideVertices(blockPos, chunk, nullptr, blockType);
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
            
            {
                std::lock_guard currentChunkBuffersGuard(chunk->m_buffersChangeMutex);
                
                chunk->m_indices = m_currentChunkIndicesCopy;
                chunk->m_vertices = m_currentChunkVerticesCopy;
            }
            
            chunk->m_needsSubData = true;
            
            m_lastOccupiedIndices.emplace(p, chunk);
            
            m_freeChunksEntities.erase(fIt);
        }
    }

    auto t1 = SGCore::now();

    // std::cout << "time dif0: " << SGCore::timeDiff<double, std::milli>(t0, t1) << std::endl;
    
    // now fill extreme blocks of chunks
    /*for(auto& blocksPair : m_visibleBlocksTypes)
    {
        auto& curChunk = m_lastOccupiedIndices[blocksPair.first];
        
        if(!curChunk) continue;
        
        m_currentChunkIndicesCopy.clear();
        m_currentChunkVerticesCopy.clear();
        
        for(auto& block : blocksPair.second)
        {
            // block is on left of chunk
            if(block.m_indices.x == 0)
            {
                ivec2_64 leftChunkIndices = { blocksPair.first.x - 1, blocksPair.first.y };
                
                auto foundChunkIt = m_visibleBlocksTypes.find(leftChunkIndices);
                if(foundChunkIt != m_visibleBlocksTypes.end())
                {
                    ivec3_16 blockIndices = { Settings::s_chunksSize.x - 1, block.m_indices.y, block.m_indices.z };
                    
                    auto foundVisibleBlockIt = std::find_if(foundChunkIt->second.begin(), foundChunkIt->second.end(), [&blockIndices](const BlockData& blockData) {
                        return blockIndices == blockData.m_indices;
                    });
                    
                    if(foundVisibleBlockIt == foundChunkIt->second.end())
                    {
                        ivec3_32 blockPos = { curChunk->m_position.x + block.m_indices.x,
                                              curChunk->m_position.y + block.m_indices.y,
                                              curChunk->m_position.z + block.m_indices.z };
                        
                        addBlockLeftSideVertices(block.m_indices, curChunk, nullptr, block.m_type);
                    }
                }
            }
        }
        
        {
            std::lock_guard currentChunkBuffersGuard(curChunk->m_buffersChangeMutex);
            
            curChunk->m_indices.insert(curChunk->m_indices.end(), m_currentChunkIndicesCopy.begin(), m_currentChunkIndicesCopy.end());
            curChunk->m_vertices.insert(curChunk->m_vertices.end(), m_currentChunkVerticesCopy.begin(), m_currentChunkVerticesCopy.end());
        }
        
        // curChunk->m_needsSubData = true;
    }*/
    
    // =============================
    
    auto playerTransform = scene->getECSRegistry()->get<SGCore::Ref<SGCore::Transform>>(m_playerEntity);
    auto& localPlayer = scene->getECSRegistry()->get<LocalPlayer>(m_playerEntity);
    auto debugDraw = scene->getSystem<SGCore::DebugDraw>();
    
    // std::cout << playerTransform->m_ownTransform.m_forward << std::endl;
    
    debugDraw->drawLine(playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_position +
                        playerTransform->m_ownTransform.m_forward * 10, { 1.0, 1.0, 1.0, 1.0 });
    
    bool lmbPressed = m_playerInputListener->mouseButtonPressed(SGCore::MouseButton::MOUSE_BUTTON_LEFT);
    bool rmbPressed = m_playerInputListener->mouseButtonPressed(SGCore::MouseButton::MOUSE_BUTTON_RIGHT);
    
    if(lmbPressed || rmbPressed)
    {
        ivec2_64 foundNearestBlockChunk;
        BlockData* foundNearestBlockData = nullptr;
        float nearestDistance = std::numeric_limits<float>::max();
        
        for(auto& p : m_visibleBlocksTypes)
        {
            auto foundChunk = m_lastOccupiedIndices.find(p.first);
            
            if(foundChunk == m_lastOccupiedIndices.end()) continue;
            
            // td::cout << "sddf" << std::endl;
            
            glm::vec3 chunkPos = foundChunk->second->m_position;
            
            for(auto& blockData : p.second)
            {
                glm::vec3 blockMin =
                        chunkPos + glm::vec3 { blockData.m_indices.x, blockData.m_indices.y, blockData.m_indices.z };
                glm::vec3 blockMax = blockMin + glm::vec3 { 1, 1, 1 };
                
                SGCore::MathPrimitivesUtils::RayIntersectionInfo intersectionInfo;
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin,
                        blockMax,
                        7,
                        intersectionInfo);
                
                if(intersectionInfo.m_isIntersected)
                {
                    if(intersectionInfo.m_hitDistance < nearestDistance)
                    {
                        foundNearestBlockChunk = p.first;
                        foundNearestBlockData = &blockData;
                        nearestDistance = intersectionInfo.m_hitDistance;
                    }
                }
                
                // if(blo)
            }
        }
        
        auto foundNearestChunkIt = m_lastOccupiedIndices.find(foundNearestBlockChunk);
        
        if(foundNearestBlockData && foundNearestChunkIt != m_lastOccupiedIndices.end())
        {
            if(lmbPressed)
            {
                std::vector<std::string>* soundsName = nullptr;
                std::uint32_t rndNearRange = 0;
                std::uint32_t rndFarRange = 0;

                if(foundNearestBlockData->m_type == BlocksTypes::OEB_BRICKS)
                {
                    soundsName = &Resources::m_bricksBreakBuffersNames;
                }
                else if(foundNearestBlockData->m_type == BlocksTypes::OEB_MUD_WITH_GRASS)
                {
                    soundsName = &Resources::m_grassWithMudBreakBuffersNames;
                }
                else if(foundNearestBlockData->m_type == BlocksTypes::OEB_STONE)
                {
                    soundsName = &Resources::m_stoneBreakBuffersNames;
                }

                if(soundsName)
                {
                    rndFarRange = soundsName->size() - 1;

                    std::random_device rndDevice;
                    std::mt19937 rng(rndDevice());
                    std::uniform_int_distribution<std::mt19937::result_type> distribution(rndNearRange, rndFarRange);

                    createBlockSound(foundNearestChunkIt->second, *foundNearestBlockData,
                                     (*soundsName)[distribution(rng)]);
                }

                m_changedBlocks[foundNearestBlockChunk][foundNearestBlockData->m_indices] = BlocksTypes::OEB_AIR;
                m_freeChunksEntities.insert(foundNearestChunkIt->second);
                m_lastOccupiedIndices.erase(foundNearestBlockChunk);
            }
            
            if(rmbPressed)
            {
                // DUMMY !!!!!

                std::vector<std::string>* soundsName = nullptr;
                std::uint32_t rndNearRange = 0;
                std::uint32_t rndFarRange = 0;

                if(localPlayer.m_currentSelectedBlockType == BlocksTypes::OEB_BRICKS)
                {
                    soundsName = &Resources::m_bricksBreakBuffersNames;
                }
                else if(localPlayer.m_currentSelectedBlockType == BlocksTypes::OEB_MUD_WITH_GRASS)
                {
                    soundsName = &Resources::m_grassWithMudBreakBuffersNames;
                }
                else if(localPlayer.m_currentSelectedBlockType == BlocksTypes::OEB_STONE)
                {
                    soundsName = &Resources::m_stoneBreakBuffersNames;
                }

                if(soundsName)
                {
                    rndFarRange = soundsName->size() - 1;

                    std::random_device rndDevice;
                    std::mt19937 rng(rndDevice());
                    std::uniform_int_distribution<std::mt19937::result_type> distribution(rndNearRange, rndFarRange);

                    createBlockSound(foundNearestChunkIt->second, *foundNearestBlockData,
                                     (*soundsName)[distribution(rng)]);
                }

                glm::vec3 chunkPos = foundNearestChunkIt->second->m_position;
                
                ivec3_16 blockToPutIdx = foundNearestBlockData->m_indices;
                ivec2_64 chunkToPutIdx = foundNearestBlockChunk;
                
                glm::vec3 blockMin =
                        chunkPos + glm::vec3 { blockToPutIdx.x, blockToPutIdx.y, blockToPutIdx.z };
                
                SGCore::MathPrimitivesUtils::RayIntersectionInfo<> intersectionInfos[6];
                
                // +Y
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin + glm::vec3 { 0, 0.999, 0 },
                        blockMin + glm::vec3 { 1, 1, 1 },
                        7,
                        intersectionInfos[0]
                );
                
                // -Y
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin,
                        blockMin + glm::vec3 { 1, 0.001, 1 },
                        7,
                        intersectionInfos[1]
                );
                
                // +X
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin + glm::vec3 { 0.999, 0, 0 },
                        blockMin + glm::vec3 { 1, 1, 1 },
                        7,
                        intersectionInfos[2]
                );
                
                // -X
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin,
                        blockMin + glm::vec3 { 0.001, 1, 1 },
                        7,
                        intersectionInfos[3]
                );
                
                // +Z
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin + glm::vec3 { 0, 0, 0.999 },
                        blockMin + glm::vec3 { 1, 1, 1 },
                        7,
                        intersectionInfos[4]
                );
                
                // -Z
                SGCore::MathPrimitivesUtils::lineAABBIntersection<float>(
                        playerTransform->m_ownTransform.m_position,
                        playerTransform->m_ownTransform.m_forward,
                        blockMin,
                        blockMin + glm::vec3 { 1, 1, 0.001 },
                        7,
                        intersectionInfos[5]
                );
                
                float foundNearestDistance = std::numeric_limits<float>::max();
                std::uint8_t foundFaceIdx = 0;
                
                for(std::uint8_t i = 0; i < 6; ++i)
                {
                    if(intersectionInfos[i].m_isIntersected && intersectionInfos[i].m_hitDistance < foundNearestDistance)
                    {
                        foundNearestDistance = intersectionInfos[i].m_hitDistance;
                        foundFaceIdx = i;
                    }
                }
                
                // top face
                if(foundFaceIdx == 0)
                {
                    blockToPutIdx.y = std::clamp<float>(blockToPutIdx.y + 1, 0, Settings::m_chunksSize.y);
                }
                else if(foundFaceIdx == 1)
                {
                    blockToPutIdx.y = std::clamp<float>(blockToPutIdx.y - 1, 0, Settings::m_chunksSize.y);
                }
                else if(foundFaceIdx == 2)
                {
                    ++blockToPutIdx.x;
                    if(blockToPutIdx.x > Settings::m_chunksSize.x - 1)
                    {
                        ++chunkToPutIdx.x;
                        blockToPutIdx.x = 0;
                    }
                }
                else if(foundFaceIdx == 3)
                {
                    --blockToPutIdx.x;
                    if(blockToPutIdx.x < 0)
                    {
                        --chunkToPutIdx.x;
                        blockToPutIdx.x = Settings::m_chunksSize.x - 1;
                    }
                }
                else if(foundFaceIdx == 4)
                {
                    ++blockToPutIdx.z;
                    if(blockToPutIdx.z > Settings::m_chunksSize.z - 1)
                    {
                        ++chunkToPutIdx.y;
                        blockToPutIdx.z = 0;
                    }
                }
                else if(foundFaceIdx == 5)
                {
                    --blockToPutIdx.z;
                    if(blockToPutIdx.z < 0)
                    {
                        --chunkToPutIdx.y;
                        blockToPutIdx.z = Settings::m_chunksSize.z - 1;
                    }
                }
                
                m_changedBlocks[chunkToPutIdx][blockToPutIdx] = localPlayer.m_currentSelectedBlockType;
                auto chunkToUpdateIt = m_lastOccupiedIndices.find(chunkToPutIdx);
                if(chunkToUpdateIt != m_lastOccupiedIndices.end())
                {
                    m_freeChunksEntities.insert(chunkToUpdateIt->second);
                    m_lastOccupiedIndices.erase(chunkToPutIdx);
                }
            }
        }
    }
    
    // =============================
}

void OceansEdge::World::createBlockSound
(const SGCore::Ref<OceansEdge::Chunk>& chunk, const OceansEdge::BlockData& blockData, const std::string& audioBufferName) noexcept
{
    SGCore::AudioSource* audioSource = nullptr;

    bool isCreatedNew;
    SGCore::entity_t audioEntity = m_audioEntitiesPool.pop(isCreatedNew);
    auto poolRegistry = m_audioEntitiesPool.getAttachedRegistry();
    if(isCreatedNew)
    {
        if(poolRegistry)
        {
            audioSource = &poolRegistry->emplace<SGCore::AudioSource>(audioEntity);
        }
    }
    else
    {
        if(poolRegistry)
        {
            audioSource = &poolRegistry->get<SGCore::AudioSource>(audioEntity);
        }
    }

    if(audioSource && isCreatedNew)
    {
        audioSource->create();
        audioSource->onStateChanged += [this, audioEntity](SGCore::AudioSource& as, SGCore::AudioSourceState lastState,
                                                           SGCore::AudioSourceState newState)
        {
            if(lastState == SGCore::AudioSourceState::SOURCE_PLAYING && newState == SGCore::AudioSourceState::SOURCE_STOPPED)
            {
                m_audioEntitiesPool.push(audioEntity);
            }
        };
    }

    if(audioSource)
    {
        glm::vec3 audioPos = { chunk->m_position.x + blockData.m_indices.x + 0.5f,
                               chunk->m_position.y + blockData.m_indices.y + 0.5f,
                               chunk->m_position.z + blockData.m_indices.z + 0.5f };

        audioSource->setRolloffFactor(0.1f);
        audioSource->setPosition(audioPos);
        audioSource->attachBuffer(Resources::getAudioBuffersMap()[audioBufferName]);
        audioSource->setState(SGCore::AudioSourceState::SOURCE_PLAYING);
    }
}

void OceansEdge::World::addBlockTopSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    
    int face = 0;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBottomSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 1;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockFaceSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z + 1 };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    //                          13  12  11  10   9   8   7   6   5   4   3   2   1   0
    // | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
    
    int face = 4;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockBackSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z };
    
    int face = 5;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockLeftSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x, blockPos.y, blockPos.z + 1 };
    
    // std::cout << std::to_string(rt.x) << ", " << std::to_string(rt.y) << ", " << std::to_string(rt.z) << std::endl;
    
    int face = 3;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::addBlockRightSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept
{
    if(!chunk->isBuffersHaveFreeSpace()) return;
    
    ivec3_32 ld = { blockPos.x + 1, blockPos.y, blockPos.z };
    ivec3_32 lt = { blockPos.x + 1, blockPos.y + 1, blockPos.z };
    ivec3_32 rt = { blockPos.x + 1, blockPos.y + 1, blockPos.z + 1 };
    ivec3_32 rd = { blockPos.x + 1, blockPos.y, blockPos.z + 1 };
    
    int face = 2;
    std::int32_t blockType32 = blockType;
    
    int ldData = 0;
    ldData |= ld.x;
    ldData |= ld.z << 6;
    ldData |= face << 12;
    ldData |= blockType32 << 15;
    
    int ltData = 0;
    ltData |= lt.x;
    ltData |= lt.z << 6;
    ltData |= face << 12;
    ltData |= blockType32 << 15;
    
    int rtData = 0;
    rtData |= rt.x;
    rtData |= rt.z << 6;
    rtData |= face << 12;
    rtData |= blockType32 << 15;
    
    int rdData = 0;
    rdData |= rd.x;
    rdData |= rd.z << 6;
    rdData |= face << 12;
    rdData |= blockType32 << 15;
    
    m_currentChunkVerticesCopy.push_back(ldData);
    m_currentChunkVerticesCopy.push_back(ld.y);
    
    m_currentChunkVerticesCopy.push_back(ltData);
    m_currentChunkVerticesCopy.push_back(lt.y);
    
    m_currentChunkVerticesCopy.push_back(rtData);
    m_currentChunkVerticesCopy.push_back(rt.y);
    
    m_currentChunkVerticesCopy.push_back(rdData);
    m_currentChunkVerticesCopy.push_back(rd.y);
    
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 1);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 0);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 3);
    m_currentChunkIndicesCopy.push_back(chunk->m_currentIndex + 2);
    
    chunk->m_currentIndex += 4;
}

void OceansEdge::World::render(const SGCore::Ref<SGCore::Scene>& scene) noexcept
{
    using namespace SGCore;
    
    auto camerasView = scene->getECSRegistry()->view<Ref<Camera3D>, Ref<RenderingBase>, Ref<Transform>>();
    
    camerasView.each([&scene, this]
    (const entity_t& cameraEntity, Ref<Camera3D> camera3D, Ref<RenderingBase> renderingBase, Ref<Transform> transform) {
        CoreMain::getRenderer()->prepareUniformBuffers(renderingBase, transform);
        
        LayeredFrameReceiver* cameraLayeredFrameReceiver = scene->getECSRegistry()->try_get<LayeredFrameReceiver>(cameraEntity);
        
        Ref<PostProcessLayer> chunksPPLayer;
        
        if(cameraLayeredFrameReceiver)
        {
            chunksPPLayer = cameraLayeredFrameReceiver->getLayer("chunks_layer");
            
            chunksPPLayer->m_frameBuffer->bind();
            chunksPPLayer->m_frameBuffer->bindAttachmentsToDrawIn(chunksPPLayer->m_attachmentsToRenderIn);
        }
        
        for(const auto& chunk : m_chunks)
        {
            if(!chunk.second) continue;
            chunk.second->render(scene);
        }
        
        if(cameraLayeredFrameReceiver)
        {
            chunksPPLayer->m_frameBuffer->unbind();
        }
    });
}

void OceansEdge::World::save() noexcept
{
    std::string buf = glz::write<glz::opts { .prettify = true }>(this);
    
    SGUtils::FileUtils::writeToFile(Settings::m_worldSavePath, buf, false, true);
    
    std::cout << "world saved" << std::endl;
}

SGCore::entity_t OceansEdge::World::getPlayerEntity() const noexcept
{
    return m_playerEntity;
}

void OceansEdge::World::load() noexcept
{
    if(std::filesystem::exists(Settings::m_worldSavePath))
    {
        std::string json = SGUtils::FileUtils::readFile(Settings::m_worldSavePath);
        
        glz::read_json(*this, json);
        
        std::cout << "world loaded" << std::endl;
    }
}
