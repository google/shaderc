use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '9b20b25138bfe916173c9341075b996be14baa69',
  'googletest_revision': '389cb68b87193358358ae87cc56d257fd0d80189',
  're2_revision': '7107ebc4fbf7205151d8d2a57b2fc6e7853125d4',
  'spirv_headers_revision': 'eddd4dfc930f1374a70797460240a501c7d333f7',
  'spirv_tools_revision': '7d768812e20296c877a44ce0633d71f952fbf83c',
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
