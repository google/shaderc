use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': 'dbf88f932096c7f7714356e919f04749eb87c3e9',
  'effcee_revision': 'f8e8a164822d4f65e757bff66bc00e1567959aa0',
  'glslang_revision': '73b25650df6dc4545966674531e6b7cc8f5d2407',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '2da0056814cf180480a19f5cf811e7e1c054bf6d',
  'spirv_headers_revision': '86f980c731e62ae4eaf383d320449d71687936bf',
  'spirv_tools_revision': '71f287f18ed78d496e724fbfb1c3feae3a2af75a',
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
