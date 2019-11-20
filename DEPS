use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': 'f4d4668529f1dad0e475295456b601353fe7cf33',
  'googletest_revision': '50cfbb726b26700d143ce5bb55c0b5e86de7a1e6',
  're2_revision': 'af3455996c0213fa030546eeba0cc44b947b1de8',
  'spirv_headers_revision': 'af64a9e826bf5bb5fcd2434dd71be1e41e922563',
  'spirv_tools_revision': '0391d0823ebfd7c37c07a54b8726cc417183a95f',
  'spirv_cross_revision': 'fd5aa3ad51ece55a1b51fe6bfb271db6844ae291',
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
