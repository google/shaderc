use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': 'dbf88f932096c7f7714356e919f04749eb87c3e9',
  'effcee_revision': '910ed15722d5d05c9d71ecf36c1a22243cb79b02',
  'glslang_revision': '168d452a4f460d24b588fed08477a81c44ee27a1',
  'googletest_revision': '52eb8108c5bdec04579160ae17225d66034bd723',
  're2_revision': '927f5d53caf8111721e734cf24724686bb745f55',
  'spirv_headers_revision': '29981f65241605e08b0ede4cfeb999fe3b723c6a',
  'spirv_tools_revision': 'b707790a898e44038547df54580022fc1cf89c3d',
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
