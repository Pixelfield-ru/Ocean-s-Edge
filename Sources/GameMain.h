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
        static void fixedUpdate();
        static void update();
        
    private:
        static inline SGCore::Ref<SGCore::Scene> m_worldScene;
    };
}

#endif //OCEANSEDGE_GAMEMAIN_H
