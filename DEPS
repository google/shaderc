use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '9b620aa0c12d12dd7ec7ced43ce9e58f275d47c1',
  'googletest_revision': 'e588eb1ff9ff6598666279b737b27f983156ad85',
  're2_revision': 'bc40cdeb40ad56327da31ae3d8f3983994d9c616',
  'spirv_headers_revision': '30ef660ce2e666f7ae925598b8a267f4da6d33aa',
  'spirv_tools_revision': 'dd3d91691f1e1dc4c0f42818756cf5e165c8918c',
  'spirv_cross_revision': '65aa0c35d6c2044e4be25e13e6f070caf9b75f31',
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
