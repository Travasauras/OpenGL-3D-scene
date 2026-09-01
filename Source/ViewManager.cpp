///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
// Updated by: Travis Erwin - SNHU Student / Computer Science, March 26, 2026
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
// all subsequent mouse moves can correctly calculate the X position offset and Y
// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
* Mouse_Scroll_Callback()
*
*  This method is automatically called from GLFW whenever the
*  the mouse scroll wheel is used within the active GLFW display window.
*
* ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xScrollOffset, double yScrollOffset) {   // this function is used to receive mouse scroll wheel events, it will allow 
	// the user to adjust the movement speed of the camera by using the scroll
	// wheel on the mouse. 

	g_pCamera->ProcessMouseScroll(yScrollOffset); // process the scroll wheel movement for speeding up and slowing down the camera movement speed. 
}


/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
	// if the camera object is null, then exit this method
	if (NULL == g_pCamera)
	{
		return;
	}

	// process camera moving in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)    // if the w key is pressed,
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);	// them move the camera forward
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)	// if the s key is pressed,
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);   // then move the camera backward
	}

	// process camera moving left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)    // if the a key is pressed,
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);		// then move the camera left
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)	// if the d key is pressed,
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);		// them move the camera right
	}

	// process camera moving up and down
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) // if the q key is pressed,
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);      // then move the camera up
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)  // if the e key is pressed,
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);	  // then move the camera down
	}

	//toggle between 2d and 3d
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) //if the o key is pressed,
	{
		bOrthographicProjection = true;					// then set the projection to orthographic projection
	}

	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) //if the p key is pressed,
	{
		bOrthographicProjection = false;				// then set the projection to perspective projection
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix with orthographic or perspective projection based on the current value of the bOrthographicProjection variable
	if (bOrthographicProjection == true)
	{
		projection = glm::ortho(
			-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);	// set the projection matrix to orthographic projection with the specified left, right, bottom, top, near and far values
	}
	else
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}