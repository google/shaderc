use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'cd25ec17e9382f99a895b9ef53ff3c277464d07d',
  'glslang_revision': '834ee546f93d33a80fb2dea6fdef6764f8730b75',
  'googletest_revision': 'f966ed158177f2ed6ff2c402effb16f7f00ca40b',
  're2_revision': 'ab12219ba56a47200385673446b5d371548c25db',
  'spirv_headers_revision': 'af64a9e826bf5bb5fcd2434dd71be1e41e922563',
  'spirv_tools_revision': 'e8c3f9b0b463c24eab839c82332344ccaddfa19d',
  'spirv_cross_revision': 'ff1897ae0e1fc1e37c604933694477f335ca8e44',
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
