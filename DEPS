use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '4bef5dbed590d1edfd3e34bc83d4141f41b998b0',
  'glslang_revision': '71892a5eda90fd7d3d6ccc12745f066d0ca5dc5f',
  'googletest_revision': 'ee3aa831172090fd5442820f215cb04ab6062756',
  're2_revision': 'e356bd3f80e0c15c1050323bb5a2d0f8ea4845f4',
  'spirv_headers_revision': '29c11140baaf9f7fdaa39a583672c556bf1795a1',
  'spirv_tools_revision': '59de04ad687695c110daecfe45e9fd267b6ab0a7',
  'spirv_cross_revision': '94710fe608784c5c660112b9a9fb6f58b514e510',
}

deps = {
  'third_party/effcee': Var('google_git') + '/effcee.git@' +
      Var('effcee_revision'),

  'third_party/googletest': Var('google_git') + '/googletest.git@' +
      Var('googletest_revision'),

  'third_party/glslang': Var('khronos_git') + '/glslang.git@' +
      Var('glslang_revision'),

  'third_party/re2': Var('google_git') + '/re2.git@' +
      Var('re2_revision'),

  'third_party/spirv-headers': Var('khronos_git') + '/SPIRV-Headers.git@' +
      Var('spirv_headers_revision'),

  'third_party/spirv-tools': Var('khronos_git') + '/SPIRV-Tools.git@' +
      Var('spirv_tools_revision'),

  'third_party/spirv-cross': Var('khronos_git') + '/SPIRV-Cross.git@' +
      Var('spirv_cross_revision'),
}
