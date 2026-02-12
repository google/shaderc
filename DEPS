use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '0e5031d3c00d8a482cd5705b5d006518447f39da',
  'effcee_revision': '514b52ec61609744d7e587d93a7ef9b60407ab45',
  'glslang_revision': 'f0bd0257c308b9a26562c1a30c4748a0219cc951',
  'googletest_revision': '56efe3983185e3f37e43415d1afa97e3860f187f',
  're2_revision': '972a15cedd008d846f1a39b2e88ce48d7f166cbd',
  'spirv_headers_revision': '04f10f650d514df88b76d25e83db360142c7b174',
  'spirv_tools_revision': 'fbe4f3ad913c44fe8700545f8ffe35d1382b7093',
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
