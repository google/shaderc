use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'cd25ec17e9382f99a895b9ef53ff3c277464d07d',
  'glslang_revision': '7bc047326e06961c59b785f827026947d81c7f02',
  'googletest_revision': 'dc1ca9ae4c206434e450ed4ff535ca7c20c79e3c',
  're2_revision': '5bd613749fd530b576b890283bfb6bc6ea6246cb',
  'spirv_headers_revision': '842ec90674627ed2ffef609e3cd79d1562eded01',
  'spirv_tools_revision': '9eb1c9a4c45099c23098d97c5a6a8b35a45a8f25',
  'spirv_cross_revision': '5431e1da2dc11123750f39a5ba4e5a8c117a0773',
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
