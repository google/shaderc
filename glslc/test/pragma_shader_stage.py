# Copyright 2015 The Shaderc Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import expect
from environment import Directory, File
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader, StdinShader


VERTEX_ONLY_SHADER_WITH_PRAGMA = \
    """#version 310 es
    #pragma shader_stage(vertex)
    void main() {
        gl_Position = vec4(1.);
    }"""

FRAGMENT_ONLY_SHADER_WITH_PRAGMA = \
    """#version 310 es
    #pragma shader_stage(fragment)
    void main() {
        gl_FragDepth = 10.;
    }"""


TESS_CONTROL_ONLY_SHADER_WITH_PRAGMA = \
    """#version 440 core
    #pragma shader_stage(tesscontrol)
    layout(vertices = 3) out;
    void main() { }"""


TESS_EVAL_ONLY_SHADER_WITH_PRAGMA = \
    """#version 440 core
    #pragma shader_stage(tesseval)
    layout(triangles) in;
    void main() { }"""


GEOMETRY_ONLY_SHDER_WITH_PRAGMA = \
    """#version 150 core
    #pragma shader_stage(geometry)
    layout (triangles) in;
    layout (line_strip, max_vertices = 4) out;
    void main() { }"""


COMPUTE_ONLY_SHADER_WITH_PRAGMA = \
    """#version 310 es
    #pragma shader_stage(compute)
    void main() {
        uvec3 temp = gl_WorkGroupID;
    }"""


RAYGEN_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(raygen)
    #extension GL_EXT_ray_tracing : enable

    // 1. Top-Level Acceleration Structure (TLAS) containing your scene geometry
    layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

    // 2. Storage image to write the output colors to
    layout(binding = 1, set = 0, rgba8) uniform writeonly image2D imageOutput;

    // 3. Payload layout to receive ray tracing result (matches hit/miss shaders)
    layout(location = 0) rayPayloadEXT vec3 hitColor;

    void main()
    {
        // Compute pixel coordinates based on launch ID
        ivec2 pixel = ivec2(gl_LaunchIDEXT.xy);
        vec2  uv    = vec2(gl_LaunchIDEXT.xy) / vec2(gl_LaunchSizeEXT.xy);

        // Simple orthographic/normalized ray setup
        vec3 origin    = vec3(uv * 2.0 - 1.0, -1.0); // Normalized Screen Space to World [-1, 1]
        vec3 direction = vec3(0.0, 0.0, 1.0);        // Ray pointing forward into screen

        uint  rayFlags = gl_RayFlagsOpaqueEXT;
        uint  cullMask = 0xFF; // Trace all geometries
        float tMin     = 0.001;
        float tMax     = 1000.0;

        // Trace the ray into the acceleration structure
        traceRayEXT(
            topLevelAS,  // TLAS handle
            rayFlags,    // Ray behavior flags
            cullMask,    // Visibility mask
            0, 0, 0,     // SBT offset, stride, miss index
            origin,      // Ray Origin (vec3)
            tMin,        // Minimum intersection distance
            direction,   // Ray Direction (vec3)
            tMax,        // Maximum intersection distance
            0            // Payload location index
        );

        // Store the resulting color into the output image
        imageStore(imageOutput, pixel, vec4(hitColor, 1.0));
    }
    """

INTERSECTION_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(intersect)
    #extension GL_EXT_ray_tracing : enable

    // Intersection payload/attribute sent to Closest Hit shader
    hitAttributeEXT vec3 normal;

    void main()
    {
        // Built-in ray parameters supplied by Vulkan API
        vec3 rayOrigin    = gl_WorldRayOriginEXT;
        vec3 rayDirection = gl_WorldRayDirectionEXT;
        float tMin        = gl_RayTminEXT;
        float tMax        = gl_RayTmaxEXT;

        // Define a sphere centered at (0,0,0) with radius 1.0
        float radius = 1.0;

        // Ray-Sphere Intersection Math (Quadratic Equation: a*t^2 + b*t + c = 0)
        float a = dot(rayDirection, rayDirection);
        float b = 2.0 * dot(rayOrigin, rayDirection);
        float c = dot(rayOrigin, rayOrigin) - (radius * radius);

        float discriminant = b * b - 4.0 * a * c;

        if (discriminant >= 0.0)
        {
            // Calculate nearest intersection point distance (t)
            float t = (-b - sqrt(discriminant)) / (2.0 * a);

            // Check if the intersection point lies within the valid ray range [tMin, tMax]
            if (t >= tMin && t <= tMax)
            {
                // Compute hit normal to pass along to Closest Hit Shader
                normal = normalize(rayOrigin + t * rayDirection);

                // Report the intersection to Vulkan (0 is a user-defined hit type)
                reportIntersectionEXT(t, 0);
            }
        }
    }
    """

