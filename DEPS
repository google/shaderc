use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '66edefd2bb641de8a2f46b476de21f227fc03a28',
  'glslang_revision': 'ef77cf3a92490f7c37f36f20263cd3cd8c94f009',
  'googletest_revision': 'd9c309fdab807b716c2cf4d4a42989b8c34f712a',
  're2_revision': '3a8436ac436124a57a4e22d5c8713a2d42b381d7',
  'spirv_headers_revision': '1feaf4414eb2b353764d01d88f8aa4bcc67b60db',
  'spirv_tools_revision': '44d72a9b36702f093dd20815561a56778b2d181e',
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
