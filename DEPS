use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '2cf9621d6d6f3b5e38f471709f45d35720a7ffee',
  'googletest_revision': '011959aafddcd30611003de96cfd8d7a7685c700',
  're2_revision': '52b4b94b00f094d4a86c9b6bbb8276b21ec53505',
  'spirv_headers_revision': 'ac638f1815425403e946d0ab78bac71d2bdbf3be',
  'spirv_tools_revision': '55193b06e5ed8e7b41c63a166b465c61753afb9a',
  'spirv_cross_revision': '287e93ff80ee8788f326c9bab46a20645b7595c5',
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
