use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'b83b58d177b797edd1f94c5f10837f2cc2863f0a',
  'glslang_revision': '4b4b41a63499d34c527ee4f714dde8072f60c900',
  'googletest_revision': '437e1008c97b6bf595fec85da42c6925babd96b2',
  're2_revision': 'e356bd3f80e0c15c1050323bb5a2d0f8ea4845f4',
  'spirv_headers_revision': '29c11140baaf9f7fdaa39a583672c556bf1795a1',
  'spirv_tools_revision': 'b8ab80843f67479b1b0c096138e62ec78145f05b',
  'spirv_cross_revision': '53ab2144b90abede33be5161aec5dfc94ddc3caf',
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
