use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '90cecb52067a665a444914f8696d6a42fcbd4ebf',
  'googletest_revision': 'e8a82dc7ede61c4af3b9d75aa0e953b8cecfc8bb',
  're2_revision': 'eecfdbf1701dd7ebad2f28b9965ca09e0bfb45b0',
  'spirv_headers_revision': 'af64a9e826bf5bb5fcd2434dd71be1e41e922563',
  'spirv_tools_revision': 'f1e5cd73f658abcc23ee96d78f2dc27c4b7028c1',
  'spirv_cross_revision': '24f209eb9306eb1ae184c0cd95afce279d696045',
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
