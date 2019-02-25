## Updating SPIRV-Cross tests

When rolling to a new revision of SPIRV-Cross you need to bring any new SPIRV-Cross tests into the shaderc repo.

1. Go into `third_party/spirv-cross` and run `checkout_glslang_spirv_tools.sh` then `build_glslang_spirv_tools.sh`.
2. Go into `spvc/test` and run `python extract_spirv_cross_tests.py > spirv_cross_test_inputs/test_cases`.  This compiles each test input, places it in the shaderc repo, and writes a new `test_cases` file.
3. Resolve any conflicts in `test_cases`.
