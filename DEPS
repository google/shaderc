use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : 'cd25ec17e9382f99a895b9ef53ff3c277464d07d',
  'glslang_revision': '4b97a1108114107a8082a55e9e0721a40f9536d3',
  'googletest_revision': 'cd17fa2abda2a2e4111cdabd62a87aea16835014',
  're2_revision': '266119da0a8328ecb18fbd31398100d2ec230619',
  'spirv_headers_revision': 'b252a50953ac4375cb1864e94f4b0234db9d215d',
  'spirv_tools_revision': 'df15a4a3cbda6bde5e4bbd43492e17ac7752cd68',
  'spirv_cross_revision': 'e5d3a6655e13870a6f38f40bd88cc662b14e3cdf',
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
