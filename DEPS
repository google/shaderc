use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': 'f9d08a25fbe17e0677a89d398f4d7f232339c3f9',
  'googletest_revision': 'ee32b72e12048c14868012a8f339202f82f1ef7d',
  're2_revision': '848dfb7e1d7ba641d598cb66f81590f3999a555a',
  'spirv_headers_revision': 'de99d4d834aeb51dd9f099baa285bd44fd04bb3d',
  'spirv_tools_revision': '001e823b65345145bcaaeb94d39290b10f8661b3',
  'spirv_cross_revision': '05ea055096df8b7b8b3e3f7e121279acf83f74c0',
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
