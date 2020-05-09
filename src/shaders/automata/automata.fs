#version 410 core

out vec4 fragColor;
in vec2 UV;

#define COLOR_MAX 0.00392f // 1/255

uniform usampler2D automata;

void main() {
    vec3 col = vec3(texture(automata, UV).rgb) * COLOR_MAX;
    fragColor = vec4(col, 1.0f);
}

