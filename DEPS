use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '2e0f0a0517e4251ac8e0d6fd46e92af33fcb800d',
  'googletest_revision': '011959aafddcd30611003de96cfd8d7a7685c700',
  're2_revision': '52b4b94b00f094d4a86c9b6bbb8276b21ec53505',
  'spirv_headers_revision': 'c0df742ec0b8178ad58c68cff3437ad4b6a06e26',
  'spirv_tools_revision': '2b987c49a4e1558117095ad3b77c55fef7aa63ef',
  'spirv_cross_revision': '29ad40e93ed826cfa04da0326a0cfd8717f88583',
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
