use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': '9866ad9195cec8f266f16191fb4ec2ce4896e5c0',
  'googletest_revision': '076b7f7788833ca31206bc30e5a2cfbdb9628f29',
  're2_revision': '0c95bcce2f1f0f071a786ca2c42384b211b8caba',
  'spirv_headers_revision': '9cf7c3a7d2d203b1ee35896547b9644e28d9280e',
  'spirv_tools_revision': '9c0830133b07203a47ddc101fa4b298bab4438d8',
  'spirv_cross_revision': 'fccf1d046204802aa04f05b4af7ca91fab37c50f',
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
