use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': 'dbf88f932096c7f7714356e919f04749eb87c3e9',
  'effcee_revision': 'f8e8a164822d4f65e757bff66bc00e1567959aa0',
  'glslang_revision': '83bc342ad741773f1ec12a591d845b7cac6e95ab',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '972a15cedd008d846f1a39b2e88ce48d7f166cbd',
  'spirv_headers_revision': '04fd3caa1e8267e4d95c806cad901181728e1006',
  'spirv_tools_revision': 'ef96ed763b43b59b33b31b362f09a02b729fa1c9',
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
