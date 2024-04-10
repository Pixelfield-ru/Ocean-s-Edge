#sg_pragma once

#sg_include "fog_calc.glsl"
#sg_include "utils.glsl"

struct VS_OUT
{
    vec3 normal;
    vec3 vertexPosition;
    vec2 uv;
    float face;
    float blockType;
};

SGSubPass(ChunksPass)
{
    SGSubShader(Vertex)
    {
        layout (location = 0) in ivec2 vertexData;

        const vec3 normals[6] = vec3[] (
            vec3(0.0, 1.0, 0.0), // Y+
            vec3(0.0, -1.0, 0.0), // Y-
            vec3(1.0, 0.0, 0.0), // X+
            vec3(-1.0, 0.0, 0.0), // X-
            vec3(0.0, 0.0, 1.0), // Z+
            vec3(0.0, 0.0, -1.0) // Z-
        );

        uniform vec3 u_chunkPosition;

        out VS_OUT vsOut;

        void main()
        {
            int localVertexID = gl_VertexID - int(floor(float(gl_VertexID) / 4.0)) * 4;

            int vPosX = vertexData.x & 63;
            int vPosY = vertexData.y;
            // int vPosY = (vertexData >> 6) & 63;
            int vPosZ = (vertexData.x >> 6) & 63;
            int face = (vertexData.x >> 12) & 7;
            int blockType = (vertexData.x >> 15) & 65535;

            vec3 normal = normals[face];

            vec3 finalPos = u_chunkPosition + vec3(vPosX, vPosY, vPosZ);

            changeUVForCurrentVertex(blockType, face);

            vsOut.normal = normal;
            vsOut.vertexPosition = finalPos;
            vsOut.uv = currentUVs[localVertexID];
            vsOut.face = face;

            gl_Position = camera.projectionSpaceMatrix * vec4(finalPos, 1.0);
        }
    }

    SGSubShader(Fragment)
    {
        layout(location = 0) out vec4 fragColor;

        in VS_OUT vsOut;

        uniform sampler2D u_blocksAtlas;

        void main()
        {
            vec3 lightDir = normalize(atmosphere.sunPosition);
            float diff = max(dot(vsOut.normal, lightDir), 0.0);
            vec3 diffuse = diff * atmosphere.sunColor;

            vec3 diffuseCol = clamp(diffuse * calculateFog(vsOut.vertexPosition), minFogColor, vec3(1.0));

            vec4 atlasCol = texture(u_blocksAtlas, vsOut.uv);

            fragColor = vec4(atlasCol.rgb * diffuseCol, 1.0);
            // fragColor = vec4(vsOut.uv, 0.0, 1.0);
            // fragColor = vec4(diffuseCol, 1.0);
        }
    }
}