use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '1315c900e1ddbb08a23e06eeb9a06450052ccb5e',
  'effcee_revision': '08da24ec245a274fea3a128ba50068f163390565',
  'glslang_revision': 'e966816ab28ab7cb448d5b33270b43c941b343d4',
  'googletest_revision': '1d17ea141d2c11b8917d2c7d029f1c4e2b9769b2',
  're2_revision': '4a8cee3dd3c3d81b6fe8b867811e193d5819df07',
  'spirv_headers_revision': 'f88a2d766840fc825af1fc065977953ba1fa4a91',
  'spirv_tools_revision': '4972c69eb50255b314fc0925ca757c4417e6b6c0',
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
