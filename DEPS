use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '517f39eee46f27c83527117d831c4d7e2f7c9fe3',
  'googletest_revision': 'df6b75949b1efab7606ba60c0f0a0125ac95c5af',
  're2_revision': 'ca11026a032ce2a3de4b3c389ee53d2bdc8794d6',
  'spirv_headers_revision': '3fdabd0da2932c276b25b9b4a988ba134eba1aa6',
  'spirv_tools_revision': '8a0ebd40f86d1f18ad42ea96c6ac53915076c3c7',
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
}
