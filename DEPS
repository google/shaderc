use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '4bef5dbed590d1edfd3e34bc83d4141f41b998b0',
  'glslang_revision': '7fc86834914354011f9b7756ffd43e58162e7956',
  'googletest_revision': 'bb481d2da65003a6afae672814dd99a9a17bda53',
  're2_revision': 'd5d32aaf5a4c68388d25ce936245a16e0cafb254',
  'spirv_headers_revision': 'e4322e3be589e1ddd44afb20ea842a977c1319b8',
  'spirv_tools_revision': '9559cdbdf011c487f67f89e2d694bd4a18d5c1e0',
  'spirv_cross_revision': 'ffca8735ff42a9e7a3d1dbb984cf3bf2ca724ebc',
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
