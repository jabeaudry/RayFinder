#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 viewPos;

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirectionalLight dirLight;

uniform float shininess; // Add a shininess uniform

void main()
{    
    // Normalize the normal vector
    vec3 norm = normalize(Normal);
    
    // Calculate the view direction vector
    vec3 viewDir = normalize(viewPos - FragPos);

    // Directional light
    vec3 lightDir = normalize(-dirLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = max(dot(norm, halfwayDir), 0.0);

    vec3 ambient = dirLight.ambient * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse = dirLight.diffuse * diff * vec3(texture(texture_specular1, TexCoords));
    vec3 specular = dirLight.specular * spec * vec3(texture(texture_diffuse1, TexCoords));

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
