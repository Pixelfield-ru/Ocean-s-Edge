#sg_pragma once

#sg_include "../../../SGResources/shaders/glsl4/uniform_bufs_decl.glsl"
#sg_include "../../../SGResources/shaders/glsl4/color_correction/aces.glsl"

vec3 fogColor = vec3(40.0, 40.0, 40.0);
vec3 minFogColor = vec3(0.2, 0.2, 0.2);
vec3 maxFogColor = vec3(0.9, 0.9, 0.9);

vec3 calculateFog(vec3 vertexGlobalPosition)
{
    float fogCoeff = saturate(1.0 / distance(camera.position, vertexGlobalPosition));
    vec3 fog = min(vec3(fogCoeff) * fogColor, maxFogColor);

    return fog;
}
