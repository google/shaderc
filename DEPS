use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': 'dbf88f932096c7f7714356e919f04749eb87c3e9',
  'effcee_revision': '66c954c82ab04686b3b3583adbdc7c06c891cc29',
  'glslang_revision': '291d83913ac47c8103daf7138d16bc8072efdce3',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '2da0056814cf180480a19f5cf811e7e1c054bf6d',
  'spirv_headers_revision': 'f6f78c717057ac7f5f3ea021f3f962aaf9d4da7e',
  'spirv_tools_revision': 'b1d2dc272aee6e6c9454d202fe88cbcbbeaab72c',
}
deps = {
  'third_party/abseil_cpp':
      Var('abseil_git') + '/abseil-cpp.git@' + Var('abseil_revision'),

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
