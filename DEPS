use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': 'c594de23cdd790d64ad5f9c8b059baae0ee2941d',
  'googletest_revision': 'e5644f5f12ff3d5b2232dabc1c5ea272a52e8155',
  're2_revision': '91420e899889cffd100b70e8cc50611b3031e959',
  'spirv_headers_revision': 'f027d53ded7e230e008d37c8b47ede7cd308e19d',
  'spirv_tools_revision': '3b85234542cadf0e496788fcc2f20697152e72f8',
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
