#version 330 core
out vec4 FragColor;

in vec3 normal_vec;  
in vec4 FragPos;  
in vec2 TexCoords;
  
uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1; 
uniform vec3 viewPos;
uniform vec3 lightPos;

void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec4 ambient = ambientStrength * texture(texture_diffuse1, TexCoords);
  	
    // diffuse
    vec3 fp = vec3(FragPos.x, FragPos.y, FragPos.z);
    vec3 norm = normalize(normal_vec);
    vec3 lightDir = normalize(lightPos - fp);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * texture(texture_diffuse1, TexCoords);
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fp);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec4 specular = specularStrength * spec * texture(texture_specular1, TexCoords);  
        
    vec4 result = (ambient + diffuse + specular);
    FragColor = result;
} 
