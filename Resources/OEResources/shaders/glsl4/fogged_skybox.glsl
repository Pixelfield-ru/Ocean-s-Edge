#sg_pragma once

#sg_include "fog_calc.glsl"

SGSubPass(GeometryPass)
{
    SGSubShader(Vertex)
    {
        layout (location = 0) in vec3 positionsAttribute;
        layout (location = 1) in vec3 UVAttribute;
        layout (location = 2) in vec3 normalsAttribute;

        out vec3 vs_UVAttribute;

        void main()
        {
            vs_UVAttribute = positionsAttribute;

            gl_Position = camera.projectionMatrix * mat4(mat3(camera.viewMatrix)) * objectTransform.modelMatrix * vec4(positionsAttribute, 1.0);
        }
    }

    SGSubShader(Fragment)
    {
        layout(location = 0) out vec4 fragColor;

        in vec3 vs_UVAttribute;

        void main()
        {
            fragColor = vec4(minFogColor, 1.0);
        }
    }
}
