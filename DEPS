use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '839704450200e407490c538418f4d1a493b789ab',
  'googletest_revision': 'c6e309b268d4fb9138bed7d0f56b7709c29f102f',
  're2_revision': '14d3193228e3b3cfd21094474d64434a4840bae9',
  'spirv_headers_revision': '11d7637e7a43cd88cfd4e42c99581dcb682936aa',
  'spirv_tools_revision': 'd4b9f576ebb48e716efe14c7ea634a11427fa34d',
  'spirv_cross_revision': 'f9ae06512ef744a1379d564ed0a202b12dc3478b',
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
