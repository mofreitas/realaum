#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 normal_vec;
out vec3 FragPos;

uniform float scale_factor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat3 model3 = mat3(model);
    TexCoords = aTexCoords;  
    mat3 scale = mat3(scale_factor);

    FragPos =  model3 * scale * aPos;
    normal_vec = model3 * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
