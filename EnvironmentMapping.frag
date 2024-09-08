in vec3 vertexPos;
in vec3 normal;
in float fresnelRatio;

out vec3 outColor;

void main() {
	vec3 toEye = -vertexPos;
	vec3 toEyeDir = normalize(toEye);
	vec3 normalDir = normalize(normal);

	vec3 envColor = vec3(0.0f);
	if (envMappingMode == 1) { // reflection only
		vec3 refl = CalcReflectionDir(toEyeDir, normalDir);
		refl = vec3(inverse(transform.normalMat) * vec4(refl, 0.0f));
		vec3 reflectColor = CalcCubeMapColor(refl, CalcCubeMap(refl));
		envColor = reflectColor;
	} else if (envMappingMode == 2) { // refraction only
		vec3 refr = CalcRefractionDir(toEyeDir, normalDir, refractiveIndexRatio);
		refr = vec3(inverse(transform.normalMat) * vec4(refr, 0.0f));
		vec2 uv = CalcCubeMap(refr);
		vec3 refractColor = CalcCubeMapColor(refr, uv);
		envColor = refractColor;
	} else if (envMappingMode == 3) { // fresnel
		vec3 refractColor;
		vec3 refr_R = CalcRefractionDir(toEyeDir, normalDir, refractiveIndexRatio - refractiveWaveLengthOffsetRG);
		refr_R = vec3(inverse(transform.normalMat) * vec4(refr_R, 0.0f));
		refractColor.r = CalcCubeMapColor(refr_R, CalcCubeMap(refr_R)).r;

		vec3 refr_G = CalcRefractionDir(toEyeDir, normalDir, refractiveIndexRatio);
		refr_G = vec3(inverse(transform.normalMat) * vec4(refr_G, 0.0f));
		refractColor.g = CalcCubeMapColor(refr_G, CalcCubeMap(refr_G)).g;

		vec3 refr_B = CalcRefractionDir(toEyeDir, normalDir, refractiveIndexRatio + refractiveWaveLengthOffsetGB);
		refr_B = vec3(inverse(transform.normalMat) * vec4(refr_B, 0.0f));
		refractColor.b = CalcCubeMapColor(refr_B, CalcCubeMap(refr_B)).b;

		vec3 refl = CalcReflectionDir(toEyeDir, normalDir);
		refl = vec3(inverse(transform.normalMat) * vec4(refl, 0.0f));
		vec3 reflectColor = CalcCubeMapColor(refl, CalcCubeMap(refl));

		vec3 fresnelColor = mix(refractColor, reflectColor, fresnelRatio);
		envColor = fresnelColor;
	}

	outColor = envColor;
}
