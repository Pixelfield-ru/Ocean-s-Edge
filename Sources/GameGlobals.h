//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_GAMEGLOBALS_H
#define OCEANSEDGE_GAMEGLOBALS_H

#include <cstddef>
#include <glm/vec3.hpp>
#include <SGCore/Utils/ShadersPaths.h>

namespace OceansEdge
{
    using uvec3 = glm::vec<3, unsigned int, glm::defaultp>;
    using ulvec3 = glm::vec<3, size_t, glm::defaultp>;
    
    using uvec2 = glm::vec<2, unsigned int, glm::defaultp>;
    using ulvec2 = glm::vec<2, size_t, glm::defaultp>;
    
    using lvec3 = glm::vec<3, long, glm::defaultp>;
    using lvec2 = glm::vec<2, long, glm::defaultp>;
    
    using ivec3_8 = glm::vec<3, std::int8_t, glm::defaultp>;
    using ivec2_8 = glm::vec<2, std::int8_t, glm::defaultp>;
    
    using ivec3_32 = glm::vec<3, std::int32_t, glm::defaultp>;
    using ivec2_32 = glm::vec<2, std::int32_t, glm::defaultp>;
    
    class Settings
    {
    public:
        static inline long s_drawingRange = 5;
        static inline lvec3 s_chunksSize { 62, 1024, 62 };
        
        static void init()
        {
            m_shadersPaths["ChunkShader"].m_GLSL4RealizationPath = "../OEResources/shaders/glsl4/chunk_shader.glsl";
        }
        
        static auto& getShadersPaths() noexcept
        {
            return m_shadersPaths;
        }
        
    private:
        static inline SGCore::ShadersPaths m_shadersPaths;
    };
}

#endif //OCEANSEDGE_GAMEGLOBALS_H
