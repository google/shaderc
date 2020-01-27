use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '19ec0d2ff9059e8bc8ca0fc539597a771cefdc89',
  'googletest_revision': '10b1902d893ea8cc43c69541d70868f91af3646b',
  're2_revision': '85c014206aee9bc1730dc416b33609974ae3ff5f',
  'spirv_headers_revision': 'dc77030acc9c6fe7ca21fff54c5a9d7b532d7da6',
  'spirv_tools_revision': '1b3441036a8f178cb3b41c1aa222b652db522a88',
  'spirv_cross_revision': '68bf0f824cc9d4fa9e0844b8ecdc5fc271fa6b7a',
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
