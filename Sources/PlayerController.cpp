//
// Created by ilya on 13.04.24.
//

#include <SGCore/Input/InputManager.h>
#include "PlayerController.h"
#include "GameMain.h"
#include "Player/LocalPlayer.h"

OceansEdge::PlayerController::PlayerController()
{
    SGCore::InputManager::addInputListener(m_playerInputListener);
}

void OceansEdge::PlayerController::update(const double& dt, const double& fixedDt)
{
    auto lockedScene = m_scene.lock();
    
    if(!lockedScene) return;
    
    auto& registry = lockedScene->getECSRegistry();
    
    auto& localPlayer = registry.get<LocalPlayer>(GameMain::getCurrentWorld()->getPlayerEntity());
    
    if(m_playerInputListener->keyboardKeyPressed(SGCore::KeyboardKey::KEY_1))
    {
        localPlayer.m_currentSelectedBlockType = BlocksTypes::OEB_MUD_WITH_GRASS;
    }
    if(m_playerInputListener->keyboardKeyPressed(SGCore::KeyboardKey::KEY_2))
    {
        localPlayer.m_currentSelectedBlockType = BlocksTypes::OEB_BRICKS;
    }
    if(m_playerInputListener->keyboardKeyPressed(SGCore::KeyboardKey::KEY_3))
    {
        localPlayer.m_currentSelectedBlockType = BlocksTypes::OEB_STONE;
    }
}
