#include "Material.h"

Material::Material(GLuint shaderProgram, glm::vec3 colorL, glm::vec3 colorO)
{
	this->shaderProgram = shaderProgram;
	this->colorLght = colorL;
	this->colorObj = colorO;
}

Material::~Material()
{
}

void Material::Bind(Camera * camera, glm::mat4 worldMatrix)
{
	
	//enable shader
	//TODO - refactor this binding call to be managed by a RenderManager
	//       the RenderManager can be in charge of grouping material with same
	//       shaders together and passing certain uniforms on a per-frame or 
	//       per-object basis.
	glUseProgram(shaderProgram);

	//get location of the view matrix in the shader
	//TODO - cache this as a private varaible (on init) because the location is same every frame
	GLuint viewMatLoc = glGetUniformLocation(
		shaderProgram,  //the shader program to look for
		"viewMatrix"    //the name of the variable
	);

	//pass the variable up to our shader
	glUniformMatrix4fv(
		viewMatLoc,     //location of the uniform
		1,              //'count' of the uniforms
		GL_FALSE,       //we don't need to transpose this matrix
		&(camera->GetView()[0][0])   //the location of the first index
	);

	//get projection matrix location, and feed the value
	//TODO - cache this as a private varaible (on init) because the location is same every frame
	GLuint projectionMatLoc = glGetUniformLocation(shaderProgram, "projectionMatrix");
	glUniformMatrix4fv(projectionMatLoc, 1, GL_FALSE, &(camera->GetProjection()[0][0]));

	//do the same for our model to world matrix
	//TODO - cache this as a private varaible (on init) because the location is same every frame
	GLuint modelToWorldLoc = glGetUniformLocation(shaderProgram, "modelToWorld");
	glUniformMatrix4fv(modelToWorldLoc, 1, GL_FALSE, &(worldMatrix[0][0]));

	GLuint colorLamp = glGetUniformLocation(shaderProgram, "lightColor");
	glUniform3fv(colorLamp,1, &colorLght[0]);

	GLuint colorObject = glGetUniformLocation(shaderProgram, "objectColor");
	glUniform3fv(colorObject,1, &colorObj[0]);


}
