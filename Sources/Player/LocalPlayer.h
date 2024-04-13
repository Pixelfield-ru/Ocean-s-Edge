//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_PLAYER_H
#define OCEANSEDGE_PLAYER_H

namespace OceansEdge
{
    struct LocalPlayer
    {
        std::uint16_t m_currentSelectedBlockType = BlocksTypes::OEB_MUD_WITH_GRASS;
        
    private:
        bool m_dummy = false;
    };
}

#endif //OCEANSEDGE_PLAYER_H
