//
// Created by ilya on 10.04.24.
//

#ifndef OCEANSEDGE_RESOURCES_H
#define OCEANSEDGE_RESOURCES_H

#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/Graphics/API/ITexture2D.h>
#include <SGCore/Memory/AssetManager.h>
#include "Defines.h"

namespace OceansEdge
{
    struct Resources
    {
        SG_NOINLINE static auto getBlocksAtlas() noexcept
        {
            return res_blocksAtlas;
        }
        
        static void init()
        {
            res_blocksAtlas = SGCore::AssetManager::loadAsset<SGCore::ITexture2D>(OE_BLOCKS_ATLAS_PATH);
        }
        
    private:
        static inline SGCore::Ref<SGCore::ITexture2D> res_blocksAtlas;
    };
}

#endif //OCEANSEDGE_RESOURCES_H
