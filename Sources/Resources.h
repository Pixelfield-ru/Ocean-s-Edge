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
        static inline std::vector<std::string> m_bricksBreakBuffersNames = {
                "bricks_brake",
                "stone0_brake",
                "stone1_brake",
                "stone2_brake",
                "stone3_brake"
        };


        static inline std::vector<std::string> m_grassWithMudBreakBuffersNames = {
                "grass_w_mud_brake",
                "grass_w_mud_brake0",
                "grass_w_mud_brake1"
        };

        static inline std::vector<std::string> m_stoneBreakBuffersNames = {
                "stone0_brake",
                "stone1_brake",
                "stone2_brake",
                "stone3_brake"
        };
        
        static inline std::vector<std::string> m_ambientBuffersNames = {
                "amb_bird0",
                "amb_bird1",
                "amb_bird2",
                "amb_bird3",
                "amb_bird4",
                "amb_bird5",
                "amb_bird6"
        };


        SG_NOINLINE static auto getBlocksAtlas() noexcept
        {
            return res_blocksAtlas;
        }

        SG_NOINLINE static auto& getAudioBuffersMap() noexcept
        {
            return m_audioBuffers;
        }
        
        static void init()
        {
            res_blocksAtlas = SGCore::AssetManager::getInstance()->loadAsset<SGCore::ITexture2D>(OE_BLOCKS_ATLAS_PATH);
            
            m_audioBuffers["bricks_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/bricks.ogg");
            m_audioBuffers["grass_w_mud_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/grass_w_mud.ogg");
            m_audioBuffers["grass_w_mud_brake0"] = createAudioBufferByPath("../OEResources/audio/block_break/grass_w_mud0.ogg");
            m_audioBuffers["grass_w_mud_brake1"] = createAudioBufferByPath("../OEResources/audio/block_break/grass_w_mud1.ogg");
            m_audioBuffers["stone0_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/stone0.ogg");
            m_audioBuffers["stone1_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/stone1.ogg");
            m_audioBuffers["stone2_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/stone2.ogg");
            m_audioBuffers["stone3_brake"] = createAudioBufferByPath("../OEResources/audio/block_break/stone3.ogg");
            
            m_audioBuffers["amb_bird0"] = createAudioBufferByPath("../OEResources/audio/ambient/b1.ogg");
            m_audioBuffers["amb_bird1"] = createAudioBufferByPath("../OEResources/audio/ambient/b2.ogg");
            m_audioBuffers["amb_bird2"] = createAudioBufferByPath("../OEResources/audio/ambient/b3.ogg");
            m_audioBuffers["amb_bird3"] = createAudioBufferByPath("../OEResources/audio/ambient/b4.ogg");
            m_audioBuffers["amb_bird4"] = createAudioBufferByPath("../OEResources/audio/ambient/b5.ogg");
            m_audioBuffers["amb_bird5"] = createAudioBufferByPath("../OEResources/audio/ambient/b6.ogg");
            m_audioBuffers["amb_bird6"] = createAudioBufferByPath("../OEResources/audio/ambient/b7.ogg");
        }
        
    private:
        static SGCore::Ref<SGCore::AudioBuffer> createAudioBufferByPath(const std::string& path) noexcept
        {
            SGCore::Ref<SGCore::AudioTrackAsset> audio =
                    SGCore::AssetManager::getInstance()->loadAsset<SGCore::AudioTrackAsset>(path);
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
