use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'cd25ec17e9382f99a895b9ef53ff3c277464d07d',
  'glslang_revision': 'b131630e7c749a5dc19faa458024260c71fb170f',
  'googletest_revision': 'ba33a8876c3eda4cb8def8e0e90f45930ef8c54f',
  're2_revision': 'eecfdbf1701dd7ebad2f28b9965ca09e0bfb45b0',
  'spirv_headers_revision': 'af64a9e826bf5bb5fcd2434dd71be1e41e922563',
  'spirv_tools_revision': '7e2cba6a52415ee478d0e6e56c8699a6fe4a5b03',
  'spirv_cross_revision': '00189b19a5da553c668290a051604209586b2139',
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
