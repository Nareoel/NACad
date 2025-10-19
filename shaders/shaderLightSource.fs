#version 330 core

in vec2 TexCoord;

uniform vec3 sourceColor;

out vec4 FragColor;

void main(){  
    FragColor = vec4(sourceColor, 1.0);
}