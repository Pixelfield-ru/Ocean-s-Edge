//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNK_H
#define OCEANSEDGE_CHUNK_H

#include <vector>
#include <entt/entity/entity.hpp>

#include <unordered_map>
#include <SGUtils/Math/MathUtils.h>
#include <SGUtils/MultiDimensionalSingleArray.h>
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
        
        void render(const SGCore::Ref<SGCore::Scene>& scene);
        
        // using blocks_containter_t = std::unordered_map<lvec3, SGCore::Ref<BlockData>, SGCore::MathUtils::GLMVectorHash<lvec3>>;
        // using blocks_containter_t = std::vector<std::vector<std::vector<BlockData>>>;
        // using blocks_container_t = BlockData***;
        using blocks_container_t = SGCore::MultiDimensionalSingleArray<BlockData, 3>;
        
        blocks_container_t m_blocks;
        
        std::vector<std::uint32_t> m_indices;
        std::vector<float> m_vertices;
        
        SGCore::Ref<SGCore::IVertexArray> m_vertexArray;
        SGCore::Ref<SGCore::IVertexBuffer> m_positionsVertexBuffer;
        SGCore::Ref<SGCore::IIndexBuffer> m_indicesBuffer;
        SGCore::MeshDataRenderInfo m_renderInfo;
        
        size_t m_currentIndex = 0;
        
        bool m_needsSubData = false;
        
        const size_t m_maxVerticesCount = 250'000;
        const size_t m_maxIndicesCount = 250'000 + (m_maxVerticesCount / 3) + 10;
        
        SGCore::EventListener<void()> m_onRenderPipelineSetEventListener = SGCore::MakeEventListener<void()>([this]() {
            onRenderPipelineSet();
        });
        
        void onRenderPipelineSet() noexcept;
        
        SGCore::Ref<SGCore::IShader> m_shader;
    };
}

#endif //OCEANSEDGE_CHUNK_H
