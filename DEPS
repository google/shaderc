use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '2b0eafb1de5b4a1b77cf123545ea269d44248885',
  'googletest_revision': '011959aafddcd30611003de96cfd8d7a7685c700',
  're2_revision': '787495f0ba2e76dcadb21db84455ea0e2ce15beb',
  'spirv_headers_revision': 'ac638f1815425403e946d0ab78bac71d2bdbf3be',
  'spirv_tools_revision': '9cb2571a184c0fe571100c799301426a492f7407',
  'spirv_cross_revision': 'd385bf096f5dabbc4cdaeb6872b0f64be1a63ad0',
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
