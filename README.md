# Shaderc

[![Build Status](https://travis-ci.org/google/shaderc.svg)](https://travis-ci.org/google/shaderc)

A collection of tools, libraries and tests for shader compilation.
At the moment it includes:

- `glslc`, a command line compiler for GLSL to SPIR-V, and
- `libshaderc` a library API for doing the same.

## Status

Shaderc is experimental, and subject to significant incompatible changes.

For licensing terms, please see the [`LICENSE`](LICENSE) file.  If interested in
contributing to this project, please see [`CONTRIBUTING.md`](CONTRIBUTING.md)

This is not an official Google product (experimental or otherwise), it is just
code that happens to be owned by Google.  That may change if Shaderc gains
contributions from others.  See the [`CONTRIBUTING.md`](CONTRIBUTING.md) file
for more information. See also the [`AUTHORS`](AUTHORS) and
[`CONTRIBUTORS`](CONTRIBUTORS) files.

## File organization

- `android_test/` : a small Android application to verify compilation
- `cmake/`: CMake utility functions and configuration for Shaderc
- `glslc/`: an executable to compile GLSL to SPIR-V
- `libshaderc/`: a library for compiling shader strings into SPIR-V
- `libshaderc_util/`: a utility library used by multiple shaderc components
- `third_party/`: third party open source packages; see below
- `utils/`: utility scripts for Shaderc

Shaderc depends on `glslang`, the Khronos reference compiler for GLSL.
Sometimes a change updates both Shaderc and glslang.  In that case the
glslang change will appear in [google/glslang](https://github.com/google/glslang)
before it appears upstream in
[KhronosGroup/glslang](https://github.com/KhronosGroup/glslang).
We intend to upstream all changes to glslang. We maintain the separate
copy only to stage those changes for review, and to provide something for
Shaderc to build against in the meantime.  Please see
[DEVELOPMENT.howto.md](DEVELOPMENT.howto.md) for more details.

Shaderc also depends on the
[Google Mock](https://code.google.com/p/googlemock/) testing framework.

In the following sections, `$SOURCE_DIR` is the directory you intend to clone
Shaderc into.

## Getting and building Shaderc

1) Check out the source code:

```sh
git clone https://github.com/google/shaderc $SOURCE_DIR
cd $SOURCE_DIR/third_party
svn checkout http://googlemock.googlecode.com/svn/tags/release-1.7.0 \
    gmock-1.7.0
git clone https://github.com/google/glslang glslang
cd $SOURCE_DIR/
```

2) Ensure you have the requisite tools -- see the tools subsection below.

3) Decide where to place the build output. In the following steps, we'll call it
   `$BUILD_DIR`. Any new directory should work. We recommend building outside
   the source tree, but it is also common to build in a (new) subdirectory of
   `$SOURCE_DIR`, such as `$SOURCE_DIR/build`.

4a) Build (and test) with Ninja on Linux or Windows:

```sh
cd $BUILD_DIR
cmake -GNinja -DCMAKE_BUILD_TYPE={Debug|Release|RelWithDebInfo} $SOURCE_DIR
ninja
ctest # optional
```

4b) Or build (and test) with MSVC on Windows:

```sh
cd $BUILD_DIR
cmake $SOURCE_DIR
cmake --build . --config {Release|Debug|MinSizeRel|RelWithDebInfo}
ctest -C {Release|Debug|MinSizeRel|RelWithDebInfo}
```

After a successful build, you should have a `glslc` executable somewhere under
the `$BUILD_DIR/glslc/` directory, as well as a `libshaderc` library somewhere
under the `$BUILD_DIR/libshaderc/` directory.

### Tools you'll need

For building, testing, and profiling Shaderc, the following tools should be
installed regardless of your OS:

- [CMake](http://www.cmake.org/): for generating compilation targets.
- [Python](http://www.python.org/): for running the test suite.

On Linux, the following tools should be installed:

- [`gcov`](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html): for testing code
    coverage, provided by the `gcc` package on Ubuntu.
- [`lcov`](http://ltp.sourceforge.net/coverage/lcov.php): a graphical frontend
    for `gcov`, provided by the `lcov` package on Ubuntu.
- [`genhtml`](http://linux.die.net/man/1/genhtml): for creating reports in html
    format from `lcov` output, provided by the `lcov` package on Ubuntu.

On Windows, the following tools should be installed and available on your path:

- Visual Studio 2013 Update 4 or later. Previous versions of Visual Studio
  will likely work but are untested.
- Git - including the associated tools, Bash, `diff`.

Optionally, the following tools may be installed on any OS:

 - [`asciidoctor`](http://asciidoctor.org/): for generating documenation.
 - [`nosetests`](https://nose.readthedocs.org): for testing the Python code.

## Bug tracking

We track bugs using GitHub -- click on the "Issues" button on
[the project's GitHub page](https://github.com/google/shaderc).

## Test coverage

On Linux, you can obtain test coverage as follows:

```sh
cd $BUILD_DIR
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_CODE_COVERAGE=ON $SOURCE_DIR
ninja
ninja report-coverage
```

Then the coverage report can be found under the `$BUILD_DIR/coverage-report`
directory.
