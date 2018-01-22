#version 400
in vec3 colour;
in float should_discard;
out vec4 frag_colour;

void main() {
    if (should_discard > 0.5) {
        discard;
    }
    frag_colour = vec4(colour, 1.0);
}
