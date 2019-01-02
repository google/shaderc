use_relative_paths = True

vars = {
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'effcee_revision' : '8f0a61dc95e0df18c18e0ac56d83b3fa9d2fe90b',
  'glslang_revision': 'e26630fe2020db48bbd697b0070e61a2bfdea00b',
  'googletest_revision': 'd5932506d6eed73ac80b9bcc47ed723c8c74eb1e',
  're2_revision': 'f620af75bd693f917c684106d26de1b99ffe0e0d',
  'spirv_headers_revision': '4618b86e9e4b027a22040732dfee35e399cd2c47',
  'spirv_tools_revision': '64f2750e5dc553baa2922b780f15049023689ef9',
}

deps = {
  'third_party/effcee': vars['google_git'] + '/effcee.git@' +
      vars['effcee_revision'],

  'third_party/googletest': vars['google_git'] + '/googletest.git@' +
      vars['googletest_revision'],

  'third_party/glslang': vars['khronos_git'] + '/glslang.git@' +
      vars['glslang_revision'],

  'third_party/re2': vars['google_git'] + '/re2.git@' +
      vars['re2_revision'],

  'third_party/spirv-headers': vars['khronos_git'] + '/SPIRV-Headers.git@' +
      vars['spirv_headers_revision'],

  'third_party/spirv-tools': vars['khronos_git'] + '/SPIRV-Tools.git@' +
      vars['spirv_tools_revision'],
}
