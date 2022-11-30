use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '35912e1b7778ec2ddcff7e7188177761539e59',
  'glslang_revision': '728c689574fba7e53305b475cd57f196c1a21226',
  'googletest_revision': 'd9bb8412d60b993365abb53f00b6dad9b2c01b62',
  're2_revision': 'd2836d1b1c34c4e330a85a1006201db474bf2c8a',
  'spirv_headers_revision': 'c214f6f2d1a7253bb0e9f195c2dc5b0659dc99ef',
  'spirv_tools_revision': 'd9446130d5165f7fafcb3599252a22e264c7d4bd',
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
