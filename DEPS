use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '4bef5dbed590d1edfd3e34bc83d4141f41b998b0',
  'glslang_revision': '302fe97e7b02e37856961ea74f76759098b99188',
  'googletest_revision': 'b77e5c76252bac322bb82c5b444f050bd0d92451',
  're2_revision': 'c58f15086aa70a86946730bdb6ff6a673d636dfa',
  'spirv_headers_revision': 'e4322e3be589e1ddd44afb20ea842a977c1319b8',
  'spirv_tools_revision': 'aea4e6b1b9e3c1f99766b34b2b6a02515b5dd9b1',
  'spirv_cross_revision': '98d8bcd64b58ddc754927985861ef6f53625917c',
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
