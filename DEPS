use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': 'dd69df7f3dac26362e10b0f38efb9e47990f7537',
  'googletest_revision': '36d8eb532022d3b543bf55aa8ffa01b6e9f03490',
  're2_revision': '9e5430536b59ad8a8aff8616a6e6b0f888594fac',
  'spirv_headers_revision': 'f027d53ded7e230e008d37c8b47ede7cd308e19d',
  'spirv_tools_revision': 'b27b1afd12d05bf238ac7368bb49de73cd620a8e',
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
