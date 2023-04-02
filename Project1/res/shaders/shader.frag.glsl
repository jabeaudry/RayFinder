#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

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
uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;

float shininess = 10.0f;
float gamma = 2.2;

// Add this function to compute the shadow factor
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get the closest depth value from the depth map
    float closestDepth = texture(depthMap, projCoords.xy).r;

    // Convert the depth value in the range [0,1]
    float currentDepth = projCoords.z;

    // Calculate the shadow factor
    float shadow = currentDepth > closestDepth + 0.005 ? 1.0 : 0.0;

    return shadow;
}

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
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);

    // simple attenuation
    float max_distance = 1.5;
    float distance = length(dirLight.direction - FragPos);
    float attenuation = 1.0 /  (distance * distance );

    vec3 ambient = dirLight.ambient* attenuation * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse = dirLight.diffuse* attenuation * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = dirLight.specular * attenuation * spec * vec3(texture(texture_specular1, TexCoords));

    // Calculate the shadow factor
    float shadow = ShadowCalculation(FragPosLightSpace);

    vec3 result = (ambient + shadow * (diffuse + specular));
    FragColor = vec4(pow(result, vec3(1.0 / gamma)), 1.0);
}
