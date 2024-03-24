#sg_pragma once

#sg_include "../../../SGResources/shaders/glsl4/uniform_bufs_decl.glsl"

struct VS_OUT
{
    vec3 normal;
    vec3 vertexPosition;
};

SGSubPass(ChunksPass)
{
    SGSubShader(Vertex)
    {
        layout (location = 0) in int vertexData;

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
            int vPosX = vertexData & 63;
            int vPosY = (vertexData >> 6) & 63;
            int vPosZ = (vertexData >> 12) & 63;
            int face = (vertexData >> 18) & 7;

            vec3 normal = normals[face];

            vec3 finalPos = u_chunkPosition + vec3(vPosX, vPosY, vPosZ);

            vsOut.normal = normal;
            vsOut.vertexPosition = finalPos;

            gl_Position = camera.projectionSpaceMatrix * vec4(finalPos, 1.0);
        }
    }

    SGSubShader(Fragment)
    {
        layout(location = 0) out vec4 fragColor;

        in VS_OUT vsOut;

        void main()
        {
            vec3 lightDir = normalize(atmosphere.sunPosition);
            float diff = max(dot(vsOut.normal, lightDir), 0.0);
            vec3 diffuse = diff * atmosphere.sunColor;

            fragColor = vec4(diffuse, 1.0);
        }
    }
}