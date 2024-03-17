//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_GAMEGLOBALS_H
#define OCEANSEDGE_GAMEGLOBALS_H

#include <cstddef>
#include <glm/vec3.hpp>

namespace OceansEdge
{
    using uvec3 = glm::vec<3, unsigned int, glm::defaultp>;
    using ulvec3 = glm::vec<3, size_t, glm::defaultp>;
    
    using uvec2 = glm::vec<2, unsigned int, glm::defaultp>;
    using ulvec2 = glm::vec<2, size_t, glm::defaultp>;
    
    namespace Settings
    {
        static inline size_t s_drawingRange = 10;
        static inline ulvec3 s_chunksSize { 32, 256, 32 };
    }
}

#endif //OCEANSEDGE_GAMEGLOBALS_H
