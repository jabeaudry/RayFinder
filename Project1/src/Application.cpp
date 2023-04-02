#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>


glm::vec3 calculateSunDirection(float timeOfDay) {
	float azimuth = glm::radians(360.0f * timeOfDay);
	float elevation = glm::radians(60.0f * cos(azimuth));

	glm::vec3 sunDirection;
	sunDirection.x = cos(elevation) * cos(azimuth);
	sunDirection.y = sin(elevation);
	sunDirection.z = cos(elevation) * sin(azimuth);

	return sunDirection;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 0.02f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "RayFinder", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader shader("res/shaders/shader.vert.glsl", "res/shaders/shader.frag.glsl");
	Shader depthShader("res/shaders/depthShader.vert.glsl", "res/shaders/depthShader.frag.glsl");

	// load models
	// -----------
	Model table("res/obj/table/OBJ+MTL.obj");
	Model room("res/obj/room/untitled.obj");
	Model plant("res/obj/plant/Plant N310122.obj");

	// Framebuffer obj and depth texture to storedepth map
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// Create a 2D texture for the depth map
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Attach the depth map to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.63f, 0.83f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update the sun direction
		float timeOfDay = static_cast<float>(glfwGetTime()) * 0.1f; // Adjust the multiplier to control the speed of the sun movement
		glm::vec3 sunDirection = calculateSunDirection(timeOfDay);

		// 1. Render the depth map
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClear(GL_DEPTH_BUFFER_BIT);
		depthShader.use();
		// Set up the light's projection and view matrices
		float near_plane = 1.0f;
		float far_plane = 7.5f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(sunDirection * -2.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Calculate the light-space matrix
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;
		depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		// Render your scene with the depth shader
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. Render the scene with shadows
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();
		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		shader.setInt("depthMap", 1);
		// Set other shader uniforms and render your scene


		// Set the directional light properties
		shader.setVec3("dirLight.direction", sunDirection);
		shader.setVec3("dirLight.ambient", 0.1f, 0.1f, 0.1f);
		shader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f);
		shader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);

		// render the room model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, -2.3f, 4.1f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(1.1f, 1.1f, 1.1f));	// it's a bit too big for our scene, so scale it down
		shader.setMat4("model", model);
		glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
		shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
		room.Draw(shader);
		shader.setInt("reverse_normals", 0); // and of course disable it
		glEnable(GL_CULL_FACE);

		//// render the table model
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.28f,-2.21f, 0.23f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.00199f, 0.00199f, 0.00199f));	// it's a bit too big for our scene, so scale it down
		shader.setMat4("model", model);
		table.Draw(shader);

		// render the plant model
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.25f, -1.2f, 0.23f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.0018f, 0.0018f, 0.0018f));	// it's a bit too big for our scene, so scale it down
		shader.setMat4("model", model);
		glDisable(GL_CULL_FACE);
		plant.Draw(shader);
		glEnable(GL_CULL_FACE);



		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteFramebuffers(1, &depthMapFBO);
	glDeleteTextures(1, &depthMap);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}



// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}