ANYHIT_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(anyhit)
    #extension GL_EXT_ray_tracing : enable
    #extension GL_EXT_nonuniform_qualifier : enable

    // Texture sampler layout (e.g., alpha mask)
    layout(binding = 2, set = 0) uniform sampler2D alphaTextures[];

    // Attributes received from Intersection Shader or Built-in Triangle barycentrics
    hitAttributeEXT vec2 hitAttribs;

    void main()
    {
        // Compute UVs using barycentric coordinates (for triangle geometry)
        vec3 barycentrics = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

        // Fetch UV coordinates or material ID for this instance/primitive
        // (Assuming simple UV mapping from vertex attribute data)
        vec2 uv = hitAttribs;

        // Sample the alpha channel from texture
        float alpha = texture(alphaTextures[gl_InstanceCustomIndexEXT], uv).a;

        // Alpha cutoff threshold
        if (alpha < 0.5)
        {
            // Reject this intersection candidate; traversal continues along the ray
            ignoreIntersectionEXT;
        }

        // If not ignored, Vulkan accepts this candidate hit by default.
    }
    """

MISS_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(miss)
    #extension GL_EXT_ray_tracing : enable

    // Payload output sent back to the Ray Generation Shader
    // (Location index MUST match the payload location index passed to traceRayEXT)
    layout(location = 0) rayPayloadInEXT vec3 hitColor;

    void main()
    {
        // Get the normalized direction vector of the ray that missed
        vec3 rayDir = normalize(gl_WorldRayDirectionEXT);

        // Create a simple vertical sky gradient (blue to white) based on Y direction
        float t = 0.5 * (rayDir.y + 1.0);
        vec3 skyColor = mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t);

        // Write output color into payload
        hitColor = skyColor;
    }
    """

CLOSEST_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(closest)
    #extension GL_EXT_ray_tracing : enable

    // Payload received from Raygen (location MUST match traceRayEXT call)
    layout(location = 0) rayPayloadInEXT vec3 hitColor;

    // Attributes passed from hardware ray-triangle intersection (Barycentric coordinates)
    hitAttributeEXT vec2 hitAttribs;

    void main()
    {
        // 1. Calculate barycentric coordinates for the hit point on the triangle
        vec3 barycentrics = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

        // 2. Derive surface normal (For demo purposes: using flat normal from triangle hit)
        // In a production setup, you would fetch vertex normals using gl_PrimitiveID / SSBOs
        vec3 objectNormal = vec3(0.0, 1.0, 0.0);
        vec3 worldNormal  = normalize(gl_ObjectToWorldEXT * vec4(objectNormal, 0.0));

        // 3. Define a simple directional light source
        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
        vec3 baseColor = vec3(0.8, 0.2, 0.2); // Red material

        // 4. Compute diffuse lighting (N dot L)
        float diffuse = max(dot(worldNormal, lightDir), 0.1); // 0.1 ambient floor

        // 5. Output shaded color to payload
        hitColor = baseColor * diffuse;
    }
    """


CALLABLE_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(callable)
    #extension GL_EXT_ray_tracing : enable

    layout(location = 0) callableDataInEXT struct Params {
        float foo;
    } params;

    void main()
    {
        params.foo *= 2;
    }
    """

