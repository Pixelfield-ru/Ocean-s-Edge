//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_WORLD_H
#define OCEANSEDGE_WORLD_H

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <entt/entt.hpp>
#include <glaze/glaze.hpp>
#include <SGCore/Scene/Scene.h>
#include <bitset>
#include <bit>
#include <stack>
#include <SGCore/Input/InputListener.h>
#include <SGCore/Utils/ECS/EntitiesPool.h>

#include "GameGlobals.h"
#include "Chunk.h"
#include "SGUtils/Noise/PerlinNoise.hpp"
#include "PhysicalChunk.h"

namespace OceansEdge
{
    using changed_block_collection_t = std::unordered_map<ivec2_64, std::unordered_map<ivec3_16, std::uint16_t, SGCore::MathUtils::GLMVectorHash<ivec3_16>>, SGCore::MathUtils::GLMVectorHash<ivec2_64>>;
    
    class World
    {
    public:
        std::mutex m_saveWorldMutex;
        
        siv::PerlinNoise::seed_type m_seed = 123456u;
        
        using chunks_container_t = std::unordered_map<ivec2_64, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<ivec2_64>>;
        
        void render(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void buildChunksGrid(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed);

        void createBlockSound(const SGCore::Ref<Chunk>& chunk, const BlockData& blockData, const std::string& audioBufferName) noexcept;

        void save() noexcept;
        void load() noexcept;
        
        SGCore::entity_t getPlayerEntity() const noexcept;
    private:
        SGCore::EntitiesPool m_audioEntitiesPool;
        
        std::uniform_int_distribution<std::mt19937::result_type> m_yDirDistribution;
        
        SGCore::Ref<SGCore::InputListener> m_playerInputListener = SGCore::MakeRef<SGCore::InputListener>();
        
        chunks_container_t m_chunks;
        
        SGCore::entity_t m_playerEntity;
        
        std::vector<std::uint32_t> m_currentChunkIndicesCopy;
        std::vector<std::int32_t> m_currentChunkVerticesCopy;
        
        // first - chunk index
        // second - vector of visible blocks types
        std::unordered_map<ivec2_64, std::vector<BlockData>, SGCore::MathUtils::GLMVectorHash<ivec2_64>> m_visibleBlocksTypes;
        // FIRST - CHUNK IDX
        // SECOND - MAP where FIRST - BLOCK IDX and SECOND - BLOCK TYPE
        changed_block_collection_t m_changedBlocks;
        
        flat_array<std::uint16_t, 3> m_chunkTmpBlocks;
        flat_array<long, 2> m_chunkTmpYMaximums;
        
        siv::PerlinNoise m_perlinNoise { m_seed };
        
        std::unordered_set<SGCore::Ref<Chunk>> m_freeChunksEntities;
        // std::unordered_set<SGCore::Ref<Chunk>> m_occupiedChunksEntities;
        std::unordered_map<ivec2_64, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<ivec2_64>> m_lastOccupiedIndices;
        
        void addBlockTopSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        void addBlockBottomSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        void addBlockFaceSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        void addBlockBackSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        void addBlockLeftSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        void addBlockRightSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk, const SGCore::Ref<PhysicalChunk>& physicalChunk, const std::uint16_t& blockType) noexcept;
        // static inline std::unordered_set<entt::entity> m_chunksEntities;
        
    public:
        struct glaze
        {
            using T = OceansEdge::World;
            static constexpr auto value = glz::object(
                    &T::m_seed,
                    &T::m_changedBlocks
            );
        };
    };
    
    struct ChangedBlockMeta
    {
        ivec2_64 m_chunkIndices { };
        ivec3_16 m_blockIndices { };
        std::uint16_t m_blockType = BlocksTypes::OEB_AIR;
    };
    
    struct WorldSavingMeta
    {
        siv::PerlinNoise::seed_type m_seed;
        
        std::vector<ChangedBlockMeta> m_changedBlocks;
    };
}

/*template <>
struct glz::meta<OceansEdge::World> {
    using T = OceansEdge::World;
    static constexpr auto value = object(
            &T::m_seed,
            &T::m_changedBlocks
    );
};*/

#endif //OCEANSEDGE_WORLD_H
