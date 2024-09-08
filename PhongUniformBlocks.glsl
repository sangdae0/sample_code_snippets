#version 440 core

#define PI 3.141592653

layout(binding = 0) uniform Transforms {
	mat4 modelMat;
	mat4 normalMat;
	mat4 viewMat;
	mat4 projMat;
} transform;

layout(binding = 1) uniform PhongProps {
	int numOfActiveLights;
	vec4 fogColor;
	vec4 globalAmbientColor;
	float zNear;
	float zFar;
	int useFaceNormal;
} phongProps;

layout(binding = 2) uniform LightProps {
	int type[16];
	vec4 position[16];
	vec4 direction[16];
	vec4 ltI_ambient[16];
	vec4 ltI_diffuse[16];
	vec4 ltI_specular[16];
	float spot_inner_angle[16];
	float spot_outer_angle[16];
	float spot_falloff[16];
	float constantAttCoeff[16];
	float linearAttCoeff[16];
	float quadAttCoeff[16];	
} lightProps;

layout(binding = 3) uniform MatProps {
	vec4 ambient;
	vec4 emissive;
	int mappingType;
	int calcMode;
	int texEntityMode;
} matProps;

uniform sampler2D envTexPositiveXAxis;
uniform sampler2D envTexNegativeXAxis;
uniform sampler2D envTexPositiveYAxis;
uniform sampler2D envTexNegativeYAxis;
uniform sampler2D envTexPositiveZAxis;
uniform sampler2D envTexNegativeZAxis;
uniform float refractiveIndexRatio;
uniform float refractiveWaveLengthOffsetRG;
uniform float refractiveWaveLengthOffsetGB;
uniform int envMappingMode;
uniform float phongEnvInterpolationFactor;
uniform float nt;

vec3 CalcLocalIntensityDirectionalLight(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPropDirection_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											vec3 toEyeDir_) {
	vec3 localIntensity = vec3(0.0);

	vec3 lightDir = -normalize(lightPropDirection_);
	float dotNL = max(dot(lightDir, normalDir_), 0);
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);
	if (dotNL > 0.0) {
		diffuseIntensity = ltIDiffuse_ 
							* diffuseColor_ 
							* dotNL;
		vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
		float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
		if (dotRV > 0.0) {
			specularIntensity = ltISpecular_ 
									* specularColor_ 
									* pow(dotRV, specularNS_);					
		}
	}
	localIntensity = (ambientIntensity_ + diffuseIntensity + specularIntensity);
	return localIntensity;
}

vec3 CalcLocalIntensityDirectionalLightWithEnv(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPropDirection_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											vec3 toEyeDir_,
											vec3 envDirT_,
											vec3 envColor_,
											float nt_) {
	vec3 localIntensity = vec3(0.0);

	vec3 lightDir = -normalize(lightPropDirection_);
	float dotNL = max(dot(lightDir, normalDir_), 0);
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);
	if (dotNL > 0.0) {
		diffuseIntensity = ltIDiffuse_ 
							* diffuseColor_ 
							* dotNL;
		vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
		float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
		if (dotRV > 0.0) {
			specularIntensity = ltISpecular_ 
									* specularColor_ 
									* pow(dotRV, specularNS_);					
		}
	}
	float dotTV = dot(envDirT_, toEyeDir_);
	vec3 envIntensity = ltISpecular_ * envColor_ * pow(max(dotTV, 0), nt_);

	localIntensity = (ambientIntensity_
						 + diffuseIntensity 
						 + specularIntensity
						 + envIntensity);
	return localIntensity;
}

vec3 CalcLocalIntensityPointLight(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPosition_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											float constantAttCoeff_,
											float linearAttCoeff_,
											float quadAttCoeff_,
											vec3 vertexPos_,
											vec3 toEyeDir_) {
	vec3 localIntensity = vec3(0.0);
	vec3 toLight = lightPosition_ - vertexPos_;
	vec3 lightDir = normalize(toLight);

	float dotNL = max(dot(lightDir, normalDir_), 0);
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);
	if (dotNL > 0.0) {
		diffuseIntensity = ltIDiffuse_
							* diffuseColor_ 
							* dotNL;
		vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
		float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
		if (dotRV > 0.0) {
			specularIntensity = ltISpecular_
									* specularColor_ 
									* pow(dotRV, specularNS_);					
		}
	}
	float distToLight = length(toLight);
	float att = min(1.0 / (constantAttCoeff_
						+ (linearAttCoeff_ * distToLight)
						+ (quadAttCoeff_ * distToLight * distToLight)),
					1.0);

	localIntensity = att * (ambientIntensity_ + diffuseIntensity + specularIntensity);
	return localIntensity;					
}

