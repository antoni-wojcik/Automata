#version 410 core

out vec4 fragColor;
in vec2 fragPos;

uniform sampler2D screen_texture;

void main() {
    fragColor = texture(screen_texture, fragPos);
}
