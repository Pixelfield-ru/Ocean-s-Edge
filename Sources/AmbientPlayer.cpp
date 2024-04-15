//
// Created by ilya on 15.04.24.
//

#include <random>
#include <SGCore/Audio/AudioSource.h>

#include "AmbientPlayer.h"
#include "Resources.h"

OceansEdge::AmbientPlayer::AmbientPlayer()
{
    m_timerRandom = std::mt19937(m_timerRandomDevice());
    m_timerRandomDistribution = std::uniform_int_distribution<std::mt19937::result_type>(m_minReloadTime, m_maxReloadTime);
    
    m_audioRandom = std::mt19937(m_audioRandomDevice());
    m_audioRandomDistribution = std::uniform_int_distribution<std::mt19937::result_type>(0, 6);
    
    m_playAmbientTimer.setTargetTime(m_timerRandomDistribution(m_timerRandom));
    
    m_playAmbientTimer.m_cyclic = true;
    m_playAmbientTimer.onUpdate += onTimerFinishedListener;
}

void OceansEdge::AmbientPlayer::onAddToScene(const SGCore::Ref<SGCore::Scene>& scene)
{
    if(!scene) return;
    
    m_audioEntitiesPool = SGCore::EntitiesPool(scene->getECSRegistry());
}

void OceansEdge::AmbientPlayer::update(const double& dt, const double& fixedDt)
{
    m_playAmbientTimer.startFrame();
}

void OceansEdge::AmbientPlayer::onTimerFinished() noexcept
{
    playAmbient(Resources::m_ambientBuffersNames[m_audioRandomDistribution(m_audioRandom)]);
    m_playAmbientTimer.setTargetTime(m_timerRandomDistribution(m_timerRandom));
}

void OceansEdge::AmbientPlayer::playAmbient(const std::string& audioBufferName)
{
    SGCore::AudioSource* audioSource = nullptr;
    
    bool isCreatedNew;
    SGCore::entity_t audioEntity = m_audioEntitiesPool.pop(isCreatedNew);
    auto poolRegistry = m_audioEntitiesPool.getAttachedRegistry();
    if(isCreatedNew)
    {
        if(poolRegistry)
        {
            audioSource = &poolRegistry->emplace<SGCore::AudioSource>(audioEntity);
        }
    }
    else
    {
        if(poolRegistry)
        {
            audioSource = &poolRegistry->get<SGCore::AudioSource>(audioEntity);
        }
    }
    
    if(audioSource && isCreatedNew)
    {
        audioSource->create();
        audioSource->onStateChanged += [this, audioEntity](SGCore::AudioSource& as, SGCore::AudioSourceState lastState,
                                                           SGCore::AudioSourceState newState)
        {
            if(lastState == SGCore::AudioSourceState::SOURCE_PLAYING && newState == SGCore::AudioSourceState::SOURCE_STOPPED)
            {
                m_audioEntitiesPool.push(audioEntity);
            }
        };
    }
    
    if(audioSource)
    {
        audioSource->setType(SGCore::AudioSourceType::AST_AMBIENT);
        audioSource->attachBuffer(Resources::getAudioBuffersMap()[audioBufferName]);
        audioSource->setState(SGCore::AudioSourceState::SOURCE_PLAYING);
    }
}
