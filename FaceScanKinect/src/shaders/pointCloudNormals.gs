#version 400
layout(points) in;
layout(line_strip, max_vertices = 2) out;

in vec3[] normal;
in vec4[] color;
in vec3[] pos;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;

out vec4 fcolor;

void main() {
    fcolor = color[0];
    fcolor = vec4(normalize(normal[0])* 0.5f + 0.5f, 1.0f);


    gl_Position = projMatrix * mvMatrix * vec4(pos[0], 1.0);
    EmitVertex();

    gl_Position = projMatrix * mvMatrix * vec4(pos[0] + 0.015 * normal[0], 1.0);
  //  gl_Position = projMatrix * mvMatrix * vec4(pos[0], 1.0);
    EmitVertex();

    EndPrimitive();
}
