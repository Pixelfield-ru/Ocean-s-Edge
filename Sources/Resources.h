//
// Created by ilya on 10.04.24.
//

#ifndef OCEANSEDGE_RESOURCES_H
#define OCEANSEDGE_RESOURCES_H

#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/Graphics/API/ITexture2D.h>
#include <SGCore/Memory/AssetManager.h>
#include <SGCore/Memory/Assets/AudioTrackAsset.h>
#include <SGCore/Audio/AudioBuffer.h>
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
            
            m_audioBuffers["bricks_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/bricks.ogg");
        }
        
    private:
        static SGCore::Ref<SGCore::AudioBuffer> createAudioBufferByPath(const std::string& path) noexcept
        {
            SGCore::Ref<SGCore::AudioTrackAsset> audio =
                    SGCore::AssetManager::loadAsset<SGCore::AudioTrackAsset>(path);
            SGCore::Ref<SGCore::AudioBuffer> audioBuf =
                    SGCore::MakeRef<SGCore::AudioBuffer>();
            audioBuf->create();
            
            audioBuf->putData(audio->getAudioTrack().getNumChannels(),
                              audio->getAudioTrack().getBitsPerSample(),
                              audio->getAudioTrack().getDataBuffer(),
                              audio->getAudioTrack().getDataBufferSize(),
                              audio->getAudioTrack().getSampleRate());
            
            return audioBuf;
        }
        
        static inline std::unordered_map<std::string, SGCore::Ref<SGCore::AudioBuffer>> m_audioBuffers;
        static inline SGCore::Ref<SGCore::ITexture2D> res_blocksAtlas;
    };
}

#endif //OCEANSEDGE_RESOURCES_H
