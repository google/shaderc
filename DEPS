use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': 'ffccefddfd9a02ec0c0b6dd04ef5e1042279c97f',
  'googletest_revision': '36d8eb532022d3b543bf55aa8ffa01b6e9f03490',
  're2_revision': '9e5430536b59ad8a8aff8616a6e6b0f888594fac',
  'spirv_headers_revision': '104ecc356c1bea4476320faca64440cd1df655a3',
  'spirv_tools_revision': 'cd590fa3341284cd6d1ee82366155786cfd44c96',
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
