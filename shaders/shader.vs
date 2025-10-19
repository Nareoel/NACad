#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 localTr;
uniform mat4 modelTr;
uniform mat4 viewTr;
uniform mat4 projectionTr;

out vec3 Normal;
out vec2 TexCoord;
out vec3 FragPosition;

void main() {
    gl_Position = projectionTr * viewTr * modelTr * localTr * vec4(aPos, 1.0f);
    FragPosition = vec3(modelTr * localTr * vec4(aPos, 1.0f));
    Normal = mat3(transpose(inverse(modelTr * localTr))) * aNormal;
    TexCoord = aTextureCoords;
}