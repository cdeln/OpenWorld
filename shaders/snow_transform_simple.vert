#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inVelocity;
out vec3 outPosition;
out vec3 outVelocity;

//uniform layout(rgba32f) image2D snowMap;
//uniform layout(rgba32f) image2D heightMap;

uniform sampler3D windMapX;
uniform sampler3D windMapZ;
uniform float mapSize;
uniform float mapScale;
uniform float time;

uniform float maxX;
uniform float maxZ;
uniform float seedY;

// http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void)
{
	const float dt = 0.01;
	const vec3 maxVel = vec3(10,10,10);
	const vec3 minVel = -vec3(10,10,10);

	if( inPosition.y < 0 ) 
	{
		float nx = maxX*rand(vec2(inPosition.x,inPosition.y));
		float nz = maxZ*rand(vec2(inPosition.y,inPosition.z));
		outPosition = vec3(nx,seedY,nz);
		outVelocity = vec3(0,0,0);
	}
	else
	{
		vec3 windTexCoord = vec3(inPosition.x, time, inPosition.z) / mapSize;
		windTexCoord *= 0.01;
		float windX = texture(windMapX, windTexCoord).r;
		float windZ = texture(windMapZ, windTexCoord).r;
		windX -= 0.5;
		windZ -= 0.5;
		windX *= 100;
		windZ *= 100;
		vec3 acc = vec3(windX,-9.82,windZ);
		float ax = rand(vec2(inPosition.x,inPosition.y))-0.5;
		float ay = rand(vec2(inPosition.y,inPosition.z))-0.5;
		float az = rand(vec2(inPosition.z, inPosition.x))-0.5;
		acc += 100*vec3(ax,ay,az);
		outVelocity = inVelocity + dt*acc;
        outVelocity = max(min(outVelocity, maxVel), minVel);
		outPosition = inPosition + dt*outVelocity;
	}
}
