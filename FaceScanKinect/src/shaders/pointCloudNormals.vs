#version 400
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec3 vertex_normal;

out vec3 pos;
out vec3 normal;
out vec4 color;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;

void main() {
    color  = vec4(vertex_color, 1.0);
    // color  = vec4(1.0, 0.4, 0.4, 1.0);
    normal = vertex_normal;
    pos    = vertex_position;
    gl_Position = projMatrix * mvMatrix * vec4(vertex_position, 1.0);
}
