use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': 'f5ed7a69d5d64bd3ac802712c24995c6c12d23f8',
  'googletest_revision': '356f2d264a485db2fcc50ec1c672e0d37b6cb39b',
  're2_revision': 'fe8a81adc2ef24b99d44fb87e882d7f2cd504b91',
  'spirv_headers_revision': '308bd07424350a6000f35a77b5f85cd4f3da319e',
  'spirv_tools_revision': 'bd2a9ea85210d3bb474bc5adb9ff4b0bb536b4fc',
  'spirv_cross_revision': '3b366db7f1eed21c31b275f9f7b119d7b00c1b2a',
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
