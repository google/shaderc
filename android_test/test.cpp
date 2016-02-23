#include "shaderc/shaderc.hpp"
#include <android_native_app_glue.h>

void android_main(struct android_app* state) {
  app_dummy();
  shaderc::Compiler compiler;
  const char* test_program = "void main() {}";
  compiler.CompileGlslToSpv(test_program, strlen(test_program),
                            shaderc_glsl_vertex_shader, "shader");
}
