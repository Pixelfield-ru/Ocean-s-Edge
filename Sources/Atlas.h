//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_ATLAS_H
#define OCEANSEDGE_ATLAS_H

#include <SGCore/Graphics/API/ITexture2D.h>
#include <SGCore/Memory/AssetManager.h>

namespace OceansEdge
{
    struct Atlas
    {
        static void init() noexcept
        {
            m_atlasTexture = SGCore::AssetManager::loadAsset<SGCore::ITexture2D>(
                    "../OEResources/textures/atlas.png"
            );
        }
        
        static auto getAtlas() noexcept
        {
            return m_atlasTexture;
        }
        
    private:
        static inline SGCore::Ref<SGCore::ITexture2D> m_atlasTexture;
    };
}

#endif //OCEANSEDGE_ATLAS_H