vec3 CalcLocalIntensityPointLightWithEnv(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPosition_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											float constantAttCoeff_,
											float linearAttCoeff_,
											float quadAttCoeff_,
											vec3 vertexPos_,
											vec3 toEyeDir_,
											vec3 envDirT_,
											vec3 envColor_,
											float nt_) {
	vec3 localIntensity = vec3(0.0);
	vec3 toLight = lightPosition_ - vertexPos_;
	vec3 lightDir = normalize(toLight);

	float dotNL = max(dot(lightDir, normalDir_), 0);
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);
	if (dotNL > 0.0) {
		diffuseIntensity = ltIDiffuse_
							* diffuseColor_ 
							* dotNL;
		vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
		float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
		if (dotRV > 0.0) {
			specularIntensity = ltISpecular_
									* specularColor_ 
									* pow(dotRV, specularNS_);		
		}
	}
	float dotTV = dot(envDirT_, toEyeDir_);
	vec3 envIntensity = ltISpecular_ * envColor_ * pow(max(dotTV, 0), nt_);

	float distToLight = length(toLight);
	float att = min(1.0 / (constantAttCoeff_
						+ (linearAttCoeff_ * distToLight)
						+ (quadAttCoeff_ * distToLight * distToLight)),
					1.0);

	localIntensity = att * (ambientIntensity_ 
								+ diffuseIntensity 
								+ specularIntensity
								+ envIntensity);
	return localIntensity;					
}

vec3 CalcLocalIntensitySpotLight(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPosition_,
											vec3 lightDirection_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											float constantAttCoeff_,
											float linearAttCoeff_,
											float quadAttCoeff_,
											float spotInnerAngle_,
											float spotOuterAngle_,
											float spotFallOff_,
											vec3 vertexPos_,
											vec3 toEyeDir_) {
	vec3 localIntensity = vec3(0.0);
	vec3 lightPos = lightPosition_;
	vec3 fromLightDir = normalize(lightDirection_);
	vec3 dDir = normalize(vertexPos_ - lightPos);
	float dotLD = dot(fromLightDir, dDir);

	float spotLightEffect = 0.0;
	float spotCosInner = cos(radians(spotInnerAngle_));
	float spotCosOuter = cos(radians(spotOuterAngle_));
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);	
	if (dotLD < spotCosOuter) { // when the vertex is out of outer cone
		spotLightEffect = 0.0; // optimize
	} else {
		if (dotLD > spotCosInner) // when the vertex is in inner
			spotLightEffect = 1.0;
		else // when the vertex is between inner and outer
			spotLightEffect = pow((dotLD - spotCosOuter) / (spotCosInner - spotCosOuter), spotFallOff_); // outer angle shoulde be bigger than inner angle
		vec3 lightDir = -fromLightDir;
		float dotNL = dot(lightDir, normalDir_);
		if (dotNL > 0.0) {
			diffuseIntensity = ltIDiffuse_
								* diffuseColor_ 
								* dotNL;
			vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
			float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
			if (dotRV > 0.0) {
				specularIntensity = ltISpecular_ 
										* specularColor_ 
										* pow(dotRV, specularNS_);					
			}
		}
	}

	float distToLight = length(lightPos - vertexPos_);
	float att = min(1.0 / (constantAttCoeff_
						+ (linearAttCoeff_ * distToLight)
						+ (quadAttCoeff_ * distToLight * distToLight)),
					1.0);
	localIntensity = (att * ambientIntensity_) 
								+ (att * spotLightEffect * (diffuseIntensity + specularIntensity));
	return localIntensity;
}

