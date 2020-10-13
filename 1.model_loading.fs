#version 330 core
out vec4 FragColor;
 
in vec2 TexCoords;
in vec3 normal_vec;  
in vec3 FragPos; 
  
uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1; 
uniform vec3 camera_position;
uniform vec3 lightPos;

void main()
{
    vec4 colorDiff = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 colorSpec = vec4(1.0, 1.0, 1.0, 1.0);

    if(TexCoords[0] >= 0){
        colorDiff = texture(texture_diffuse1, TexCoords);
        colorSpec = texture(texture_specular1, TexCoords);
    }

    //ambient
    float ambientStrength = 0.4;
    vec4 ambient = ambientStrength * colorDiff;
  	
    // diffuse
    float diffuseStrength = 0.4;
    vec3 normalDir = normalize(normal_vec);
    vec3 lightDir = normalize(lightPos- FragPos);
    float diff = max(dot(normalDir, lightDir), 0.0);
    vec4 diffuse = diffuseStrength * diff * colorDiff;
    
    // specular
    float specularStrength = 0.2;
    vec3 viewDir = normalize(camera_position - FragPos);
    vec3 reflectDir = reflect(-lightDir, normalDir);      
    float spec = pow(max(dot(camera_position, reflectDir), 0.0), 16);
    vec4 specular = specularStrength * spec * colorSpec;
        
    vec4 result = min(ambient + diffuse + specular, 1.0);
    FragColor = result;
} 
