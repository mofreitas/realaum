#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 color_ambi;
layout (location = 4) in vec3 color_diff;
layout (location = 5) in vec3 color_spec;
layout (location = 6) in float shininess;

out vec2 TexCoords;
out vec3 normal_vec;
out vec3 FragPos;
out vec3 ka;
out vec3 kd;
out vec3 ks;
out float ns;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;  

    ka = color_ambi;
    kd = color_diff;
    ks = color_spec;
    ns = shininess;

    FragPos =  vec3(model * vec4(aPos, 1.0));
    normal_vec = mat3(model) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
