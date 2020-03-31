use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '98980e2b785403b5f43c23ed5a81e1a22e7297e8',
  'glslang_revision': 'e157435c1e777aa1052f446dafed162b4a722e03',
  'googletest_revision': '67cc66080d64e3fa5124fe57ed0cf15e2cecfdeb',
  're2_revision': '209eda1b607909cf3c9ad084264039546155aeaa',
  'spirv_headers_revision': 'f8bf11a0253a32375c32cad92c841237b96696c0',
  'spirv_tools_revision': 'fd773eb50d628c1981338addc093df879757c2cf',
  'spirv_cross_revision': '9b3c5e12be12c55533f3bd3ab9cc617ec0f393d8',
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
