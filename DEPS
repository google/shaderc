use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': '9866ad9195cec8f266f16191fb4ec2ce4896e5c0',
  'googletest_revision': '176eccfb8f42339b8ea2341e926f6835f7932bc4',
  're2_revision': '848dfb7e1d7ba641d598cb66f81590f3999a555a',
  'spirv_headers_revision': 'de99d4d834aeb51dd9f099baa285bd44fd04bb3d',
  'spirv_tools_revision': '9c0830133b07203a47ddc101fa4b298bab4438d8',
  'spirv_cross_revision': '4104e363005a079acc215f0920743a8affb31278',
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
