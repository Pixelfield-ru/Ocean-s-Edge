//
// Created by Ilya on 29.03.2024.
//

#include "WorldChunksUpdater.h"

#include "GameMain.h"
#include "Player/LocalPlayer.h"
#include <SGCore/Render/Camera3D.h>

#include <SGCore/Transformations/Transform.h>

void OceansEdge::WorldChunksUpdater::parallelUpdate(const double& dt, const double& fixedDt) noexcept
{
    auto lockedScene = m_scene.lock();
    if(!lockedScene) return;

    auto& registry = lockedScene->getECSRegistry();
    
    auto playerTransform = registry.get<SGCore::Ref<SGCore::Transform>>(GameMain::getCurrentWorld()->getPlayerEntity());
    
    GameMain::getCurrentWorld()->buildChunksGrid(lockedScene, playerTransform->m_ownTransform.m_position, GameMain::getCurrentWorld()->m_seed);
}
