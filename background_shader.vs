#version 330 core
layout (location = 0) in vec3 vec;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
void main(){
    gl_Position = vec4(vec.x, vec.y, vec.z, 1.0);
    TexCoord = aTexCoord;
}