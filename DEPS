use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '6527fb25482ee16f0ae8c735d1d0c33f7d5f220a',
  'glslang_revision': '56f61ccceffab3225da20c4751cf03a5884431c2',
  'googletest_revision': '565f1b848215b77c3732bca345fe76a0431d8b34',
  're2_revision': '5bd613749fd530b576b890283bfb6bc6ea6246cb',
  'spirv_headers_revision': '38cafab379e5d16137cb97a485b9385191039b92',
  'spirv_tools_revision': 'b54d950298001ad689b404b6871a4136d63b3489',
  'spirv_cross_revision': 'a06997a6a496662c49fefc02bb2421d391b45309',
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
