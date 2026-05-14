use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '630e92d5d51d73a1f60ddd7654980ca2eae91582',
  'effcee_revision': '63394054b5afa14aee3e32bd12b227eb7225b871',
  'glslang_revision': 'd561f02e83fcbd13331dbd2750666c31bc7fc036',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '927f5d53caf8111721e734cf24724686bb745f55',
  'spirv_headers_revision': 'fe44b2002bf7871e2e92fc001bc9f6e09f92194f',
  'spirv_tools_revision': '83876d92207829cc16f1c93d7354ee376d9c8241',
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
