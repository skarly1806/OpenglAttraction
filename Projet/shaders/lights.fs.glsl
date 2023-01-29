#version 300 es
precision mediump float;

/* STRUCTURES */
#define MAX_LIGHTS 10
#define MAX_TEXTURES 2

struct Material {
    vec3 color;
    float shininess;
    float specularIntensity;
    bool hasTexture;
    bool isLamp;
    sampler2D textures[MAX_TEXTURES];
};

struct DirLight {
    vec3 direction;
    float intensity; // lamp intensity
    vec3 color; // lamp color
};

struct PointLight {
    vec3 position;
    float intensity; // lamp intensity
    vec3 color; // lamp color
};

/* IN VARIABLES */
in vec3 vPosition_vs;
in vec3 vNormal_vs;
in vec2 vTexCoords; // Coordonnées de texture du sommet

/* UNIFORM VARIABLES */
uniform vec3 uViewPos; // position camera

uniform Material uMaterial;

// LIGHTS
uniform DirLight[MAX_LIGHTS] uDirLights;

uniform PointLight[MAX_LIGHTS] uPointLights;

uniform vec2 NbLights; // x dir lights, y point lights

uniform vec3 uAmbiantLight;

/* OUT VARIABLES */
out vec3 fFragColor;


vec3 CalcDirLight(DirLight light){
	vec3 lightdir = normalize(-light.direction);

	float diff = dot(lightdir, normalize(vNormal_vs));
	vec3 diffColor = vec3(0);
	vec3 specColor = vec3(0);

	if(diff > 0.f){
		diffColor = vec3(light.color * light.intensity * diff);

		vec3 vertToEye = normalize(uViewPos - vPosition_vs);
		vec3 lightReflect = normalize(reflect(-lightdir, vNormal_vs));
		float spec = dot(vertToEye, lightReflect);

		if(spec > 0.f){
			spec = pow(spec, uMaterial.shininess);
			specColor = vec3(light.color * uMaterial.specularIntensity * spec);
		}
	}


	return diffColor + specColor;
}

vec3 CalcPointLight(PointLight light) {
	float d = distance(light.position, vPosition_vs);
	float Li = light.intensity / (d * d);

	vec3 lightdir = normalize(light.position - vPosition_vs);

	float diff = dot(lightdir, normalize(vNormal_vs));
	vec3 diffColor = vec3(0);
	vec3 specColor = vec3(0);

	if(diff > 0.f){
		diffColor = vec3(light.color * light.intensity * diff);

		vec3 vertToEye = normalize(uViewPos - vPosition_vs);
		vec3 lightReflect = normalize(reflect(-lightdir, vNormal_vs));
		float spec = dot(vertToEye, lightReflect);

		if(spec > 0.f){
			spec = pow(spec, uMaterial.shininess);
			specColor = vec3(light.color * uMaterial.specularIntensity * spec);
		}
	}


	return Li * (diffColor + specColor);
}

void main() {
	vec3 result = vec3(1);

	vec3 colorMat = uMaterial.color;
	if(uMaterial.hasTexture){
		colorMat = texture(uMaterial.textures[0], vTexCoords).rgb + texture(uMaterial.textures[1], vTexCoords).rgb;
	}

	if(!uMaterial.isLamp){
		result = uAmbiantLight;

		for(int i = 0; i<int(NbLights.x); i++)
			result += CalcDirLight(uDirLights[i]);

		for(int i = 0; i<int(NbLights.y); i++)
			result += CalcPointLight(uPointLights[i]);
	}

	fFragColor = result * colorMat;

};