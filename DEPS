use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '35912e1b7778ec2ddcff7e7188177761539e59',
  'glslang_revision': '1fb2f1d7896627d62a289439a2c3e750e551a7ab',
  'googletest_revision': 'd9bb8412d60b993365abb53f00b6dad9b2c01b62',
  're2_revision': '954656f47fe8fb505d4818da1e128417a79ea500',
  'spirv_headers_revision': 'd13b52222c39a7e9a401b44646f0ca3a640fbd47',
  'spirv_tools_revision': '63de608daeb7e91fbea6d7477a50debe7cac57ce',
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
