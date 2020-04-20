use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '3f4e5c4563068f277141f5fb3d96ec02afc7ac95',
  'googletest_revision': 'dcc92d0ab6c4ce022162a23566d44f673251eee4',
  're2_revision': 'a5c78ae3ea906cdbd759d83511509a985b4b3c8f',
  'spirv_headers_revision': '2ad0492fb00919d99500f1da74abf5ad3c870e4e',
  'spirv_tools_revision': '61b7de3c39f01a0eeb717f444c86990547752e26',
  'spirv_cross_revision': 'f38cbeb814c73510b85697adbe5e894f9eac978f',
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
