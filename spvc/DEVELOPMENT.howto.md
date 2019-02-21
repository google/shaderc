## Updating SPIRV-Cross tests

When rolling to a new revision of SPIRV-Cross it is necessary to run the script
`extract_spirv_cross_tests.py` in the `test` directory to bring in any new
SPIRV-Cross tests.
The script creates a binary version of each test input and places it in the
`spirv_cross_test_inputs` directory and also outputs a new `test_cases` file.
At present the changes in `test_cases` file need to be manually merged but this
could be automated.
