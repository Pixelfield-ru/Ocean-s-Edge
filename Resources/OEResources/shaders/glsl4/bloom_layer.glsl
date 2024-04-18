#sg_pragma once

#sg_include "../SGResources/shaders/glsl4/postprocessing/layered/base.glsl"

#sg_include "../SGResources/shaders/glsl4/disks.glsl"
#sg_include "../SGResources/shaders/glsl4/math.glsl"
#sg_include "../SGResources/shaders/glsl4/random.glsl"
#sg_include "../SGResources/shaders/glsl4/color_correction/aces.glsl"

#define SG_NOT_INCLUDE_LIGHTS

#sg_include "../SGResources/shaders/glsl4/uniform_bufs_decl.glsl"

SGSubPass(SGLPPLayerFXPass)
{
    SGSubShader(Fragment)
    {
        in vec2 vs_UVAttribute;

        uniform sampler2D test_pp_layer_ColorAttachments[1];
        uniform int SGLPP_CurrentSubPassIndex;

        void main()
        {
            vec2 finalUV = vs_UVAttribute.xy;

            #ifdef FLIP_TEXTURES_Y
            finalUV.y = 1.0 - vs_UVAttribute.y;
            #endif

            vec4 col;

            int samplesCount = 14;

            float rand = random(finalUV);
            float rotAngle = rand * PI;
            vec2 rotTrig = vec2(cos(rotAngle), sin(rotAngle));

            for(int i = 0; i < samplesCount; ++i)
            {
                col += texture(test_pp_layer_ColorAttachments[0], saturate(finalUV + rotate(poissonDisk[i], rotTrig) / 75.0));
            }

            col /= 4;

            if(SGLPP_CurrentSubPassIndex == 0)
            {
                gl_FragColor = col;
                // gl_FragColor = texture(test_pp_layer_ColorAttachments[0], vec2(finalUV.x + currentTime, finalUV.y));
            }
        }
    }
}