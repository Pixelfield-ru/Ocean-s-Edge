//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNK_H
#define OCEANSEDGE_CHUNK_H

#include <vector>
#include <entt/entity/entity.hpp>
#include <unordered_set>
#include <mutex>

#include <unordered_map>
#include <SGUtils/Math/MathUtils.h>
#include <SGUtils/flat_array.h>
#include <SGCore/ImportedScenesArch/MeshDataRenderInfo.h>
#include <SGCore/Graphics/API/IShader.h>
#include <SGUtils/EventListener.h>
#include "GameGlobals.h"
#include "BlockData.h"

namespace OceansEdge
{
    struct Chunk
    {
        Chunk();
        
        SGCore::AABB<> m_aabb;
        std::unordered_set<SGCore::entity_t> m_overlappedPhysicalEntities;
        
        void render(const SGCore::Ref<SGCore::Scene>& scene);
        
        // using blocks_container_t = std::unordered_map<lvec3, BlockData, SGCore::MathUtils::GLMVectorHash<lvec3>>;
        // using blocks_container_t = std::vector<std::vector<std::vector<BlockData>>>;
        // using blocks_container_t = BlockData***;
        using blocks_container_t = flat_array<BlockData, 3>;
        
        // blocks_container_t m_blocks;
        
        std::vector<std::uint32_t> m_indices;
        std::vector<std::int32_t> m_vertices;
        
        std::mutex m_buffersChangeMutex;
        
        afvec32 m_position { 0, 0, 0 };
        // glm::vec3 m_position { 0, 0, 0 };
        
        SGCore::Ref<SGCore::IVertexArray> m_vertexArray;
        SGCore::Ref<SGCore::IVertexBuffer> m_positionsVertexBuffer;
        SGCore::Ref<SGCore::IIndexBuffer> m_indicesBuffer;
        SGCore::MeshDataRenderInfo m_renderInfo;
        
        size_t m_currentIndex = 0;
        
        std::atomic<bool> m_needsSubData = false;

        const size_t m_maxVerticesCount = 150'000;
        const size_t m_maxIndicesCount = m_maxVerticesCount + (m_maxVerticesCount / 3) + 10;
        
        bool isBuffersHaveFreeSpace() noexcept;

        void onRenderPipelineSet() noexcept;

        SGCore::EventListener<void()> m_onRenderPipelineSetEventListener = [this]() {
            onRenderPipelineSet();
        };
        
        SGCore::Ref<SGCore::IShader> m_shader;
    };
}

#endif //OCEANSEDGE_CHUNK_H
