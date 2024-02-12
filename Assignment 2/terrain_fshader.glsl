R"(
#version 330 core

uniform sampler2D grass;
uniform sampler2D rock;
uniform sampler2D sand;
uniform sampler2D snow;

uniform vec3 viewPos;
uniform float waterHeight;
uniform vec3 skyColor;
uniform vec3 lightPos;

in vec2 uv;
in vec3 fragPos;
in float height;
in vec3 normal;
in float slope; 
in vec3 distanceFromCamera;

out vec4 color;


void main() {
    float snow_level = waterHeight+1.2;
    float shore_level =  waterHeight;
    vec4 col;
    
    // Texturing
    // Sand -> blend -> Grass -> blend -> Snow/Rock(depends on slope)
    if (height > snow_level && slope < 0.4){   
        col = texture(snow, uv);
    } else if((height > shore_level) && (height < snow_level)){
        float halfDistance = (snow_level-shore_level)/2.0f;
        float quartDistance = halfDistance/2.0f;
            if (height < (shore_level + quartDistance)) {
                float pos = height - shore_level;
                float posScaled = float(pos / quartDistance);
                col = (texture(grass, uv) * (posScaled))+(texture(sand, uv) * (1 - posScaled));
            } else if (height > (snow_level - quartDistance) && slope < 0.4) {
                float pos = snow_level - height;
                float posScaled = float(pos / quartDistance);
                col = (texture(grass, uv) * (posScaled))+(texture(snow, uv) * (1 - posScaled));
            } else if (height > (snow_level - quartDistance) && slope >= 0.4) {
                float pos = snow_level - height;
                float posScaled = float(pos / quartDistance);
                col = (texture(grass, uv) * (posScaled))+(texture(rock, uv) * (1 - posScaled));
            } else {
                col = texture(grass, uv);  
            }
    } else if(height < shore_level){
        col = texture(sand, uv);  
    } else {
        col = texture(rock, uv);
    }

    // Blinn-Phong lighting
    float ka = 0.2f, kd = 0.9f, ks = 1.0f, p = 0.8f;    

    vec3 lightDir = normalize(lightPos - fragPos);
    float diffuse = kd * max(0.0f, dot(normal,lightDir));

    vec3 viewDirection = viewPos - fragPos;
    vec3 halfVector = normalize(lightDir + viewDirection);
    float specular = ks * max(0.0f, pow(dot(normal, halfVector), p));

    col = ka*vec4(1.0, 1.0, 1.0, 1.0f) + diffuse*col + specular*col;

    // Fog calculation
    float density = 0.07;
    float gradient = 1.5;
    float distance = length(distanceFromCamera.xyz);
    float visibility = exp(-pow(distance * density, gradient));
    visibility = clamp(visibility, 0.0, 1.0);

    color = mix(vec4(0.6, 0.7, 0.8, 1.0f),  col, visibility);
    
}
)"