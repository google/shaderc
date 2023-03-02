use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '66edefd2bb641de8a2f46b476de21f227fc03a28',
  'glslang_revision': '2ca0ee3ba4ed94e95f16c3d5b500a0a76b133ed1',
  'googletest_revision': 'd9c309fdab807b716c2cf4d4a42989b8c34f712a',
  're2_revision': '3a8436ac436124a57a4e22d5c8713a2d42b381d7',
  'spirv_headers_revision': '295cf5fb3bfe2454360e82b26bae7fc0de699abe',
  'spirv_tools_revision': '63de608daeb7e91fbea6d7477a50debe7cac57ce',
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
}
