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
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader


def simple_vertex_shader():
    return """#version 310 es
    void main() {
        gl_Position = vec4(1., 2., 3., 4.);
    }"""


def simple_hlsl_vertex_shader():
    # Use "main" so we don't have to specify -fentry-point
    return """float4 main() : SV_POSITION { return float4(1.0); } """


def simple_fragment_shader():
    return """#version 310 es
    void main() {
        gl_FragDepth = 10.;
    }"""


def simple_tessellation_control_shader():
    return """#version 440 core
    layout(vertices = 3) out;
    void main() { }"""


def simple_tessellation_evaluation_shader():
    return """#version 440 core
    layout(triangles) in;
    void main() { }"""


def simple_geometry_shader():
    return """#version 150 core
    layout (triangles) in;
    layout (line_strip, max_vertices = 4) out;
    void main() { }"""


def simple_compute_shader():
    return """#version 310 es
    void main() {
        uvec3 temp = gl_WorkGroupID;
    }"""

def simple_raygen_shader():
    return """#version 460
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

def simple_intersection_shader():
    return """#version 460
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

def simple_anyhit_shader():
    return """#version 460
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

def simple_miss_shader():
    return """#version 460
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

def simple_closest_shader():
    return """#version 460
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

def simple_callable_shader():
    return """#version 460
    #extension GL_EXT_ray_tracing : enable

    layout(location = 0) callableDataInEXT struct Params {
        float foo;
    } params;

    void main()
    {
        params.foo *= 2;
    }
    """

def simple_mesh_shader():
    return """#version 460
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

