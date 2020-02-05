use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '5e86b28ffb8124a5ee2a58c640245ee5b285110d',
  'googletest_revision': '10b1902d893ea8cc43c69541d70868f91af3646b',
  're2_revision': '05faa8db3a964918db24003c4614b701b7bbd08a',
  'spirv_headers_revision': 'dc77030acc9c6fe7ca21fff54c5a9d7b532d7da6',
  'spirv_tools_revision': 'ddcc11763a38485bb0fd16e1bdbfe6276e6eac01',
  'spirv_cross_revision': '6b2add8e2cdd2fcc3a94214775c35422881f9d13',
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
