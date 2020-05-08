#version 410 core
layout (location = 0) in vec3 aPos;

out vec2 UV;

uniform float pos_x;
uniform float pos_y;
uniform float width_inv;
uniform float height_inv;

void main() {
    vec2 pos_sh = vec2((aPos.x + 1.0f) * 0.5f - 0.5f, 1.0f - (aPos.y + 1.0f) * 0.5f - 0.5f);
    
    UV = vec2(0.5f + pos_sh.x * width_inv - pos_x , 0.5f + pos_sh.y * height_inv - pos_y);
    gl_Position = vec4(aPos.x, aPos.y, 0.0f, 1.0f);
}