def simple_task_shader():
    return """#version 460
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


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithGlslExtension(expect.ValidObjectFile):
    """Tests -fshader-stage with .glsl extension."""

    shader = FileShader(simple_vertex_shader(), '.glsl')
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithHlslExtension(expect.ValidObjectFile):
    """Tests -fshader-stage with .hlsl extension."""

    uses_hlsl = True
    shader = FileShader(simple_hlsl_vertex_shader(), '.hlsl')
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithKnownExtension(expect.ValidObjectFile):
    """Tests -fshader-stage with known extension."""

    shader = FileShader(simple_fragment_shader(), '.frag')
    glslc_args = ['-c', '-fshader-stage=fragment', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithUnknownExtension(expect.ValidObjectFile):
    """Tests -fshader-stage with unknown extension."""

    shader = FileShader(simple_vertex_shader(), '.unknown')
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithNoExtension(expect.ValidObjectFile):
    """Tests -fshader-stage with no extension."""

    shader = FileShader(simple_vertex_shader(), '')
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestAllShaderStagesForSpv1p0(expect.ValidObjectFile):
    """Tests all possible -fshader-stage values for SPIR-V 1.0."""

    shader1 = FileShader(simple_vertex_shader(), '.glsl')
    shader2 = FileShader(simple_fragment_shader(), '.glsl')
    shader3 = FileShader(simple_tessellation_control_shader(), '.glsl')
    shader4 = FileShader(simple_tessellation_evaluation_shader(), '.glsl')
    shader5 = FileShader(simple_geometry_shader(), '.glsl')
    shader6 = FileShader(simple_compute_shader(), '.glsl')
    glslc_args = [
        '-c',
        '-fshader-stage=vertex', shader1,
        '-fshader-stage=fragment', shader2,
        '-fshader-stage=tesscontrol', shader3,
        '-fshader-stage=tesseval', shader4,
        '-fshader-stage=geometry', shader5,
        '-fshader-stage=compute', shader6]


@inside_glslc_testsuite('OptionShaderStage')
class TestAllShaderStagesForSpv1p4(expect.ValidObjectFile1_4):
    """Tests all possible -fshader-stage values that only target at least SPIR-V 1.4"""

    raygen = FileShader(simple_raygen_shader(), '.glsl')
    intersection = FileShader(simple_intersection_shader(), '.glsl')
    anyhit = FileShader(simple_anyhit_shader(), '.glsl')
    miss = FileShader(simple_miss_shader(), '.glsl')
    closest = FileShader(simple_closest_shader(), '.glsl')
    miss = FileShader(simple_miss_shader(), '.glsl')
    callable_ = FileShader(simple_callable_shader(), '.glsl')
    mesh = FileShader(simple_mesh_shader(), '.glsl')
    task = FileShader(simple_task_shader(), '.glsl')
    glslc_args = [
        '-c',
        '--target-spv=spv1.4',
        '-fshader-stage=raygen', raygen,
        '-fshader-stage=intersect', intersection,
        '-fshader-stage=anyhit', anyhit,
        '-fshader-stage=closest', closest,
        '-fshader-stage=miss', miss,
        '-fshader-stage=callable', callable_,
        '-fshader-stage=mesh', mesh,
        '-fshader-stage=task', task]


@inside_glslc_testsuite('OptionShaderStage')
class TestAllShaderStagesForSpv1p4ShortNames(expect.ValidObjectFile1_4):
    """Tests all possible -fshader-stage values that only target at least SPIR-V 1.4"""

    raygen = FileShader(simple_raygen_shader(), '.glsl')
    intersection = FileShader(simple_intersection_shader(), '.glsl')
    anyhit = FileShader(simple_anyhit_shader(), '.glsl')
    miss = FileShader(simple_miss_shader(), '.glsl')
    closest = FileShader(simple_closest_shader(), '.glsl')
    callable_ = FileShader(simple_callable_shader(), '.glsl')
    glslc_args = [
        '-c',
        '--target-spv=spv1.4',
        '-fshader-stage=rgen', raygen,
        '-fshader-stage=rint', intersection,
        '-fshader-stage=rahit', anyhit,
        '-fshader-stage=rchit', closest,
        '-fshader-stage=rmiss', miss,
        '-fshader-stage=rcall', callable_]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageOverwriteFileExtension(expect.ValidObjectFile):
    """Tests -fshader-stage has precedence over file extension."""

    # a vertex shader camouflaged with .frag extension
    shader = FileShader(simple_vertex_shader(), '.frag')
    # Command line says it's vertex shader. Should compile successfully
    # as a vertex shader.
    glslc_args = ['-c', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageLatterOverwriteFormer(expect.ValidObjectFile):
    """Tests a latter -fshader-stage overwrite a former one."""

    shader = FileShader(simple_vertex_shader(), '.glsl')
    glslc_args = [
        '-c', '-fshader-stage=fragment', '-fshader-stage=vertex', shader]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWithMultipleFiles(expect.ValidObjectFile):
    """Tests -fshader-stage covers all subsequent files."""

    shader1 = FileShader(simple_vertex_shader(), '.glsl')
    # a vertex shader with .frag extension
    shader2 = FileShader(simple_vertex_shader(), '.frag')
    shader3 = FileShader(simple_vertex_shader(), '.a_vert_shader')
    glslc_args = ['-c', '-fshader-stage=vertex', shader1, shader2, shader3]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageMultipleShaderStage(expect.ValidObjectFile):
    """Tests multiple -fshader-stage."""

    shader1 = FileShader(simple_vertex_shader(), '.glsl')
    shader2 = FileShader(simple_fragment_shader(), '.frag')
    shader3 = FileShader(simple_vertex_shader(), '.a_vert_shader')
    glslc_args = [
        '-c',
        '-fshader-stage=vertex', shader1,
        '-fshader-stage=fragment', shader2,
        '-fshader-stage=vertex', shader3]


@inside_glslc_testsuite('OptionShaderStage')
class TestFileExtensionBeforeShaderStage(expect.ValidObjectFile):
    """Tests that file extensions before -fshader-stage are not affected."""

    # before -fshader-stage
    shader1 = FileShader(simple_vertex_shader(), '.vert')
    # after -fshader-stage
    shader2 = FileShader(simple_fragment_shader(), '.frag')
    shader3 = FileShader(simple_fragment_shader(), '.vert')
    glslc_args = ['-c', shader1, '-fshader-stage=fragment', shader2, shader3]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageWrongShaderStageValue(expect.ErrorMessage):
    """Tests that wrong shader stage value results in an error."""

    shader = FileShader(simple_vertex_shader(), '.glsl')
    glslc_args = ['-c', '-fshader-stage=unknown', shader]
    expected_error = ["glslc: error: stage not recognized: 'unknown'\n"]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageGlslExtensionMissingShaderStage(expect.ErrorMessage):
    """Tests that missing -fshader-stage for .glsl extension results in
    an error."""

    shader = FileShader(simple_vertex_shader(), '.glsl')
    glslc_args = ['-c', shader]
    expected_error = [
        "glslc: error: '", shader,
        "': .glsl file encountered but no -fshader-stage specified ahead\n"]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageHlslExtensionMissingShaderStage(expect.ErrorMessage):
    """Tests that missing -fshader-stage for .hlsl extension results in
    an error."""

    uses_hlsl = True
    shader = FileShader(simple_hlsl_vertex_shader(), '.hlsl')
    glslc_args = ['-c', '-x', 'hlsl', shader]
    expected_error = [
        "glslc: error: '", shader,
        "': .hlsl file encountered but no -fshader-stage specified ahead\n"]


@inside_glslc_testsuite('OptionShaderStage')
class TestShaderStageUnknownExtensionMissingShaderStage(expect.ErrorMessage):
    """Tests that missing -fshader-stage for unknown extension results in
    an error."""

    shader = FileShader(simple_vertex_shader(), '.a_vert_shader')
    glslc_args = ['-c', shader]
    expected_error = [
        "glslc: error: '", shader,
        "': file not recognized: File format not recognized\n"]
