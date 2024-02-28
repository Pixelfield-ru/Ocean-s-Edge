//
// Created by ilya on 16.02.24.
//

#include "DayNightCycleSystem.h"

#include <SGCore/Scene/Scene.h>
#include <SGCore/Render/Atmosphere/Atmosphere.h>

void OceansEdge::DayNightCycleSystem::fixedUpdate(const double& dt, const double& fixedDt) noexcept
{
    auto lockedScene = m_scene.lock();
    if(!lockedScene) return;
    
    auto atmospheresView = lockedScene->getECSRegistry().view<SGCore::Atmosphere>();
    
    atmospheresView.each([this](SGCore::Atmosphere& atmosphere) {
        atmosphere.m_sunRotation.x += m_cycleSpeed;
    });
}
