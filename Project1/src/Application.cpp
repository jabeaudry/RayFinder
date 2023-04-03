#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"



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

#pragma warning(disable : 4996)
#include <thread>
#include <ctime>
#include <cmath>
#include <chrono>
#include <iostream>
#include <algorithm>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, upper, lower) (max(upper, min(x, lower)))



float startHour = 0.0f;
float startMinute = 0.0f;

glm::vec4 skyColour;
float cosZenith;
float interpolationSpeed = 0.01f; // Adjust this value to control the color transition speed

//gui variables
bool visualizeScene = false;
int windowDirection = 1;
float latitude = 45.499630f;
float longitude = -73.576073f;
int yday = 175;
bool timeOfDayOnly = false;
float selectedTime = 0.0f;
bool seeVisualizeSceneOptions = true;
float currentTime = 0.0f;
float timeForced = 0.0f;
float totalLight = -1.0f;

void calculateSkyColor(float zenith);

glm::vec3 calculatesunPosition(float latitude, float longitude, float time) {
	// For simplicity, we'll use a basic implementation of the SPA algorithm
	int yday = 175;
	float gha = 15.0f * (12.0f - time); // Greenwich Hour Angle
	float addedAngle = 0;
	if (windowDirection == 2) {
		addedAngle += 90;
	}
	else if (windowDirection == 3) {
		addedAngle += 180;
	}
	else if (windowDirection == 4) {
		addedAngle += 270;
	}
	else if (windowDirection == 1) {
		addedAngle += 0;
	}
	float declination = 23.45f * sin(glm::radians(360.0f * (284.0f + yday) / 365.0f)); // Declination angle

	float latRad = glm::radians(latitude);
	float lonRad = glm::radians(longitude);
	float decRad = glm::radians(declination);

	// Calculate the solar zenith angle
	cosZenith = sin(latRad) * sin(decRad) + cos(latRad) * cos(decRad) * cos(glm::radians(gha));
	float zenith = glm::acos(cosZenith);


	// Calculate the solar azimuth angle
	float sinAzimuth = cos(decRad) * sin(glm::radians(gha)) / sin(zenith);
	float cosAzimuth = (cosZenith - sin(latRad) * sin(zenith)) / (cos(latRad) * cos(zenith));
	float azimuth = glm::atan(sinAzimuth, cosAzimuth) ;

	// Modify the azimuth angle to make the sun's movement more visually circular
	azimuth += glm::radians(180.0f - longitude) ;

	float distance = 10.0f; // Set an arbitrary distance for the sun

	// Adjust the solar azimuth angle based on the window orientation
	switch (windowDirection) {
	case 1:
		break;
	case 2:
		azimuth += glm::radians(90.0f);
		break;
	case 3:
		azimuth += glm::radians(180.0f);
		break;
	case 4:
		azimuth += glm::radians(270.0f);
		break;
	}



	glm::vec3 sunPosition;
	sunPosition.x = distance* cos(zenith) * cos(azimuth);
	sunPosition.y = distance * sin(zenith);
	sunPosition.z = -distance * cos(zenith) * sin(azimuth);

	std::cout << sunPosition.z << "     " << sunPosition.y << std::endl;

	return sunPosition;
}

glm::vec4 normalizeColor(float r, float g, float b, float a) {

	r /= 255;
	g /= 255;
	b /= 255;

	return glm::vec4(r, g, b, a);
}

void calculateSkyColor(float zenith) {
	// Define day and night colors
	glm::vec4 dayColor = normalizeColor(161, 227, 255, 1);
	glm::vec4 nightColor = normalizeColor(2, 1, 4, 1);

	if (cosZenith > 0.00000000f) {
		// Clamp the zenith angle between 0 and PI radians (180 degrees)
		float clampedZenith = CLAMP(zenith, 0.0f, glm::pi<float>() / 2.0f);

		// Normalize the zenith angle between 0 and 1
		float t = clampedZenith / (glm::pi<float>() / 2.0f);

		// Linearly interpolate between day and night colors based on the zenith angle
		skyColour = glm::mix(dayColor, nightColor, t);
	}
	else {
		skyColour = nightColor;
	}
}


void interpolateSkyColor(glm::vec4 targetSkyColor, float interpolationSpeed) {
	skyColour = glm::mix(skyColour, targetSkyColor, interpolationSpeed);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(1.1f, 1.80f, -0.35f));   //for prod
//Camera camera(glm::vec3(1.1f, 21.80f, -0.35f));   //for testing






float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int planeVAO;






// renders the 3D scene
// --------------------
void renderScene(Shader_& shader, Model& room, Model& table, Model& plant, const glm::vec3& lightPos, const glm::mat4& lightSpaceMatrix)

