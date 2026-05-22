use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '630e92d5d51d73a1f60ddd7654980ca2eae91582',
  'effcee_revision': '63394054b5afa14aee3e32bd12b227eb7225b871',
  'glslang_revision': '5313f0a5b1ef12450fc4b70435a1d03a4de23ddd',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '927f5d53caf8111721e734cf24724686bb745f55',
  'spirv_headers_revision': '8c5559c134abcf432ec59db842404087b9906c1a',
  'spirv_tools_revision': 'bf3ad6e795df95455c206452ce78a6c0277a5dd3',
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
