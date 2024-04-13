//
// Created by ilya on 13.04.24.
//

#ifndef OCEANSEDGE_PLAYERCONTROLLER_H
#define OCEANSEDGE_PLAYERCONTROLLER_H

#include <SGCore/Scene/ISystem.h>
#include <SGCore/Input/InputListener.h>

namespace OceansEdge
{
    struct PlayerController : public SGCore::ISystem
    {
        PlayerController();
        
        void update(const double& dt, const double& fixedDt) final;
        
    private:
        SGCore::Ref<SGCore::InputListener> m_playerInputListener = SGCore::MakeRef<SGCore::InputListener>();
    };
}

#endif //OCEANSEDGE_PLAYERCONTROLLER_H
