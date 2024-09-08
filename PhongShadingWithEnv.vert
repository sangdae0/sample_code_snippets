layout(location = 0) in vec3 modelPosition;
layout(location = 1) in vec3 faceNormal;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec2 vertexUV;

out vec3 vertexPos;
out vec3 normal;
out float fresnelRatio;

out VS_OUT {
	vec2 vertexUV;
} vs_out;

void main() {
	vec3 modelPos = vec3(transform.modelMat * vec4(modelPosition, 1.0f));
	vertexPos = vec3(transform.viewMat * vec4(modelPos, 1.0f));
	vec3 normalToUse = phongProps.useFaceNormal == 1 ? faceNormal : vertexNormal;
	normal = vec3(transform.normalMat * vec4(normalToUse, 0.0f));

	gl_Position = transform.projMat * vec4(vertexPos, 1.0f);

	if (matProps.calcMode == 0) {
		vs_out.vertexUV = vertexUV;
	} else if (matProps.calcMode == 1) {
		vec3 texEntity = vec3(0.0);
		if (matProps.texEntityMode == 0)
			texEntity = modelPos;
		else
			texEntity = normalize(normalToUse);

		if (matProps.mappingType == 0) {
			vs_out.vertexUV = CalcSphericalTexMapping(texEntity);
		} else if (matProps.mappingType == 1) {
			vs_out.vertexUV = CalcCylindricalTexMapping(texEntity);
		} else if (matProps.mappingType == 2) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 0, true);
		} else if (matProps.mappingType == 3) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 0, false);
		} else if (matProps.mappingType == 4) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 1, true);
		} else if (matProps.mappingType == 5) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 1, false);
		} else if (matProps.mappingType == 6) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 2, true);
		} else if (matProps.mappingType == 7) {
			vs_out.vertexUV = CalcPlanarTexMapping(texEntity, 2, false);
		}		
	}

	if (envMappingMode == 3) { // fresnell
		vec3 toEyeDir = normalize(-vertexPos);
		vec3 nDir = normalize(normal);
		float FresnelPower = 5.0;
		float F = ((1.0f - refractiveIndexRatio) * (1.0f - refractiveIndexRatio))
					/ ((1.0f + refractiveIndexRatio) * (1.0f + refractiveIndexRatio));
		fresnelRatio = F + (1.0f - F) * pow((1.0f - dot(nDir, toEyeDir)), FresnelPower);
	}
}