vec3 CalcLocalIntensitySpotLightWithEnv(vec3 ambientIntensity_,
											vec3 diffuseColor_,
											vec3 specularColor_,
											vec3 lightPosition_,
											vec3 lightDirection_,
											vec3 normalDir_,
											vec3 ltIDiffuse_,
											vec3 ltISpecular_,
											float specularNS_,
											float constantAttCoeff_,
											float linearAttCoeff_,
											float quadAttCoeff_,
											float spotInnerAngle_,
											float spotOuterAngle_,
											float spotFallOff_,
											vec3 vertexPos_,
											vec3 toEyeDir_,
											vec3 envDirT_,
											vec3 envColor_,
											float nt_) {
	vec3 localIntensity = vec3(0.0);
	vec3 lightPos = lightPosition_;
	vec3 fromLightDir = normalize(lightDirection_);
	vec3 dDir = normalize(vertexPos_ - lightPos);
	float dotLD = dot(fromLightDir, dDir);

	float spotLightEffect = 0.0;
	float spotCosInner = cos(radians(spotInnerAngle_));
	float spotCosOuter = cos(radians(spotOuterAngle_));
	vec3 diffuseIntensity = vec3(0.0);
	vec3 specularIntensity = vec3(0.0);	
	if (dotLD < spotCosOuter) { // when the vertex is out of outer cone
		spotLightEffect = 0.0; // optimize
	} else {
		if (dotLD > spotCosInner) // when the vertex is in inner
			spotLightEffect = 1.0;
		else // when the vertex is between inner and outer
			spotLightEffect = pow((dotLD - spotCosOuter) / (spotCosInner - spotCosOuter), spotFallOff_); // outer angle shoulde be bigger than inner angle
		vec3 lightDir = -fromLightDir;
		float dotNL = dot(lightDir, normalDir_);
		if (dotNL > 0.0) {
			diffuseIntensity = ltIDiffuse_
								* diffuseColor_ 
								* dotNL;
			vec3 reflectionDir = normalize(((2 * dotNL) * normalDir_) - lightDir);
			float dotRV = max(dot(reflectionDir, toEyeDir_), 0);
			if (dotRV > 0.0) {
				specularIntensity = ltISpecular_ 
										* specularColor_ 
										* pow(dotRV, specularNS_);					
			}
		}
	}
	float dotTV = dot(envDirT_, toEyeDir_);
	vec3 envIntensity = ltISpecular_ * envColor_ * pow(max(dotTV, 0), nt_);		

	float distToLight = length(lightPos - vertexPos_);
	float att = min(1.0 / (constantAttCoeff_
						+ (linearAttCoeff_ * distToLight)
						+ (quadAttCoeff_ * distToLight * distToLight)),
					1.0);
	localIntensity = (att * ambientIntensity_) 
								+ (att * spotLightEffect * (diffuseIntensity 
																+ specularIntensity
																+ envIntensity));
	return localIntensity;
}

vec2 CalcPlanarTexMapping(vec3 vEntity_,
							int axis_,
							bool signOfAxis_) {
	float uv_x = 0.0f;
	float uv_y = 0.0f;

	if (axis_ == 0) {
		if (signOfAxis_) { // +X
			uv_x = -vEntity_.z;
			uv_y = vEntity_.y;
		} else { // -X
			uv_x = vEntity_.z;
			uv_y = vEntity_.y;
		}
	} else if (axis_ == 1) {
		if (signOfAxis_) { // +Y
			uv_x = vEntity_.x;
			uv_y = -vEntity_.z;
		} else { // -Y
			uv_x = vEntity_.x;
			uv_y = vEntity_.z;
		}
	} else if (axis_ == 2) {
		if (signOfAxis_) { // +Z
			uv_x = vEntity_.x;
			uv_y = vEntity_.y;
		} else { // -Z
			uv_x = -vEntity_.x;
			uv_y = vEntity_.y;
		}
	}

	// convert [-1, 1] -> [0, 1]
	uv_x = (uv_x + 1.0f) * 0.5f;
	uv_y = (uv_y + 1.0f) * 0.5f;
	return vec2(uv_x, uv_y);
}

