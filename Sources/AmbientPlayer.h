//
// Created by ilya on 15.04.24.
//

#ifndef OCEANSEDGE_AMBIENTPLAYER_H
#define OCEANSEDGE_AMBIENTPLAYER_H

#include <SGCore/Scene/ISystem.h>
#include <SGUtils/Timer.h>
#include <SGCore/Utils/ECS/EntitiesPool.h>
#include <SGCore/Scene/Scene.h>

namespace OceansEdge
{
    struct AmbientPlayer : public SGCore::ISystem
    {
        float m_minReloadTime = 0.1f;
        float m_maxReloadTime = 45.0f;
        
        AmbientPlayer();
        
        void playAmbient(const std::string& audioBufferName);
        
        void onAddToScene(const SGCore::Ref<SGCore::Scene>& scene) final;
        void update(const double& dt, const double& fixedDt) final;
        
    private:
        SGCore::EntitiesPool m_audioEntitiesPool;
        
        std::random_device m_timerRandomDevice;
        std::mt19937 m_timerRandom;
        std::uniform_int_distribution<std::mt19937::result_type> m_timerRandomDistribution;
        
        std::random_device m_audioRandomDevice;
        std::mt19937 m_audioRandom;
        std::uniform_int_distribution<std::mt19937::result_type> m_audioRandomDistribution;
        
        SGCore::EventListener<void(const double&, const double&)> onTimerFinishedListener = [this](auto, auto) {
            onTimerFinished();
        };
        
        void onTimerFinished() noexcept;
        
        SGCore::Timer m_playAmbientTimer;
    };
}

#endif //OCEANSEDGE_AMBIENTPLAYER_H
