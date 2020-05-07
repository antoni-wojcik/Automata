#version 410 core
layout (location = 0) in vec3 aPos;

out vec2 UV;

uniform float pos_x;
uniform float pos_y;
uniform float width;
uniform float height;

void main() {
    UV = vec2(pos_x + ((aPos.x + 1.0f) * 0.5f) * width, pos_y + (1.0f - (aPos.y + 1.0f) * 0.5f) * height);
    gl_Position = vec4(aPos.x, aPos.y, 0.0f, 1.0f);
}

