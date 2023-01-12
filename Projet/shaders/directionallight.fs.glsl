#version 300 es
precision mediump float;

in vec3 vPosition_vs; // Position du sommet transformé dans l'espace View
in vec3 vNormal_vs; // Normale du sommet transformé dans l'espace View
in vec2 vTexCoords; // Coordonnées de texture du sommet

// materiau
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uShininess;

// lumiere
uniform vec3 uLightDir_vs;
uniform vec3 uLightIntensity;

out vec3 fFragColor;

vec3 blinnPhong(vec3 position_vs, vec3 normal_vs){

	vec3 w_zero = normalize(-position_vs);
	vec3 w_i = normalize(-uLightDir_vs);
	vec3 halfVector = (w_zero + w_i) / 2.f;

	return uLightIntensity * ( uKd * ( dot(w_i, normal_vs ) ) + uKs * ( pow( dot(halfVector, normal_vs), uShininess ) ) );
}

void main() {

    fFragColor = blinnPhong(vPosition_vs, normalize(vNormal_vs));
}