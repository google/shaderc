use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '79ca5d7aad63973c83a4962a66ab07cd623131ea',
  'effcee_revision' : '66edefd2bb641de8a2f46b476de21f227fc03a28',
  'glslang_revision': '9fbc561947f6b5275289a1985676fb7267273e09',
  'googletest_revision': 'd9c309fdab807b716c2cf4d4a42989b8c34f712a',
  're2_revision': 'c9cba76063cf4235c1a15dd14a24a4ef8d623761',
  'spirv_headers_revision': 'bdbfd019be6952fd8fa9bd5606a8798a7530c853',
  'spirv_tools_revision': 'e7c6084fd1d6d6f5ac393e842728d8be309688ca',
}

deps = {
  'third_party/abseil_cpp':
      Var('abseil_git') + '/abseil-cpp.git@' + Var('abseil_revision'),

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
