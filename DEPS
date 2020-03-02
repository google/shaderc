use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '56364b6b602696c021349794a8d39744a1052afc',
  'googletest_revision': 'e588eb1ff9ff6598666279b737b27f983156ad85',
  're2_revision': 'f734980e70ee217cc45d3e5535b47cd2422c8f4b',
  'spirv_headers_revision': '0a7fc45259910f07f00c5a3fa10be5678bee1f83',
  'spirv_tools_revision': 'e1688b60caf77e7efd9e440e57cca429ca7c5a1e',
  'spirv_cross_revision': '9deb6ffbba01298fff649d911e45a774903e8802',
}

deps = {
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

  'third_party/spirv-cross': Var('khronos_git') + '/SPIRV-Cross.git@' +
      Var('spirv_cross_revision'),
}
