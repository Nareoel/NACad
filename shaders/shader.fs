#version 330 core

struct MaterialData {
    vec3 color;
    int diffuseLayersIndices[256];
    int specularLayersIndices[256];
    int emissionLayersIndices[256];
    int diffuseLayersCount;
    int specularLayersCount;
    int emissionLayersCount;
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

uniform MaterialData material;
uniform sampler2DArray textureArray;
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
    float alpha = 1.0;
    if(material.diffuseLayersCount > 0){
        for(int i = 0; i < material.diffuseLayersCount; ++i){
            int layerIndex = material.diffuseLayersIndices[i];
            diffuseTextureSum += texture(textureArray, vec3(TexCoord, float(layerIndex))).rgb;
        }
        diffuseTextureSum /= float(material.diffuseLayersCount);
        // ugly -- need to fix. actually I don't know why we need more than one texture layer with the same type -- need to review
        int firstIndex =  material.diffuseLayersIndices[0];
        alpha = texture(textureArray, vec3(TexCoord, float(firstIndex))).a;
    }

    vec3 specularTextureSum = vec3(0.0,0.0,0.0);
    if(material.specularLayersCount > 0){
        for(int i = 0; i < material.specularLayersCount; ++i){
            int layerIndex = material.specularLayersIndices[i];
            specularTextureSum += texture(textureArray, vec3(TexCoord, float(layerIndex))).rgb;
        }
        specularTextureSum /= float(material.specularLayersCount);
    }

    vec3 emissionTextureSum = vec3(0.0,0.0,0.0);
    if(material.emissionLayersCount > 0){
        for(int i = 0; i < material.emissionLayersCount; ++i){
            int layerIndex = material.emissionLayersIndices[i];
            emissionTextureSum += texture(textureArray, vec3(TexCoord + vec2(0.0, time), float(layerIndex))).rgb;
        }
        emissionTextureSum /= float(material.emissionLayersCount);  
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
    vec3 emission = emissionTextureSum * floor(vec3(1.0) - specularTextureSum);
    result += emission;
    
    result += material.color;

    FragColor = vec4(result, alpha);
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