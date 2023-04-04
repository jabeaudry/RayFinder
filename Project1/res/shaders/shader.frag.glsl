#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

uniform sampler2D texture_diffuse1;
//uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;


uniform vec3 viewPos;
uniform vec3 lightPos;

float shininess = 16.0f;
float gamma = 2.2;

// Add this function to compute the shadow factor
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords *= 0.5;
    projCoords += 0.5;

    // Get the closest depth value from the depth map
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Convert the depth value in the range [0,1]
    float currentDepth = projCoords.z;

    // Bias to avoid self-shadowing and incorrect depth comparisons
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
       shadow /= 9.0;
    shadow = min(shadow, 1.0); // Clamp the shadow value to 1.0
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}



void main(){

    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;

    vec3 color = vec3(texture(texture_diffuse1, TexCoords));
    
    // Normalize the normal vector
    vec3 normal = normalize(Normal);

    //light color
    vec3 lightColor = vec3(0.3);

    // ambient
    vec3 ambient = 0.3 * lightColor;
    // diffuse
    float distanceToLight = length(lightPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    float attenuation = 1.0 / (constant + linear * distanceToLight + quadratic * (distanceToLight * distanceToLight));
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
    

}
