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
    
    using ivec3_8 = glm::vec<3, std::int8_t, glm::defaultp>;
    using ivec2_8 = glm::vec<2, std::int8_t, glm::defaultp>;
    
    using ivec3_16 = glm::vec<3, std::int16_t, glm::defaultp>;
    using ivec2_16 = glm::vec<2, std::int16_t, glm::defaultp>;
    
    using ivec3_32 = glm::vec<3, std::int32_t, glm::defaultp>;
    using ivec2_32 = glm::vec<2, std::int32_t, glm::defaultp>;
    
    using ivec3_64 = glm::vec<3, std::int64_t, glm::defaultp>;
    using ivec2_64 = glm::vec<2, std::int64_t, glm::defaultp>;
    
    class Settings
    {
    public:
        static inline long s_drawingRange = 8;
        static inline ivec3_64 s_chunksSize { 62, 1024, 62 };
        
        static void init()
        {
            m_shadersPaths["ChunkShader"].m_GLSL4RealizationPath = "../OEResources/shaders/glsl4/chunk_shader.glsl";
            m_shadersPaths["FoggedSkyboxShader"].m_GLSL4RealizationPath = "../SGResources/shaders/glsl4/skybox/default_shader.glsl"; // "../OEResources/shaders/glsl4/fogged_skybox.glsl";
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
