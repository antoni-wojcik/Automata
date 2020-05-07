#version 410 core

out vec4 fragColor;
in vec2 fragPos;

uniform isampler2D automata;

void main() {
    //float color = texture(automata, fragPos).r;
    //fragColor = vec4(vec3(color), 1.0f); //= vec4(fragPos, 1.0f, 1.0f);//
    fragColor = texture(automata, fragPos);
}

