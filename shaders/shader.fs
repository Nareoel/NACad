#version 330 core

struct Material{
    vec3 color;

    sampler2D diffuse[16];
    int diffuseTexturesNumber;

    sampler2D specular[16];
    int specularTexturesNumber;

    sampler2D emission[16];
    int emissionTexturesNumber;

    float shininess;
};

struct GlobalLight{
    vec3 color;
    // position is a synonim to -direction
    vec3 position;

    float ambientIntence;
    float diffuseIntence;
    float specularIntence;
};

struct PointLight{
    vec3 color;
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    float ambientIntence;
    float diffuseIntence;
    float specularIntence;
};

struct SpotLight{
    vec3 color;
    vec3 position;
    vec3 direction;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    float ambientIntence;
    float diffuseIntence;
    float specularIntence;
};

#define NR_POINT_LIGHTS 4

in vec2 TexCoord;
in vec3 FragPosition;
in vec3 Normal;

uniform Material material;
uniform GlobalLight globalLight;
uniform PointLight pointlights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform vec3 viewPosition;
uniform float time;

out vec4 FragColor;

vec3 calcGlobalLight(GlobalLight light, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum);
vec3 calcPointLight(PointLight light, vec3 fragPosition, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum);
vec3 calcSpotLight(SpotLight light, vec3 fragPosition, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum);

void main(){  

    // textures blending. Not sure if we realy need to blend them or sum is enough   
    vec3 diffuseTextureSum = vec3(0.0,0.0,0.0);
    for(int i = 0; i < material.diffuseTexturesNumber; ++i){
        diffuseTextureSum += texture(material.diffuse[i], TexCoord).rgb  * (1/float(material.diffuseTexturesNumber));
    }
    vec3 specularTextureSum = vec3(0.0,0.0,0.0);;
    for(int i = 0; i < material.specularTexturesNumber; ++i){
        specularTextureSum += texture(material.specular[i], TexCoord).rgb  * (1/float(material.specularTexturesNumber));
    }
    vec3 eimssionTextureSum = vec3(0.0,0.0,0.0);;
    for(int i = 0; i < material.emissionTexturesNumber; ++i){
        eimssionTextureSum += texture(material.emission[i], TexCoord + vec2(0.0, time)).rgb  * (1/float(material.emissionTexturesNumber));
    }

    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragPosition);
    
    vec3 result = vec3(0.0,0.0,0.0);
    result += calcGlobalLight(globalLight, normal, viewDirection, diffuseTextureSum, specularTextureSum);
    for(int i = 0; i < NR_POINT_LIGHTS; ++i){
       result += calcPointLight(pointlights[i], FragPosition, normal, viewDirection, diffuseTextureSum, specularTextureSum);
    }
    result += calcSpotLight(spotLight, FragPosition, normal, viewDirection, diffuseTextureSum, specularTextureSum);

    // emission
    vec3 emission = eimssionTextureSum * floor(vec3(1.0) - specularTextureSum);
    result += emission;
    
    result += material.color;

    FragColor = vec4(result, 1.0);
}

vec3 calcGlobalLight(GlobalLight light, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum){
    vec3 position = normalize(light.position);
    
    // ambient
    vec3 ambient = light.ambientIntence * light.color * diffuseTextureSum;
    // diffuse
    vec3 diffuse = max(dot(normal,position ), 0.0) * light.diffuseIntence * light.color * diffuseTextureSum;
    // specular
    vec3 reflectDirection = reflect(-position, normal);
    vec3 lightSrcSpecular = light.specularIntence*light.color;
    float specularIntence = pow(max(dot(viewDir, reflectDirection), 0.0), material.shininess);
    vec3 specular = specularIntence*lightSrcSpecular*specularTextureSum;

    return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 fragPosition, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum){
    vec3 lightDirection = normalize(light.position - fragPosition);

    // ambient
    vec3 ambient = light.ambientIntence * light.color * diffuseTextureSum;
    // diffuse
    vec3 diffuse = max(dot(normal,lightDirection), 0.0) * (light.diffuseIntence * light.color) * diffuseTextureSum;
    // specular
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float specularIntence = pow(max(dot(viewDir, reflectDirection), 0.0), material.shininess);
    vec3 specular = specularIntence * light.specularIntence * light.color * specularTextureSum;
    // attenuation 
    float distance = length(light.position - fragPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));   

    return (ambient + diffuse + specular)*attenuation;
}

vec3 calcSpotLight(SpotLight light, vec3 fragPosition, vec3 normal, vec3 viewDir, vec3 diffuseTextureSum, vec3 specularTextureSum){
    vec3 toFragDir = normalize(light.position - fragPosition);
    
    // ambient
    vec3 ambient = light.ambientIntence * light.color * diffuseTextureSum;
    // diffuse
    vec3 diffuse = max(dot(normal,toFragDir), 0.0) * (light.diffuseIntence * light.color) * diffuseTextureSum;
    // specular
    vec3 reflectDirection = reflect(-toFragDir, normal);
    float specularIntence = pow(max(dot(viewDir, reflectDirection), 0.0), material.shininess);
    vec3 specular = specularIntence * light.specularIntence * light.color * specularTextureSum;
    // attenuation
    float distance = length(light.position - fragPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));   
    // intensity
    float theta = dot(toFragDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    return (ambient+diffuse+specular)*attenuation*intensity;
}