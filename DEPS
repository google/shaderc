use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '2ec8f8738118cc483b67c04a759fee53496c5659',
  'glslang_revision': 'b481744aea1ecf52ee4591afaa0f5e270b9d1636',
  'googletest_revision': '70b90929b1da20580cad9ed996397cf04ef8f16d',
  're2_revision': 'fe8a81adc2ef24b99d44fb87e882d7f2cd504b91',
  'spirv_headers_revision': '308bd07424350a6000f35a77b5f85cd4f3da319e',
  'spirv_tools_revision': 'c9b254d045ebcff627163445fddb1cb9ec7a14e6',
  'spirv_cross_revision': '6575e451f5bffded6e308988362224dd076b0f2b',
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
