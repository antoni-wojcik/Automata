#version 410 core

out vec4 fragColor;
in vec2 UV;

uniform isampler2D automata;

void main() {
    fragColor = texture(automata, UV);
}

