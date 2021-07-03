in vec4 vertex;
uniform mat4 projection;
out vec2 pos;

void main(void) {
   // The coordinates are defined such that 0 is the center, -1 is left/bottom, and +1 is top/right
   pos = vertex.xy;
   gl_Position = projection * vertex;
}
