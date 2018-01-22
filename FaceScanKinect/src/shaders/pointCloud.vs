#version 400
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_colour;
out vec3 colour;
out float should_discard;
uniform bool drawColoredPoints;
uniform mat4 projMatrix;
uniform mat4 mvMatrix;

void main() {
    if (drawColoredPoints) { colour = vertex_colour; } else { colour = vec3(0.6, 0.6, 0.6); }

    if (vertex_colour == vec3(1.0, 1.0, 1.0)) {
        should_discard = 1.0f;
    } else {
        should_discard = 0.0;
    }
    gl_Position = projMatrix * mvMatrix * vec4(vertex_position, 1.0);
}