MESH_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(mesh)
    #extension GL_EXT_mesh_shader : enable

    layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

    // Output configuration: primitive type, max vertex count, max primitive count
    layout(triangles, max_vertices = 3, max_primitives = 1) out;

    // Custom per-vertex outputs to the fragment shader
    layout(location = 0) out vec3 outColor[];

    void main()
    {
        // Set total vertices and primitives emitted by this meshlet thread group
        SetMeshOutputsEXT(3, 1);

        // 1. Define Vertex Positions (in Normalized Device Coordinates [-1, 1])
        gl_MeshVerticesEXT[0].gl_Position = vec4(-0.5, -0.5, 0.0, 1.0);
        gl_MeshVerticesEXT[1].gl_Position = vec4( 0.5, -0.5, 0.0, 1.0);
        gl_MeshVerticesEXT[2].gl_Position = vec4( 0.0,  0.5, 0.0, 1.0);

        // 2. Pass Per-Vertex Attributes
        outColor[0] = vec3(1.0, 0.0, 0.0); // Red
        outColor[1] = vec3(0.0, 1.0, 0.0); // Green
        outColor[2] = vec3(0.0, 0.0, 1.0); // Blue

        // 3. Define Primitive Index Topologies (1 triangle using 3 vertex indices)
        gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
    }
    """


TASK_ONLY_SHADER_WITH_PRAGMA = \
    """#version 460
    #pragma shader_stage(task)
    #extension GL_EXT_mesh_shader : enable

    // Define workgroup dimensions for task shader evaluation
    layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

    // Payload structure passed from Task Shader to Mesh Shader
    struct TaskPayload {
        uint meshletID;
    };
    taskPayloadSharedEXT TaskPayload payload;

    void main()
    {
        // Local meshlet index to test
        uint meshletIndex = gl_GlobalInvocationID.x;

        // 1. Perform Culling Test (Frustum, Occlusion, or Backface)
        bool isVisible = true; // Placeholder for actual culling logic

        if (isVisible)
        {
            // 2. Populate payload data shared with spawned Mesh Shaders
            payload.meshletID = meshletIndex;

            // 3. Emit task count: Launch 1 Mesh Shader workgroup (X=1, Y=1, Z=1)
            EmitMeshTasksEXT(1, 1, 1);
        }
        else
        {
            // Cull completely: Do not spawn any Mesh Shader workgroups
            EmitMeshTasksEXT(0, 0, 0);
        }
    }
    """

# In the following tests,
# PSS stands for PragmaShaderStage, and OSS stands for OptionShaderStage.


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithGlslExtension(expect.ValidObjectFile):
    """Tests #pragma shader_stage() with .glsl extension."""

    shader = FileShader(VERTEX_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithUnkownExtension(expect.ValidObjectFile):
    """Tests #pragma shader_stage() with unknown extension."""

    shader = FileShader(VERTEX_ONLY_SHADER_WITH_PRAGMA, '.unkown')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithStdin(expect.ValidObjectFile):
    """Tests #pragma shader_stage() with stdin."""

    shader = StdinShader(VERTEX_ONLY_SHADER_WITH_PRAGMA)
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithSameShaderExtension(expect.ValidObjectFile):
    """Tests that #pragma shader_stage() specifies the same stage as file
    extesion."""

    shader = FileShader(VERTEX_ONLY_SHADER_WITH_PRAGMA, '.vert')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSOverrideShaderExtension(expect.ValidObjectFile):
    """Tests that #pragma shader_stage() overrides file extension."""

    shader = FileShader(VERTEX_ONLY_SHADER_WITH_PRAGMA, '.frag')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestOSSOverridePSS(expect.ValidObjectFile):
    """Tests that -fshader-stage overrides #pragma shader_stage()."""

    # wrong pragma and wrong file extension
    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(fragment)
        void main() {
            gl_Position = vec4(1.);
        }""", '.frag')
    # -fshader-stage to the rescue! ^.^
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestMultipleSamePSS(expect.ValidObjectFile):
    """Tests that multiple identical #pragma shader_stage() works."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(vertex)
        #pragma shader_stage(vertex)
        void main() {
        #pragma shader_stage(vertex)
            gl_Position = vec4(1.);
        #pragma shader_stage(vertex)
        }
        #pragma shader_stage(vertex)
        #pragma shader_stage(vertex)
        """, '.glsl')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestConflictingPSS(expect.ErrorMessage):
    """Conflicting #pragma shader_stage() directives result in an error."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(vertex)
        void main() {
            gl_Position = vec4(1.);
        }
        #pragma shader_stage(fragment)
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        shader, ":6: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'fragment' (was 'vertex' at ", shader, ':2)\n']


