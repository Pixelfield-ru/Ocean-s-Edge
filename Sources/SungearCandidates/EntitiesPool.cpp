//
// Created by ilya on 14.04.24.
//

#include "EntitiesPool.h"

SGCore::EntitiesPool::EntitiesPool(const SGCore::Ref<SGCore::registry_t>& attachedRegistry)
{
    m_attachedRegistry = attachedRegistry;
}

SGCore::entity_t SGCore::EntitiesPool::pop() noexcept
{
    std::lock_guard guard(m_mutex);
    
    if(m_freeEntities.empty())
    {
        if(auto lockedRegistry = m_attachedRegistry.lock())
        {
            return lockedRegistry->create();
        }
        else
        {
            m_freeEntities.clear();
            m_busyEntities.clear();
            
            return entt::null;
        }
    }
    else
    {
        entity_t entity = *m_freeEntities.rbegin();
        
        m_freeEntities.pop_back();
        
        m_busyEntities.push_back(entity);
        
        return entity;
    }
}

void SGCore::EntitiesPool::push(const entity_t& entity) noexcept
{
    std::lock_guard guard(m_mutex);
    
    if(std::remove(m_busyEntities.begin(), m_busyEntities.end(), entity) != m_busyEntities.end())
    {
        m_freeEntities.push_back(entity);
    }
}
