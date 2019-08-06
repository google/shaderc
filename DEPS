use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '4bef5dbed590d1edfd3e34bc83d4141f41b998b0',
  'glslang_revision': '3cea2e5882e3455731a8b6657fb06db913eb3aa1',
  'googletest_revision': 'b4961ab1cbda56295823594e4214b0c188052fef',
  're2_revision': '67bce690decdb507b13e14050661f8b9ebfcfe6c',
  'spirv_headers_revision': 'e4322e3be589e1ddd44afb20ea842a977c1319b8',
  'spirv_tools_revision': '698b56a8f02495d115cd6c5eefd89b812d55b13d',
  'spirv_cross_revision': '4ce04480ec5469fe7ebbdd66c3016090a704d81b',
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
