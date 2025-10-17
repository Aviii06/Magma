#version 450

// Output color to fragment shader
layout (location = 0) out vec3 outColor;

void main()
{
	// Hardcoded triangle positions
	const vec3 positions[3] = vec3[3](
		vec3(0.0, -0.5, 0.0),
		vec3(0.5, 0.5, 0.0),
		vec3(-0.5, 0.5, 0.0)
	);

	// Hardcoded vertex colors (red, green, blue)
	const vec3 colors[3] = vec3[3](
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);

	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
	outColor = colors[gl_VertexIndex];
}

