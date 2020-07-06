use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': 'f5ed7a69d5d64bd3ac802712c24995c6c12d23f8',
  'googletest_revision': '356f2d264a485db2fcc50ec1c672e0d37b6cb39b',
  're2_revision': 'fe8a81adc2ef24b99d44fb87e882d7f2cd504b91',
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
