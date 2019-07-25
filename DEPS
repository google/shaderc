use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '4bef5dbed590d1edfd3e34bc83d4141f41b998b0',
  'glslang_revision': '42f813401bdfa640e4b87d8991e63f97fe5d5c43',
  'googletest_revision': 'b77e5c76252bac322bb82c5b444f050bd0d92451',
  're2_revision': 'f1a1156294f0337beb75518da3606091f3905fb1',
  'spirv_headers_revision': 'e4322e3be589e1ddd44afb20ea842a977c1319b8',
  'spirv_tools_revision': 'f54b8653dd9d4fb2068c7203b148b4276405bea5',
  'spirv_cross_revision': '301eab1b7a755865f2e7c26af9eddae8dc486c28',
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
