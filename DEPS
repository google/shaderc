use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': '9866ad9195cec8f266f16191fb4ec2ce4896e5c0',
  'googletest_revision': 'af4c2cb098a35f4898c5089335c10ba4bd5a4fce',
  're2_revision': '0c95bcce2f1f0f071a786ca2c42384b211b8caba',
  'spirv_headers_revision': 'e74c389f81915d0a48d6df1af83c3862c5ad85ab',
  'spirv_tools_revision': '42830e5a68c38b4bf7a8ad39ee83cc7ab60eb972',
  'spirv_cross_revision': '00a8539d1ddf8d0d934813e409500af6363c96f1',
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
