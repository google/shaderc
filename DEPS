use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '6527fb25482ee16f0ae8c735d1d0c33f7d5f220a',
  'glslang_revision': 'f27bd2aa2e3ff547a02fc2346dc0b155c5278d8d',
  'googletest_revision': '6a3d632f40a1882cb09aeefa767f0fdf1f61c80e',
  're2_revision': '5bd613749fd530b576b890283bfb6bc6ea6246cb',
  'spirv_headers_revision': '059a49598c3c8e4113a67aebd93a0c2b973754de',
  'spirv_tools_revision': '15fc19d0912dfc7c58dba224d321d371952ee565',
  'spirv_cross_revision': '563e994486923817d6fa0637990bf689424e7bf2',
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
