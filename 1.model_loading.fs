#version 330 core
out vec4 FragColor;
 
in vec2 TexCoords;
in vec3 normal_vec;  
in vec3 FragPos; 
in vec3 ka;
in vec3 kd;
in vec3 ks;
in float ns;
  
uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1;  
uniform sampler2D texture_ambient1; 
uniform vec3 camera_position;
uniform vec3 lightPos;

void main()
{
    vec4 colorLight = vec4(1.0);
    vec4 colorAmb = vec4(ka, 1.0);
    vec4 colorDiff = vec4(kd, 1.0);
    vec4 colorSpec = vec4(ks, 1.0);

    if(TexCoords[0] >= 0){
        colorAmb = texture(texture_ambient1, TexCoords);
        colorDiff = texture(texture_diffuse1, TexCoords);
        colorSpec = texture(texture_specular1, TexCoords);
    }

    //ambient
    float ambientStrength = 0.2;
    vec4 ambient = ambientStrength * colorAmb;
  	
    // diffuse
    float diffuseStrength = 0.5;
    vec3 normalDir = normalize(normal_vec);
    vec3 lightDir = normalize(lightPos- FragPos);
    float diffFactor = max(dot(normalDir, lightDir), 0.0);
    vec4 diffuse = diffuseStrength * diffFactor * colorDiff;
    
    // specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(camera_position - FragPos);
    vec3 reflectDir = reflect(-lightDir, normalDir);      
    float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), ns);
    vec4 specular = specularStrength * specFactor * colorSpec;
        
    vec4 result = min((ambient + diffuse + specular)*colorLight, 1.0);
    FragColor = result;
} 
