//
// Created by ilya on 14.04.24.
//

#ifndef OCEANSEDGE_ENTITIESPOOL_H
#define OCEANSEDGE_ENTITIESPOOL_H

#include <vector>
#include <entt/entity/entity.hpp>
#include <mutex>

#include "SGCore/Main/CoreGlobals.h"

namespace SGCore
{
    struct EntitiesPool
    {
        EntitiesPool() = default;
        explicit EntitiesPool(const Ref<registry_t>& attachedRegistry);
        
        entity_t pop() noexcept;
        void push(const entity_t& entity) noexcept;
    
    private:
        std::mutex m_mutex;
        
        Weak<registry_t> m_attachedRegistry;
        
        std::vector<entity_t> m_busyEntities;
        std::vector<entity_t> m_freeEntities;
    };
}

#endif //OCEANSEDGE_ENTITIESPOOL_H
