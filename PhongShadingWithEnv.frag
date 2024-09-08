in vec3 vertexPos;
in vec3 normal;
in float fresnelRatio;

out vec3 outColor;
uniform sampler2D diffuseTex;
uniform sampler2D specularTex;
in VS_OUT {
	vec2 vertexUV;
} fs_in;

void main() {
	vec3 toEye = -vertexPos;
	vec3 toEyeDir = normalize(toEye);
	vec3 normalDir = normalize(normal);
	vec3 diffuseColor = texture(diffuseTex, fs_in.vertexUV).rgb;
	vec3 specularColor = texture(specularTex, fs_in.vertexUV).rgb;
	vec3 globalAmbientIntensity = vec3(phongProps.globalAmbientColor) * matProps.ambient.rgb;
	vec3 totalLocalIntensity = matProps.emissive.rgb + globalAmbientIntensity;
	float specularNS = specularColor.r * specularColor.r;

	vec3 envDirT = vec3(0.0f);
	vec3 envColor = vec3(0.0f);
	if (envMappingMode == 1) { // reflection only
		vec3 refl = CalcReflectionDir(toEyeDir, normalDir);
		envDirT = refl;	

		refl = vec3(inverse(transform.normalMat) * vec4(refl, 0.0f));
		vec3 reflectColor = CalcCubeMapColor(refl, CalcCubeMap(refl));
		envColor = reflectColor;
	} else if (envMappingMode == 2) { // refraction only
		vec3 refr = CalcRefractionDir(toEyeDir, normalDir, refractiveIndexRatio);
		envDirT = refr;

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
		envDirT = refr_G;	
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

	for (int i = 0; i < 16; ++ i) {
		if (i >= phongProps.numOfActiveLights)
			break;
		vec3 ambientIntensity = lightProps.ltI_ambient[i].rgb * matProps.ambient.rgb;
		if (lightProps.type[i] == 0) { // point light
			totalLocalIntensity += CalcLocalIntensityPointLightWithEnv(ambientIntensity,
																		diffuseColor,
																		specularColor,
																		lightProps.position[i].rgb,
																		normalDir,
																		lightProps.ltI_diffuse[i].rgb,
																		lightProps.ltI_specular[i].rgb,
																		specularNS,
																		lightProps.constantAttCoeff[i],
																		lightProps.linearAttCoeff[i],
																		lightProps.quadAttCoeff[i],
																		vertexPos,
																		toEyeDir,
																		envDirT,
																		envColor,
																		nt);
		} else if (lightProps.type[i] == 1) { // dir light
			totalLocalIntensity += CalcLocalIntensityDirectionalLightWithEnv(ambientIntensity,
																		diffuseColor,
																		specularColor,
																		lightProps.direction[i].rgb,
																		normalDir,
																		lightProps.ltI_diffuse[i].rgb,
																		lightProps.ltI_specular[i].rgb,
																		specularNS,
																		toEyeDir,
																		envDirT,
																		envColor,
																		nt);
		} else if (lightProps.type[i] == 2) { // spot light
			totalLocalIntensity += CalcLocalIntensitySpotLightWithEnv(ambientIntensity,
																		diffuseColor,
																		specularColor,
																		lightProps.position[i].rgb,
																		lightProps.direction[i].rgb,
																		normalDir,
																		lightProps.ltI_diffuse[i].rgb,
																		lightProps.ltI_specular[i].rgb,
																		specularNS,
																		lightProps.constantAttCoeff[i],
																		lightProps.linearAttCoeff[i],
																		lightProps.quadAttCoeff[i],
																		lightProps.spot_inner_angle[i],
																		lightProps.spot_outer_angle[i],
																		lightProps.spot_falloff[i],
																		vertexPos,
																		toEyeDir,
																		envDirT,
																		envColor,
																		nt);
		} else {
			break;
		}
	}
	float distToEye = length(toEye);
	float s = (phongProps.zFar - distToEye) / (phongProps.zFar - phongProps.zNear);
	vec3 finalIntensity = s * totalLocalIntensity + (1.0 - s) * phongProps.fogColor.rgb;
	outColor = mix(finalIntensity, envColor, phongEnvInterpolationFactor);
}
