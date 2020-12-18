#!/usr/bin/env python3

import collections

Shader = collections.namedtuple("Shader", "name vs_path fs_path attributes uniforms")

SHADERS = [
   Shader(
      name = "circle",
      vs_path = "circle.vert",
      fs_path = "circle.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "radius"]
   ),
   Shader(
      name = "circle_filled",
      vs_path = "circle.vert",
      fs_path = "circle_filled.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "radius"]
   ),
   Shader(
      name = "solid",
      vs_path = "solid.vert",
      fs_path = "solid.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color"]
   ),
   Shader(
      name = "smooth",
      vs_path = "smooth.vert",
      fs_path = "smooth.frag",
      attributes = ["vertex", "vertex_color"],
      uniforms = ["projection"]
   ),
   Shader(
      name = "texture",
      vs_path = "texture.vert",
      fs_path = "texture.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "tex_mat"]
   ),
   Shader(
      name = "texture_interpolate",
      vs_path = "texture.vert",
      fs_path = "texture_interpolate.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "tex_mat", "sampler1", "sampler2", "inter"]
   ),
   Shader(
      name = "nebula",
      vs_path = "nebula.vert",
      fs_path = "nebula.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "center", "radius"]
   ),
   Shader(
      name = "stars",
      vs_path = "stars.vert",
      fs_path = "stars.frag",
      attributes = ["vertex", "brightness"],
      uniforms = ["projection", "star_xy", "wh", "xy"]
   ),
   Shader(
      name = "font",
      vs_path = "font.vert",
      fs_path = "font.frag",
      attributes = ["vertex", "tex_coord"],
      uniforms = ["projection", "color"]
   ),
   Shader(
      name = "beam",
      vs_path = "beam.vert",
      fs_path = "beam.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "color", "tex_mat"]
   ),
   Shader(
      name = "tk",
      vs_path = "tk.vert",
      fs_path = "tk.frag",
      attributes = ["vertex"],
      uniforms = ["projection", "c", "dc", "lc", "oc", "wh", "corner_radius"]
   ),
]

def write_header(f):
    f.write("/* FILE GENERATED BY %s */\n\n" % __file__)

def generate_h_file(f):
    write_header(f)

    f.write("#ifndef SHADER_GEN_C_H\n")
    f.write("#define SHADER_GEN_C_H\n")

    f.write("#include \"opengl.h\"\n\n")

    f.write("typedef struct Shaders_ {")

    for shader in SHADERS:
        f.write("   struct {\n")

        f.write("      GLuint program;\n")

        for attribute in shader.attributes:
            f.write("      GLuint {};\n".format(attribute))

        for uniform in shader.uniforms:
            f.write("      GLuint {};\n".format(uniform))

        f.write("   }} {};\n".format(shader.name))
    f.write("} Shaders;\n\n")

    f.write("extern Shaders shaders;\n\n")

    f.write("void shaders_load (void);\n")
    f.write("void shaders_unload (void);\n")

    f.write("#endif\n")

def generate_c_file(f):
    write_header(f)

    f.write("#include <string.h>\n")
    f.write("#include \"shaders.gen.h\"\n")
    f.write("#include \"opengl_shader.h\"\n\n")

    f.write("Shaders shaders;\n\n")

    f.write("void shaders_load (void) {\n")
    for i, shader in enumerate(SHADERS):
        f.write("   shaders.{}.program = gl_program_vert_frag(\"{}\", \"{}\");\n".format(
                 shader.name,
                 shader.vs_path,
                 shader.fs_path))
        for attribute in shader.attributes:
            f.write("   shaders.{}.{} = glGetAttribLocation(shaders.{}.program, \"{}\");\n".format(
                    shader.name,
                    attribute,
                    shader.name,
                    attribute))
       
        for uniform in shader.uniforms:
            f.write("   shaders.{}.{} = glGetUniformLocation(shaders.{}.program, \"{}\");\n".format(
                    shader.name,
                    uniform,
                    shader.name,
                    uniform))

        if i != len(SHADERS) - 1:
            f.write("\n")
    f.write("}\n\n")

    f.write("void shaders_unload (void) {\n")
    for shader in SHADERS:
        f.write("   glDeleteProgram(shaders.{}.program);\n".format(shader.name))

    f.write("   memset(&shaders, 0, sizeof(shaders));\n")
    f.write("}\n")

with open("shaders.gen.h", "w") as shaders_gen_h:
    generate_h_file(shaders_gen_h)

with open("shaders.gen.c", "w") as shaders_gen_c:
    generate_c_file(shaders_gen_c)