vec2 CalcCylindricalTexMapping(vec3 vEntity) {
	float uv_x = 0.0f;
	float uv_y = 0.0f;

	float theta = atan(vEntity.x, vEntity.z);
	uv_x = (theta + PI) / (2.0f * PI);
	uv_y = (vEntity.y + 1.0f) * 0.5f;
	return vec2(uv_x, uv_y);
}

vec2 CalcSphericalTexMapping(vec3 vEntity) {
	float uv_x = 0.0f;
	float uv_y = 0.0f;

	float r = sqrt((vEntity.x * vEntity.x)
					+ (vEntity.y * vEntity.y)
					+ (vEntity.z * vEntity.z));
	float theta = atan(vEntity.x, vEntity.z);

	float phi = acos(vEntity.y / r);

    uv_x = (theta + PI) / (2.0f * PI);
	uv_y = 1.0f - (phi / PI); // flip vertical
	return vec2(uv_x, uv_y);
}

vec2 CalcCubeMap(vec3 vEntity) {
	vec3 absVec = abs(vEntity);
	vec2 uv = vec2(0.0);

	// +-X
	if (absVec.x >= absVec.y && absVec.x >= absVec.z) {
		(vEntity.x < 0.0) ? (uv.x = vEntity.z) : (uv.x = -vEntity.z);
		uv.y = vEntity.y;
		uv /= absVec.x;
	} // +-Y
	else if (absVec.y >= absVec.x && absVec.y >= absVec.z) {
		uv.x = vEntity.x;
		(vEntity.y < 0.0) ? (uv.y = vEntity.z) : (uv.y = -vEntity.z);
		uv /= absVec.y;
	} // +-Z
	else if (absVec.z >= absVec.x && absVec.z >= absVec.y) {
		(vEntity.z < 0.0) ? (uv.x = -vEntity.x) : (uv.x = vEntity.x);
		uv.y = vEntity.y;
		uv /= absVec.z;
	}

	return (uv + vec2(1.0)) * 0.5;
}

vec3 CalcCubeMapColor(vec3 vEntity, vec2 calcedUv) {
	vec3 absVec = abs(vEntity);
	vec3 color = vec3(0.0);
	vec2 uvToUse = calcedUv;
	
	uvToUse.x = 1.0f - uvToUse.x;
	//uvToUse.y = 1.0f - uvToUse.y;
	
	// +-X
	if (absVec.x >= absVec.y && absVec.x >= absVec.z) {
		color = (vEntity.x < 0.0) ? (texture(envTexNegativeXAxis, uvToUse).rgb) : (texture(envTexPositiveXAxis, uvToUse).rgb);
	} // +-Y
	else if (absVec.y >= absVec.x && absVec.y >= absVec.z) {
		color = (vEntity.y < 0.0) ? (texture(envTexNegativeYAxis, uvToUse).rgb) : (texture(envTexPositiveYAxis, uvToUse).rgb);
	} // +-Z
	else if (absVec.z >= absVec.x && absVec.z >= absVec.y) {
		color = (vEntity.z < 0.0) ? (texture(envTexNegativeZAxis, uvToUse).rgb) : (texture(envTexPositiveZAxis, uvToUse).rgb);
	}
	return color;
}

vec3 CalcReflectionDir(vec3 toEyeDir_, vec3 nDir_) {
	return 2.0f * nDir_ * dot(nDir_, toEyeDir_) - toEyeDir_;
}

vec3 CalcRefractionDir(vec3 toEyeDir_,
						vec3 nDir_,
						float refractiveIndexRatio_) {
	float k = refractiveIndexRatio_;
	
	float dotNL = dot(nDir_, toEyeDir_);
	if (k > 1.0f) {
		float criticalAngle = asin(1.0f / k); // asin(n_t / n_i)
		float incidentAngle = acos(dotNL);
		if (incidentAngle > criticalAngle) { // total internal reflection
			return CalcReflectionDir(toEyeDir_, nDir_);
		}
	}

	vec3 refr = ((k * dotNL) - sqrt(1.0f - (k * k) * (1.0f - (dotNL * dotNL)))) * nDir_ 
					- (k * toEyeDir_);
	
	return refr;	
}