use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '34eb767645347f100bdd66fc1e35eee96e357961',
  'effcee_revision' : '19b4aa87af25cb4ee779a071409732f34bfc305c',
  'glslang_revision': '9575e33186c74a68831c469f7271edf386ea43a5',
  'googletest_revision': 'ec4fed93217bc2830959bb8e86798c1d86956949',
  're2_revision': 'c9cba76063cf4235c1a15dd14a24a4ef8d623761',
  'spirv_headers_revision': '3469b164e25cee24435029a569933cb42578db5d',
  'spirv_tools_revision': '04cdb2d344706052c7a2d359294e830ebac63e74',
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
