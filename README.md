# shaderc

A collection of tools, libraries and tests for shader compilation.

## Status

Shaderc is experimental, and subject to significant incompatible changes.

For licensing terms, please see the `LICENSE` file.  If interested in
contributing to this project, please see `CONTRIBUTING.md`.

This is not an official Google product (experimental or otherwise), it is just
code that happens to be owned by Google.  That may change if Shaderc gains
contributions from others.  See the `CONTRIBUTING.md` file for more information.
See also the `AUTHORS` and `CONTRIBUTORS` files.

## File organization

- `glslc/`: an executable to compile GLSL to SPIR-V
- `libshaderc/`: a library for compiling shader strings into SPIR-V
- `libshaderc_util/`: a utility library used by multiple shaderc components
- `third_party/`: third party open source packages; see below

shaderc depends on a fork of the Khronos reference GLSL compiler glslang.
shaderc also depends on the testing framework googlemock.

In the following example, $SOURCE_DIR is the directory you intend to
clone shaderc into.

## Get started

1) Check out the source code:

    git clone https://github.com/google/shaderc $SOURCE_DIR
    cd $SOURCE_DIR/third_party
    svn checkout http://googlemock.googlecode.com/svn/tags/release-1.7.0 \
        gmock-1.7.0
    git clone https://github.com/google/glslang.git glslang
    cd $SOURCE_DIR/

2) Decide where to place the build outputs. In the following steps, we'll
   call it $BUILD_DIR. Any new directory should work. We recommend building
   outside the source tree, but it is common to build in a subdirectory of
   $SOURCE_DIR, such as $SOURCE_DIR/build.

3a) Build without code coverage (Linux, Windows with Ninja):

    cd $BUILD_DIR
    cmake -GNinja -DCMAKE_BUILD_TYPE={Debug|Release|RelWithDebInfo} $SOURCE_DIR
    ninja
    ctest

3b) Build without coverage (Windows with MSVC):

    cd $BUILD_DIR
    cmake $SOURCE_DIR
    cmake --build . --config {Release|Debug|MinSizeRel|RelWithDebInfo}
    ctest -C {Release|Debug|MinSizeRel|RelWithDebInfo}

3c) Build with code coverage (Linux):

    cd $BUILD_DIR
    cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_CODE_COVERAGE=ON
        $SOURCE_DIR
    ninja
    ninja report-coverage

Then the coverage report can be found under the `$BUILD_DIR/coverage-report
directory.

### Tool dependencies

For building, testing, and profiling shaderc, the following common tools
should be installed:

- [cmake](http://www.cmake.org/): For generating compilation targets.
- [python](http://www.python.org/): For running the test suite.

On Linux, the following tools should be installed:
- [gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html): for testing code
    coverage, provided by the `gcc` package on Ubuntu.
- [lcov](http://ltp.sourceforge.net/coverage/lcov.php): a graphical frontend for
    gcov, provided by the `lcov` package on Ubuntu.
- [genhtml](http://linux.die.net/man/1/genhtml): for creating reports in html
    format from lcov output, provided by the `lcov` package on Ubuntu.

On Windows, the following tools should be installed and available on your path.
   - Visual Studio 2013 Update 4 or later. Previous versions of Visual Studio
     will likely work but are untested.
   - git - including the associated tools, bash, diff.

Optional: for all platforms
   - [asciidoctor](http://asciidoctor.org/): for generating documenation.
   - [nosetests](https://nose.readthedocs.org): for testing the Python code.
