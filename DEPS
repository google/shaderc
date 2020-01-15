use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '4fc7a33910fb8e40b970d160e1b38ab3f67fe0f3',
  'googletest_revision': 'd854bd6acc47f7f6e168007d58b5f509e4981b36',
  're2_revision': '85c014206aee9bc1730dc416b33609974ae3ff5f',
  'spirv_headers_revision': '204cd131c42b90d129073719f2766293ce35c081',
  'spirv_tools_revision': '323a81fc5e30e43a04e5e22af4cba98ca2a161e6',
  'spirv_cross_revision': '172e39f0398b920cfc221b7826c92105d44ad647',
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
