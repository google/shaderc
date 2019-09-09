use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '6527fb25482ee16f0ae8c735d1d0c33f7d5f220a',
  'glslang_revision': '664ad418f8455159fb066e9e27d159f191f976a9',
  'googletest_revision': '3f05f651ae3621db58468153e32016bc1397800b',
  're2_revision': '5bd613749fd530b576b890283bfb6bc6ea6246cb',
  'spirv_headers_revision': '38cafab379e5d16137cb97a485b9385191039b92',
  'spirv_tools_revision': '76261e2a7df1fda011a5edd7a894c42b4fa6af95',
  'spirv_cross_revision': 'b32a1b415043f5ae368e90af7a585770ecd5546e',
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
