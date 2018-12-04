/*
This is the fragment shader, and it's main job is to determine the color for each pixel (fragment)
*/

//specifies the version of the shader (and what features are enabled)
#version 400 core
in vec3 Normal;
out vec4 color;


uniform vec3 objectColor;
uniform vec3 lightColor;


//entry point for the fragment shader
void main(void)
{
	// sets the output (color) to red
	float ambientStrength = 0.8;
	vec3 ambient = ambientStrength * lightColor;

	vec3 result = ambient * objectColor;
	color = vec4(result, 1.0);
	
}