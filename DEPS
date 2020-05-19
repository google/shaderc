use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '5af957bbfc7da4e9f7aa8cac11379fa36dd79b84',
  'glslang_revision': '0ab78114a9e1b28a48cd37252ec59fdcf85e187d',
  'googletest_revision': '011959aafddcd30611003de96cfd8d7a7685c700',
  're2_revision': 'b9e7f273fd1aae2a83324d10a253165fcdd33ece',
  'spirv_headers_revision': 'c0df742ec0b8178ad58c68cff3437ad4b6a06e26',
  'spirv_tools_revision': '2f69ea849aaa0551af77c599239a68342389f86e',
  'spirv_cross_revision': '3c43f055df0d7b6948af64c825bf93beb8ab6418',
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
