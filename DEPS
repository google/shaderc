use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '7f6559d2802d0653541060f0909e33d137b9c8ba',
  'googletest_revision': '36d8eb532022d3b543bf55aa8ffa01b6e9f03490',
  're2_revision': '9e5430536b59ad8a8aff8616a6e6b0f888594fac',
  'spirv_headers_revision': '5ab5c96198f30804a6a29961b8905f292a8ae600',
  'spirv_tools_revision': '671914c28e8249f0a555726a0f3f38691fe5c1df',
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