{

	shader.setVec3("lightPos", lightPos);
	shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

	// render the room model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(1.1f, 1.1f, 1.1f));	// it's a bit too big for our scene, so scale it down
	shader.setMat4("model", model);
	room.Draw(shader);

	//// render the table model
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.20f, 0.22f, -3.50f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(0.00199f, 0.00199f, 0.00199f));	// it's a bit too big for our scene, so scale it down
	shader.setMat4("model", model);
	table.Draw(shader);

	// render the plant model
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.25f, 1.28f, -3.52f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(0.0018f, 0.0018f, 0.0018f));	// it's a bit too big for our scene, so scale it down
	shader.setMat4("model", model);
	plant.Draw(shader);

	// floor
	 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}













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
	//glfwSetCursorPosCallback(window, mouse_callback);
	//glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

	// Shaders
	// -----------------------------
	Shader_ shader("res/shaders/shader.vert.glsl", "res/shaders/shader.frag.glsl");
	Shader_ depthShader("res/shaders/depthShader.vert.glsl", "res/shaders/depthShader.frag.glsl");

	// load models
	Model table("res/obj/table/OBJ+MTL.obj");
	Model room("res/obj/room/untitled.obj");
	Model plant("res/obj/plant/Plant N310122.obj");


	// Framebuffer obj and depth texture to storedepth map
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	float planeVertices[] = {
		// positions            // normals         // texcoords
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};
	// plane VAO
	unsigned int planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);



	// Create a 2D texture for the depth map
	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
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

	shader.use();
	shader.setInt("shadowMap", 1);





	// ImGUI setup
	// ------------------
		
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

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

			calculateSkyColor(glm::acos(cosZenith));

			// Calculate the sun direction based on the currentTimeInfo
			currentTime = startHour + startMinute;
			glm::vec3 sunPosition = calculatesunPosition(latitude, longitude, currentTime);

			// render
			// ------
			glClearColor(skyColour.r, skyColour.g, skyColour.b, skyColour.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



			// imgui
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();


			if (startMinute >= 0.6f) {
				if (startHour >= 23.0f) {
					startMinute = 0.00f;
					startHour = 0.0f;
				}
				else {
					startHour += 1.0f;
					startMinute = 0.0f;
				}
			}
			else {
				startMinute += 0.05f;
			}



			std::this_thread::sleep_for(std::chrono::milliseconds(16));

			// 1. Render the depth map

			// Set up the light's projection and view matrices
			float near_plane = 1.0f;
			float far_plane = 20.0f;
			glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

			glm::vec3 sunDirection = glm::normalize(-sunPosition);
			glm::mat4 lightView = glm::lookAt(sunPosition,glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			// Calculate the light-space matrix
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			// Render your scene with the depth shader
			depthShader.use();

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 4);
			glCullFace(GL_FRONT);
			renderScene(depthShader, room, table, plant, sunPosition, lightSpaceMatrix);
			glCullFace(GL_BACK);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// reset viewport
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// 2. Render the scene with shadows
			shader.use();
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			shader.setMat4("projection", projection);
			shader.setMat4("view", view);
			// set light uniforms
			shader.setVec3("viewPos", camera.Position);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 4);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			renderScene(shader, room, table, plant, sunPosition, lightSpaceMatrix);

			ImGui::Begin("RayFinder Interface");

			
			if (seeVisualizeSceneOptions) {
				ImGui::Text("Select your window setup!");
				ImGui::Text("");
				ImGui::Text("Window orientation!");
				bool isChanged = false;
				ImGui::RadioButton("North", &windowDirection,1);\
				ImGui::RadioButton("West", &windowDirection,4);
				ImGui::RadioButton("East", &windowDirection, 2);
				ImGui::RadioButton("South", &windowDirection, 3);

				ImGui::Text("");

				ImGui::InputInt("Day",&yday, 1, 10, 1);
				ImGui::InputFloat("Lat", &latitude, 1, 2, "%.4f", 0);
				ImGui::InputFloat("Long", &longitude, 1, 2, "%.4f", 0);

				ImGui::Text("");

				ImGui::Checkbox("View at specific time only", &timeOfDayOnly);
				if (timeOfDayOnly) {
					ImGui::InputFloat("Time", &timeForced, 1, 2, "%.2f", 0);
				}
				ImGui::Text("");
				if (ImGui::Button("Visualize scene")) {
					visualizeScene = true;
					seeVisualizeSceneOptions = false;
				}
			}
			else {
				ImGui::Text("Currently running...");
				ImGui::Text("");
				ImGui::Text("Time: %f", currentTime);
				ImGui::Text("");
				if (!timeOfDayOnly) {
					ImGui::Text("This window provides: "); 
					if (totalLight < 3 && totalLight >= 0) {
						ImGui::Text("Shade");
					}
					else if (totalLight >= 3 && totalLight < 6) {
						ImGui::Text("Part Sun");
					}
					else if (totalLight >= 6) {
						ImGui::Text("Full Sun");
					}
					else if (totalLight < 0) {
						ImGui::Text("Calculating...");
					}
					ImGui::Text("");
				}
				
				if (ImGui::Button("Start over")) {
					visualizeScene = false;
					seeVisualizeSceneOptions = true;
					totalLight = -1.0f;
					currentTime = 0.0f;
				}
			}
			ImGui::End();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
			// -------------------------------------------------------------------------------
			glfwSwapBuffers(window);

		// glfw: swap buffers an poll IO events (keys pressed/released, mouse moved etc.)
		// ---------w);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


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
	//float xpos = static_cast<float>(xposIn);
	//float ypos = static_cast<float>(yposIn);

	//if (firstMouse)
	//{
	//	lastX = xpos;
	//	lastY = ypos;
	//	firstMouse = false;
	//}

	//float xoffset = xpos - lastX;
	//float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	//lastX = xpos;
	//lastY = ypos;

	//camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//camera.ProcessMouseScroll(static_cast<float>(yoffset));
}