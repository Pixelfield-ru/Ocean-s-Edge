//
// Created by ilya on 16.02.24.
//

#ifndef OCEANSEDGE_GAMEMAIN_H
#define OCEANSEDGE_GAMEMAIN_H

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
        static void update(const double& dt);
        
    private:
        static inline SGCore::Ref<SGCore::Scene> m_worldScene;
    };
}

#endif //OCEANSEDGE_GAMEMAIN_H
