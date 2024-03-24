#sg_pragma once

SGSubPass(ChunksPass)
{
    SGSubShader(Vertex)
    {
        #sg_include "../../../SGResources/shaders/glsl4/uniform_bufs_decl.glsl"

        layout (location = 0) in vec3 vertexPosition;

        void main()
        {
            gl_Position = camera.projectionSpaceMatrix * vec4(vertexPosition, 1.0);
        }
    }

    SGSubShader(Fragment)
    {
        layout(location = 0) out vec4 fragColor;

        void main()
        {
            fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    }
}