//
// Created by ilya on 16.02.24.
//

#ifndef OCEANSEDGE_GAMEMAIN_H
#define OCEANSEDGE_GAMEMAIN_H

#define NOMINMAX

#include "World.h"

int main();

namespace SGCore
{
    class Scene;
}

namespace OceansEdge
{
    struct GameMain
    {
        static void init();
        static void fixedUpdate(const double& dt, const double& fixedDt);
        static void update(const double& dt, const double& fixedDt);

        static SGCore::Ref<World> getCurrentWorld() noexcept;
        
    private:
        static inline SGCore::Ref<SGCore::Scene> m_worldScene;
        static inline SGCore::Ref<World> m_world;
    };
}

#endif //OCEANSEDGE_GAMEMAIN_H
