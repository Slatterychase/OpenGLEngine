/*
This is the fragment shader, and it's main job is to determine the color for each pixel (fragment)
*/

//specifies the version of the shader (and what features are enabled)
#version 400 core
in vec3 Normal;
in vec3 FragPos;
out vec4 color;


uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;


//entry point for the fragment shader
void main(void)
{
	//ambient light
	float ambientStrength = 0.4;
	vec3 ambient = ambientStrength * lightColor;
	//diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff*lightColor;

	vec3 result = (ambient + diffuse) * objectColor;
	color = vec4(result, 1.0);
	
}