use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': 'dbf88f932096c7f7714356e919f04749eb87c3e9',
  'effcee_revision': '66c954c82ab04686b3b3583adbdc7c06c891cc29',
  'glslang_revision': '385445a87d21d7a5c7bee2f7bef8a87850f6bf28',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '2da0056814cf180480a19f5cf811e7e1c054bf6d',
  'spirv_headers_revision': '5c6004e758e0c80185ef2cee93ae5c10fa342a5b',
  'spirv_tools_revision': '45d4e0402a9900af83e97fe8ed6786f4c33bf65c',
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
