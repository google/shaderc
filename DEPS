use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': 'def9662348b029a6578f7fa9f46fde0e605b3a6c',
  'googletest_revision': '176eccfb8f42339b8ea2341e926f6835f7932bc4',
  're2_revision': '848dfb7e1d7ba641d598cb66f81590f3999a555a',
  'spirv_headers_revision': 'e74c389f81915d0a48d6df1af83c3862c5ad85ab',
  'spirv_tools_revision': '2090d7a2d26cb9bb0b8738f36a156ed3084a7ab0',
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
