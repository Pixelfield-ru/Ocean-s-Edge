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

        uniform int SGLPP_CurrentSubPassIndex;

        uniform sampler2D chunksGBuf_Albedo;
        uniform sampler2D chunksGBuf_VerticesPositions;
        uniform sampler2D chunksGBuf_ViewModelNormals;

        // STANDARD CANDIDATES
        uniform int SG_SSAO_samplesCount;
        uniform vec3 SG_SSAO_samples[128];

        // STANDARD CANDIDATES
        uniform sampler2D SG_SSAO_noise;
        uniform sampler2D SG_SSAO_occlusionFormedTexture;
        uniform sampler2D SG_SSAO_occlusionBlurredTexture;
        uniform int SG_SSAO_ENABLED;

        void main()
        {
            vec2 finalUV = vs_UVAttribute.xy;

            #ifdef FLIP_TEXTURES_Y
            finalUV.y = 1.0 - vs_UVAttribute.y;
            #endif

            vec2 noiseScale = vec2(programData.primaryMonitorSize.x / 4.0, programData.primaryMonitorSize.y / 4.0);

            vec3 fragPos = texture(chunksGBuf_VerticesPositions, finalUV).xyz;

            vec3 modelNormal = texture(chunksGBuf_ViewModelNormals, finalUV).xyz;
            vec3 viewModelNormal = modelNormal;
            vec3 randomVec = texture(SG_SSAO_noise, finalUV * noiseScale).xyz;

            vec3 tangent = normalize(randomVec - viewModelNormal * dot(randomVec, viewModelNormal));
            vec3 bitangent = cross(viewModelNormal, tangent);
            mat3 TBN = mat3(tangent, bitangent, viewModelNormal);

            if(SGLPP_CurrentSubPassIndex == 0)
            {
                vec3 viewModelFragPos = fragPos;

                float occlusion = 0.0;
                float radius = 1;
                const float bias = 0.025;
                for (int i = 0; i < SG_SSAO_samplesCount; ++i)
                {
                    vec3 smp = TBN * SG_SSAO_samples[i];
                    smp = viewModelFragPos + smp * radius;

                    vec4 offset = vec4(smp, 1.0);
                    offset = camera.projectionMatrix * offset;// переход из видового  клиповое
                    offset.xyz /= offset.w;// перспективное деление
                    offset.xyz = offset.xyz * 0.5 + 0.5;

                    float sampleDepth = (texture(chunksGBuf_VerticesPositions, offset.xy)).z;
                    float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewModelFragPos.z - sampleDepth));
                    occlusion += (sampleDepth >= smp.z + bias ? 1.0 : 0.0) * rangeCheck;
                }

                occlusion = 1.0 - (occlusion / SG_SSAO_samplesCount);

                gl_FragColor = vec4(vec3(occlusion), 1.0);
            }
            else if(SGLPP_CurrentSubPassIndex == 1) // ssao blur pass
            {
                vec2 texelSize = 1.0 / vec2(textureSize(SG_SSAO_occlusionFormedTexture, 0));
                float result = 0.0;
                for (int x = -2; x < 2; ++x)
                {
                    for (int y = -2; y < 2; ++y)
                    {
                        vec2 offset = vec2(float(x), float(y)) * texelSize;
                        result += texture(SG_SSAO_occlusionFormedTexture, finalUV + offset).r;
                    }
                }

                vec4 albedo = texture(chunksGBuf_Albedo, finalUV);
                float ssaoCoeff = result / (4.0 * 4.0);
                if(SG_SSAO_ENABLED == 0)
                {
                    ssaoCoeff = 1.0;
                }

                gl_FragColor = ssaoCoeff * albedo;
            }
        }
    }
}