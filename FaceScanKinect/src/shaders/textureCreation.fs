#version 400

in vec2 tex;
in float faceToCameraAngle;

out vec4 color;

uniform sampler2D colorImage;

void main() {
    vec3 tex_color = texture(colorImage, tex).rgb;

    if (faceToCameraAngle > 0.0f) {
        color = vec4(tex_color, faceToCameraAngle);
    } else {
        color = vec4(tex_color, 0.0f);
    }

    //color = vec4(0.0f, 0.3f, 1.0f, 1.0f);
}
