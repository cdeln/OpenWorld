#version 420

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inVelocity;
out vec3 outPosition;
out vec3 outVelocity;

uniform layout(rgba32f) image2D snowMap;
uniform layout(rgba32f) image2D heightMap;

uniform sampler3D windMapX;
uniform sampler3D windMapZ;
uniform float mapSize;
uniform float mapScale;
uniform float time;

uniform vec3 mean;
uniform vec3 var;

// http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void)
{
	const float snowIncrease = 0.1;
	const float dt = 0.01;
	const float maxVel = 10;

	ivec2 texCoord = ivec2(inPosition.x/mapScale,inPosition.z/mapScale);
	vec4 heightVal = imageLoad(heightMap, texCoord);
	bool isOutsideMap = inPosition.x < 0 || inPosition.x > mapSize || inPosition.z < 0 || inPosition.z > mapSize;
	if( inPosition.y <= heightVal.r || isOutsideMap) 
	{
		float nx = var.x*rand(vec2(inPosition.x,inPosition.y));
		float nz = var.z*rand(vec2(inPosition.y,inPosition.z));
		float ny = var.y*rand(vec2(inPosition.z,inPosition.x));
		outPosition = mean + vec3(nx,ny,nz);
		outVelocity = vec3(0,0,0);
		if( ! isOutsideMap )
		{
			vec4 snowVal = imageLoad(snowMap, texCoord);
			vec4 newVal = snowVal + vec4(snowIncrease);
			imageStore(snowMap, texCoord, min(newVal,1));
		}
	}
	else
	{
		vec3 windTexCoord = vec3(inPosition.x, time, inPosition.z) / mapSize;
		windTexCoord *= 0.01;
		float windX = texture(windMapX, windTexCoord).r;
		float windZ = texture(windMapZ, windTexCoord).r;
		windX -= 0.5;
		windZ -= 0.5;
		windX *= 10;
		windZ *= 10;
		vec3 acc = vec3(windX,-9.82,windZ);
		float ax = rand(vec2(inPosition.x,inPosition.y))-0.5;
		float ay = rand(vec2(inPosition.y,inPosition.z))-0.5;
		float az = rand(vec2(inPosition.z, inPosition.x))-0.5;
		acc += 100*vec3(ax,ay,az);
		outVelocity = inVelocity + dt*acc;
        outVelocity = max(min(outVelocity, vec3(maxVel)), -vec3(maxVel));
		outPosition = inPosition + dt*outVelocity; //vec3(inPosition.x,inPosition.y-0.1,inPosition.z);
	}
	//gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
