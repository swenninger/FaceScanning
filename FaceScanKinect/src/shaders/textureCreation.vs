#version 400
layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in vec3 mesh_vertex_pos;
layout (location = 3) in vec3 normal;

uniform float viewport_size;

out vec2 tex;
out float faceToCameraAngle;

void main() {
    tex = tex_coord;
    faceToCameraAngle = 1.f - dot(normal, normalize(-mesh_vertex_pos));
    gl_Position = vec4(vert_pos, 1.f);
}
