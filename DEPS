use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': '4d2298bfd78a82f77f2325c4ade096ccdab1f00d',
  'googletest_revision': 'e3f0319d89f4cbf32993de595d984183b1a9fc57',
  're2_revision': 'ac65d4531798ffc9bf807d1f7c09efb0eec70480',
  'spirv_headers_revision': '2ad0492fb00919d99500f1da74abf5ad3c870e4e',
  'spirv_tools_revision': 'ca5751590ed751fba8abaae9b345663002878865',
  'spirv_cross_revision': '54658d62559a364319cb222afe826d0d68c55ad0',
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
