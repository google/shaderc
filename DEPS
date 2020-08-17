use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': 'f257e0ea6b9aeab2dc7af3207ac6d29d2bbc01d0',
  'googletest_revision': 'adeef192947fbc0f68fa14a6c494c8df32177508',
  're2_revision': 'ca11026a032ce2a3de4b3c389ee53d2bdc8794d6',
  'spirv_headers_revision': '3fdabd0da2932c276b25b9b4a988ba134eba1aa6',
  'spirv_tools_revision': 'b8de4f57e9838c107bcbf17cf1e02608651c0e1d',
  'spirv_cross_revision': '4c7944bb4260ab0466817c932a9673b6cf59438e',
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
