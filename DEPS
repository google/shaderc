use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'cd25ec17e9382f99a895b9ef53ff3c277464d07d',
  'glslang_revision': '973d0e538292c85b7baf9bb5aaf755894429f76a',
  'googletest_revision': 'f2fb48c3b3d79a75a88a99fba6576b25d42ec528',
  're2_revision': '5bd613749fd530b576b890283bfb6bc6ea6246cb',
  'spirv_headers_revision': '601d738723ac381741311c6c98c36d6170be14a2',
  'spirv_tools_revision': 'f62ee4a4a1a1e38c91e29d4cf1e2195b51551b80',
  'spirv_cross_revision': '5431e1da2dc11123750f39a5ba4e5a8c117a0773',
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
