layout(location = 0) in vec3 modelPosition;
layout(location = 1) in vec3 faceNormal;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec2 vertexUV;

out vec3 vertexPos;
out vec3 normal;
out float fresnelRatio;

void main() {
	vec3 modelPos = vec3(transform.modelMat * vec4(modelPosition, 1.0f));
	vertexPos = vec3(transform.viewMat * vec4(modelPos, 1.0f));
	vec3 normalToUse = phongProps.useFaceNormal == 1 ? faceNormal : vertexNormal;
	normal = vec3(transform.normalMat * vec4(normalToUse, 0.0f));

	gl_Position = transform.projMat * vec4(vertexPos, 1.0f);

	if (envMappingMode == 3) { // fresnell
		vec3 toEyeDir = normalize(-vertexPos);
		vec3 nDir = normalize(normal);
		float FresnelPower = 5.0;
		float F = ((1.0f - refractiveIndexRatio) * (1.0f - refractiveIndexRatio))
					/ ((1.0f + refractiveIndexRatio) * (1.0f + refractiveIndexRatio));
		fresnelRatio = F + (1.0f - F) * pow((1.0f - dot(nDir, toEyeDir)), FresnelPower);
	}
}