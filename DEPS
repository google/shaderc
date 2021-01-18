use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': '9325cc013e3df4f85a457c2d43e831a9e93713e1',
  'googletest_revision': '389cb68b87193358358ae87cc56d257fd0d80189',
  're2_revision': 'c33d1680c7e9ab7edea02d7465a8db13e80b558d',
  'spirv_headers_revision': 'f027d53ded7e230e008d37c8b47ede7cd308e19d',
  'spirv_tools_revision': 'ee39b5db5f1d51bbdb0422d529d1e99615627bcf',
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
