#include "stdafx.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "GameEntity.h"
#include "Material.h"
#include "Input.h"
#include "stb_image.h"
#include "DynamicShader.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

//TODO - maybe make some #define macro for a print if debug?
//TODO - make an Engine class with a specific Init() and Run() function such that
//       our Main.cpp is kept clean and tidy
unsigned int loadSkybox(std::vector<std::string> faces) {

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
int main()
{
	

	{
		//init GLFW
		{
			if (glfwInit() == GLFW_FALSE)
			{
#ifdef _DEBUG
				std::cout << "GLFW failed to initialize" << std::endl;
				_CrtDumpMemoryLeaks();
				std::cin.get();
#endif
				return 1;
			}
		}
#ifdef _DEBUG
		std::cout << "GLFW successfully initialized!" << std::endl;
#endif // _DEBUG

		//create & init window, set viewport
		int width = 1200;
		int height = 800;
		GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Engine", nullptr, nullptr);
		{
			if (window == nullptr)
			{
#ifdef _DEBUG
				std::cout << "GLFW failed to create window" << std::endl;
				_CrtDumpMemoryLeaks();
				std::cin.get();
#endif
				glfwTerminate();
				return 1;
			}

			//tells OpenGL to use this window for this thread
			//(this would be more important for multi-threaded apps)
			glfwMakeContextCurrent(window);

			//gets the width & height of the window and specify it to the viewport
			int windowWidth, windowHeight;
			glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
			glViewport(0, 0, windowWidth, windowHeight);
		}
#ifdef _DEBUG
		std::cout << "Window successfully initialized!" << std::endl;
#endif // _DEBUG

		//init GLEW
		{
			if (glewInit() != GLEW_OK)
			{
#ifdef _DEBUG
				std::cout << "GLEW failed to initialize" << std::endl;
				_CrtDumpMemoryLeaks();
				std::cin.get();
#endif
				glfwTerminate();
				return 1;
			}
		}
#ifdef _DEBUG
		std::cout << "GLEW successfully initialized!" << std::endl;
#endif // _DEBUG

		//init the shader program
		//TODO - this seems like a better job for a shader manager
		//       perhaps the Shader class can be refactored to fit a shader program
		//       rather than be a thing for vs and fs
		GLuint shaderProgram = glCreateProgram();
		{

			//create vS and attach to shader program
			Shader *vs = new Shader();
			vs->InitFromFile("assets/shaders/vertexShader.glsl", GL_VERTEX_SHADER);
			glAttachShader(shaderProgram, vs->GetShaderLoc());

			//create FS and attach to shader program
			Shader *fs = new Shader();
			fs->InitFromFile("assets/shaders/fragmentShader.glsl", GL_FRAGMENT_SHADER);
			glAttachShader(shaderProgram, fs->GetShaderLoc());

			//link everything that's attached together
			glLinkProgram(shaderProgram);

			GLint isLinked;
			glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
			if (!isLinked)
			{
				char infolog[1024];
				glGetProgramInfoLog(shaderProgram, 1024, NULL, infolog);
#ifdef _DEBUG
				std::cout << "Shader Program linking failed with error: " << infolog << std::endl;
				std::cin.get();
#endif

				// Delete the shader, and set the index to zero so that this object knows it doesn't have a shader.
				glDeleteProgram(shaderProgram);
				glfwTerminate();
				_CrtDumpMemoryLeaks();
				return 1;
			}

			//everything's in the program, we don't need this
			delete fs;
			delete vs;
		}
		GLuint lightShaderProgram = glCreateProgram();
		{

			//same vertex shader as the cube, but the fragment shader will be different to produce a light
			Shader *lightVertex = new Shader();
			lightVertex->InitFromFile("assets/shaders/vertexShader.glsl", GL_VERTEX_SHADER);
			glAttachShader(lightShaderProgram, lightVertex->GetShaderLoc());

			//Different shader from the regular box shader
			Shader *lightFragment = new Shader();
			lightFragment->InitFromFile("assets/shaders/lightShader.glsl", GL_FRAGMENT_SHADER);
			glAttachShader(lightShaderProgram, lightFragment->GetShaderLoc());

			//link everything that's attached together
			glLinkProgram(lightShaderProgram);

			GLint isLinked;
			glGetProgramiv(lightShaderProgram, GL_LINK_STATUS, &isLinked);
			if (!isLinked)
			{
				char infolog[1024];
				glGetProgramInfoLog(lightShaderProgram, 1024, NULL, infolog);
#ifdef _DEBUG
				std::cout << "Light Shader Program linking failed with error: " << infolog << std::endl;
				std::cin.get();
#endif

				// Delete the shader, and set the index to zero so that this object knows it doesn't have a shader.
				glDeleteProgram(lightShaderProgram);
				glfwTerminate();
				_CrtDumpMemoryLeaks();
				return 1;
			}

			//everything's in the program, we don't need this
			delete lightFragment;
			delete lightVertex;
		}
			


#ifdef _DEBUG
		std::cout << "Shaders compiled attached, and linked!" << std::endl;
#endif // _DEBUG

		//init the mesh (a cube)
		//TODO - replace this with model loading
		glm::vec3 lightPosition = glm::vec3(0.f, 25.f, -5.f);
		GLfloat vertices[] = { 
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
		};
		
		DynamicShader skyboxShader("assets/shaders/skyboxVertex.glsl", "assets/shaders/skyboxFragment.glsl");
		float skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};
		unsigned int skyboxVAO, skyboxVBO;
		glGenVertexArrays(1, &skyboxVAO);
		glGenBuffers(1, &skyboxVBO);
		glBindVertexArray(skyboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


		
		//define skybox values, since they will remain the same
		std::vector<std::string> faces
		{
		"assets/Skymap/right.png",
		"assets/Skymap/left.png",
		"assets/Skymap/top.png",
		"assets/Skymap/bot.png",
		"assets/Skymap/front.png",
		"assets/Skymap/back.png"
		};

		unsigned int cubemapTexture = loadSkybox(faces);

		//TODO - maybe a CameraManager?
		Camera* myCamera = new Camera(
			glm::vec3(7.0f, 3.0f, -20.f),    //position of camera
			glm::vec3(0.0f, 0.0f, 1.f),     //the 'forward' of the camera
			glm::vec3(0.0f, 1.f, 0.0f),     //what 'up' is for the camera
			60.0f,                          //the field of view in radians
			(float)width,                   //the width of the window in float
			(float)height,                  //the height of the window in float
			0.01f,                          //the near Z-plane
			100.f                           //the far Z-plane
		);

		//create our mesh & material
		//TODO - maybe have a MeshManager & a MaterialManager
		Mesh* myMesh = new Mesh();
		myMesh->InitWithVertexArray(vertices, _countof(vertices), lightShaderProgram);
		glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
		glm::vec3 ambientColor = glm::vec3(.5f, 0.5f, .8f);
		Material* myMaterial = new Material(lightShaderProgram, lightColor, objectColor, lightPosition, myCamera->position, ambientColor , glm::vec3(1.0f, 0.5f, .31f), glm::vec3(0.5f, 0.5f, 0.5f), 64.0f);
		Mesh* lightMesh = new Mesh();
		lightMesh->InitWithVertexArray(vertices, _countof(vertices), shaderProgram);
		Material* lightMaterial = new Material(shaderProgram, lightColor, objectColor);
	


		//TODO - maybe a GameEntityManager?
		//All my game Entities are Cubes created and manually scattered across the camera
		//Next to each I calculated the magnitude of the position, since each is on the same Z we can do it in two dimensions.
		GameEntity* myGameEntity = new GameEntity(
			myMesh,
			myMaterial,
			glm::vec3(-5.f, 1.f, 1.f),
			glm::vec3(0.f, 0.f, 0.f),
			glm::vec3(4.f, 4.f, 4.f)
		);
		GameEntity* lightEntity = new GameEntity(
			lightMesh,
			lightMaterial,
			lightPosition,
			glm::vec3(0.f, 0.f, 0.f),
			glm::vec3(1.f, 1.f, 1.f)

		);
		
		


		
		skyboxShader.use();
		skyboxShader.setInt("skybox", 0);

		Input::GetInstance()->Init(window);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		//main loop
		while (!glfwWindowShouldClose(window))
		{
			
			
			/* INPUT */
			{
				//checks events to see if there are pending input
				glfwPollEvents();

				//breaks out of the loop if user presses ESC
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				{
					break;
				}
				
			}
			glm::mat4 view = myCamera->viewMatrix;
			
			myGameEntity->Update();
			lightEntity->Update();
			


			myCamera->Update(window, 0, 0);

			/* PRE-RENDER */
			{
				//start off with clearing the 'color buffer'
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				//clear the window to have c o r n f l o w e r   b l u e
				glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
			}

			/* RENDER */
			
				myGameEntity->Render(myCamera);
				lightEntity->Render(myCamera);
				glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
				skyboxShader.use();
				view = glm::mat4(glm::mat3(myCamera->viewMatrix)); // remove translation from the view matrix
				skyboxShader.setMat4("view", view);
				skyboxShader.setMat4("projection", myCamera->projectionMatrix);

				glBindVertexArray(skyboxVAO);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				glBindVertexArray(0);
				glDepthFunc(GL_LESS);
				myGameEntity->position.x += .001f;
				


			/* POST-RENDER */
			{
				//'clear' for next draw call
				glBindVertexArray(0);
				glUseProgram(0);
				//swaps the front buffer with the back buffer
				glfwSwapBuffers(window);
			}
		}

		//de-allocate our mesh!
		delete myMesh;
		delete myMaterial;
		delete myGameEntity;
	
		delete myCamera;
		
		Input::Release();
	}

	//clean up
	glfwTerminate();
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif // _DEBUG
	return 0;
}


