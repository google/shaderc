use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : '19b4aa87af25cb4ee779a071409732f34bfc305c',
  'glslang_revision': 'd73712b8f6c9047b09e99614e20d456d5ada2390',
  'googletest_revision': 'e47544ad31cb3ceecd04cc13e8fe556f8df9fe0b',
  're2_revision': 'c9cba76063cf4235c1a15dd14a24a4ef8d623761',
  'spirv_headers_revision': '8b246ff75c6615ba4532fe4fde20f1be090c3764',
  'spirv_tools_revision': '04896c462d9f3f504c99a4698605b6524af813c1',
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
