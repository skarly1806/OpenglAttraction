#version 300 es
precision mediump float;

/* STRUCTURES */
#define MAX_LIGHTS 10
#define MAX_TEXTURES 2

struct Material {
    vec3 kd; // material color
    vec3 ks;
    float shininess;
    bool hasTexture;
    bool isLamp;
    sampler2D textures[MAX_TEXTURES];
};

struct DirLight {
    vec3 direction;
    vec3 intensity; // lamp color
};

struct PointLight {
    vec3 position;
    vec3 intensity; // lamp color
};

/* IN VARIABLES */
in vec3 vPosition_vs;
in vec3 vNormal_vs;

/* UNIFORM VARIABLES */
uniform Material uMaterial;

uniform DirLight uDirLight;

uniform PointLight uPointLight;

uniform vec3 uAmbiantLight;

/* OUT VARIABLES */
out vec3 fFragColor;

vec3 CalcPointLight() {
	vec3 w0 = normalize(-vPosition_vs);
	vec3 wi = normalize(uPointLight.position - vPosition_vs);
	float d = distance(uPointLight.position, vPosition_vs);
	vec3 Li = uPointLight.intensity / (d * d);
	vec3 halfVector = (w0 + wi) / 2.f;
	vec3 N = vNormal_vs;
	return Li * (uMaterial.kd * dot(wi, N) + uMaterial.ks * pow((dot(halfVector, N)), uMaterial.shininess));
}

void main() {
	if (uMaterial.ks == vec3(1, 1, 1))
		fFragColor = vec3(1, 1, 1);
	else
	{
		vec3 result = vec3(0);

		result+=CalcPointLight();

		fFragColor = result;
		if(!uMaterial.isLamp)
			fFragColor += uAmbiantLight;
	}
};