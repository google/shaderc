use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '983698bb34ecfbf8a172a59ee4edc2ab7bdfa3b8',
  'googletest_revision': '1e315c5b1a62707fac9b8f1d4e03180ee7507f98',
  're2_revision': 'ca11026a032ce2a3de4b3c389ee53d2bdc8794d6',
  'spirv_headers_revision': '3fdabd0da2932c276b25b9b4a988ba134eba1aa6',
  'spirv_tools_revision': '4dd122392f3ad757e70951a1198479bf233d4cd8',
  'spirv_cross_revision': '685f86471e9d26b3eb7676695a2e2cefb4551ae9',
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
