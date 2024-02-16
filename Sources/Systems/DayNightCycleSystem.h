//
// Created by ilya on 16.02.24.
//

#ifndef OCEANSEDGE_DAYNIGHTCYCLESYSTEM_H
#define OCEANSEDGE_DAYNIGHTCYCLESYSTEM_H

#include <SGCore/Scene/ISystem.h>

namespace SGCore
{
    class Scene;
}

namespace OceansEdge
{
    struct DayNightCycleSystem : SGCore::ISystem
    {
        float m_cycleSpeed = 0.01f;
        
        void fixedUpdate(const SGCore::Ref<SGCore::Scene>& scene) noexcept final;
    };
}

#endif //OCEANSEDGE_DAYNIGHTCYCLESYSTEM_H
