#version 450

// Input color from vertex shader
layout (location = 0) in vec3 inColor;

// Output final color
layout (location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = vec4(inColor, 1.0);
}

