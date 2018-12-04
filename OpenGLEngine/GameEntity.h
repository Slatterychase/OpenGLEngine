#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"

/// <summary>
/// Represents one 'renderable' objet
/// </summary>
class GameEntity
{
private:
	Mesh* mesh;
	Material* material;

	//TODO - maybe this should be in a transform class?
	
	glm::vec3 eulerAngles;
	glm::vec3 scale;

	
	bool spin;
public:
	/// <summary>
	/// Basic paramterized constructor for most of our private vars
	/// </summary>
	GameEntity(
		Mesh* mesh,
		Material* material,
		glm::vec3 position,
		glm::vec3 eulerAngles,
		glm::vec3 scale
	);
	glm::mat4 worldMatrix;
	glm::vec3 position;
	/// <summary>
	/// Destruction
	/// </summary>
	virtual ~GameEntity();

	/// <summary>
	/// Updates the worldMatrix
	/// </summary>
	virtual void Update();

	/// <summary>
	/// Renders the gameEntity based on a camera
	/// </summary>
	void Render(Camera* camera);

};