@inside_glslc_testsuite('PragmaShaderStage')
class TestAllPSSValueForSpv1p0(expect.ValidObjectFile):
    """Tests all possible #pragma shader_stage() values that can
    target SPIR-V 1.0."""

    shader1 = FileShader(VERTEX_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader2 = FileShader(FRAGMENT_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader3 = FileShader(TESS_CONTROL_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader4 = FileShader(TESS_EVAL_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader5 = FileShader(GEOMETRY_ONLY_SHDER_WITH_PRAGMA, '.glsl')
    shader6 = FileShader(COMPUTE_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    glslc_args = ['-c', shader1, shader2, shader3, shader4, shader5, shader6]

@inside_glslc_testsuite('PragmaShaderStage')
class TestAllPSSValueForSpv1p4(expect.ValidObjectFile1_4):
    """Tests all possible #pragma shader_stage() values that can only
    target SPIR-V 1.4 or later."""

    shader1 = FileShader(RAYGEN_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader2 = FileShader(INTERSECTION_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader3 = FileShader(ANYHIT_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader4 = FileShader(MISS_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader5 = FileShader(CLOSEST_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader6 = FileShader(CALLABLE_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader7 = FileShader(MESH_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    shader8 = FileShader(TASK_ONLY_SHADER_WITH_PRAGMA, '.glsl')
    glslc_args = ['-c', '--target-spv=spv1.4', \
                  shader1, shader2, shader3, shader4, \
                  shader5, shader6, shader7, shader8]


@inside_glslc_testsuite('PragmaShaderStage')
class TestWrongPSSValue(expect.ErrorMessage):
    """Tests that #pragma shader_stage([wrong-stage]) results in an error."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(superstage)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'superstage'\n"]


@inside_glslc_testsuite('PragmaShaderStage')
class TestEmptyPSSValue(expect.ErrorMessage):
    """Tests that #pragma shader_stage([empty]) results in an error."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage()
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: ''\n"]


@inside_glslc_testsuite('PragmaShaderStage')
class TestFirstPSSBeforeNonPPCode(expect.ErrorMessage):
    """Tests that the first #pragma shader_stage() should appear before
    any non-preprocessing code."""

    shader = FileShader(
        """#version 310 es
        #ifndef REMOVE_UNUSED_FUNCTION
          int inc(int i) { return i + 1; }
        #endif
        #pragma shader_stage(vertex)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        shader, ":5: error: '#pragma': the first 'shader_stage' #pragma "
        'must appear before any non-preprocessing code\n']


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSMultipleErrors(expect.ErrorMessage):
    """Tests that if there are multiple errors, they are all reported."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(idontknow)
        #pragma shader_stage(vertex)
        void main() {
            gl_Position = vec4(1.);
        }
        #pragma shader_stage(fragment)
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'idontknow'\n",
        shader, ":3: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'vertex' (was 'idontknow' at ", shader, ':2)\n',
        shader, ":7: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'fragment' (was 'idontknow' at ", shader, ':2)\n']


@inside_glslc_testsuite('PragmaShaderStage')
class TestSpacesAroundPSS(expect.ValidObjectFile):
    """Tests that spaces around #pragma shader_stage() works."""

    shader = FileShader(
        """#version 310 es
               #     pragma      shader_stage     (    vertex     )
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestTabsAroundPSS(expect.ValidObjectFile):
    """Tests that tabs around #pragma shader_stage() works."""

    shader = FileShader(
        """#version 310 es
        \t\t#\tpragma\t\t\tshader_stage\t\t(\t\t\t\tvertex\t\t)\t\t\t\t
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithMacro(expect.ValidObjectFile):
    """Tests that #pragma shader_stage() works with macros."""

    shader = FileShader(
        """#version 310 es
        #if 0
          some random stuff here which can cause a problem
        #else
        # pragma shader_stage(vertex)
        #endif
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithCmdLineMacroDef(expect.ValidObjectFile):
    """Tests that macro definitions passed in from command line work."""

    shader = FileShader(
        """#version 310 es
        #ifdef IS_A_VERTEX_SHADER
        # pragma shader_stage(vertex)
        #else
        # pragma shader_stage(fragment)
        #endif
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', '-DIS_A_VERTEX_SHADER', shader]


@inside_glslc_testsuite('PragmaShaderStage')
class TestNoMacroExpansionInsidePSS(expect.ErrorMessage):
    """Tests that there is no macro expansion inside #pragma shader_stage()."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(STAGE_FROM_CMDLINE)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', '-DSTAGE_FROM_CMDLINE=vertex', shader]

    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'STAGE_FROM_CMDLINE'\n"]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithPoundLine310(expect.ErrorMessage):
    """Tests that #pragma shader_stage() works with #line."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(unknown)
        #line 42
        #pragma shader_stage(google)
        #line 100
        #pragma shader_stage(elgoog)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]

    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'unknown'\n",
        shader, ":42: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'google' (was 'unknown' at ", shader, ':2)\n',
        shader, ":100: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'elgoog' (was 'unknown' at ", shader, ':2)\n']


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithPoundLine150(expect.ErrorMessage):
    """Tests that #pragma shader_stage() works with #line.

    For older desktop versions, a #line directive specify the line number of
    the #line directive, not the next line.
    """

    shader = FileShader(
        """#version 150
        #pragma shader_stage(unknown)
        #line 42
        #pragma shader_stage(google)
        #line 100
        #pragma shader_stage(elgoog)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]

    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'unknown'\n",
        shader, ":43: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'google' (was 'unknown' at ", shader, ':2)\n',
        shader, ":101: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'elgoog' (was 'unknown' at ", shader, ':2)\n']


@inside_glslc_testsuite('PragmaShaderStage')
class ErrorBeforePragma(expect.ErrorMessage):
    """Tests that errors before pragmas are emitted."""

    shader = FileShader(
        """#version 310 es
        #something
        #pragma shader_stage(vertex)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [shader, ':2: error: \'#\' : invalid directive:',
                      ' something\n'
                      '1 error generated.\n']


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSfromIncludedFile(expect.ValidObjectFile):
    """Tests that #pragma shader_stage() from included files works."""

    environment = Directory('.', [
        File('a.glsl', '#version 140\n#include "b.glsl"\n'
             'void main() { gl_Position = vec4(1.); }\n'),
        File('b.glsl', '#pragma shader_stage(vertex)')])
    glslc_args = ['-c', 'a.glsl']


@inside_glslc_testsuite('PragmaShaderStage')
class TestConflictingPSSfromIncludingAndIncludedFile(expect.ErrorMessage):
    """Tests that conflicting #pragma shader_stage() from including and
    included files results in an error with the correct location spec."""

    environment = Directory('.', [
        File('a.vert',
             '#version 140\n'
             '#pragma shader_stage(fragment)\n'
             'void main() { gl_Position = vec4(1.); }\n'
             '#include "b.glsl"\n'),
        File('b.glsl', '#pragma shader_stage(vertex)')])
    glslc_args = ['-c', 'a.vert']

    expected_error = [
        "b.glsl:1: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'vertex' (was 'fragment' at a.vert:2)\n"]


@inside_glslc_testsuite('PragmaShaderStage')
class TestPSSWithFileNameBasedPoundLine(expect.ErrorMessage):
    """Tests that #pragma shader_stage() works with filename-based #line."""

    shader = FileShader(
        """#version 310 es
        #pragma shader_stage(unknown)
        #line 42 "abc"
        #pragma shader_stage(google)
        #line 100 "def"
        #pragma shader_stage(elgoog)
        void main() {
            gl_Position = vec4(1.);
        }
        """, '.glsl')
    glslc_args = ['-c', shader]

    expected_error = [
        shader, ":2: error: '#pragma': invalid stage for 'shader_stage' "
        "#pragma: 'unknown'\n",
        "abc:42: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'google' (was 'unknown' at ", shader, ':2)\n',
        "def:100: error: '#pragma': conflicting stages for 'shader_stage' "
        "#pragma: 'elgoog' (was 'unknown' at ", shader, ':2)\n']
