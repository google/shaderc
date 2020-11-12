use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '142cb87f803d42f5ae3dd2da8dff4f19f6f15e8c',
  'googletest_revision': '2cf1f99b97e5a50b5d9db3d5450a6b474e36b79b',
  're2_revision': '166dbbeb3b0ab7e733b278e8f42a84f6882b8a25',
  'spirv_headers_revision': 'c43a43c7cc3af55910b9bec2a71e3e8a622443cf',
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
