///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 18 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}



/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	//sets parameters of the material for the shapes used in the scene.  Typically many different materials can be used for many different shapes but for this project, one material for the shapes used is sufficient.

	OBJECT_MATERIAL shinyMaterial;								//define the material for the shapes in the scene
	shinyMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);	//ambietn color of light reflected by the shapes material
	shinyMaterial.ambientStrength = 0.4f;						//ambient strength of the light reflected by the shapes material
	shinyMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);	//diffuse color of the shapes material, will be the color of the diffused light reflected by the shapes
	shinyMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);	//specular color of the shapes material, will be the color of the specular light reflected by the shapes
	shinyMaterial.shininess = 32.0;								//shininess of the shapes material, dermines the refelectiveness of the shapes this material is set to. 
	shinyMaterial.tag = "shinyShape";								//definition of the tag to call the material in the object rendering code to se the material properties in the shader before drawing the object. 
	m_objectMaterials.push_back(shinyMaterial);					//adds the material to the collection of materials. 

	//sets parameters of the material for the ground plane to have some contrast between the shapes used in the scene and the plane they are sitting on.

	OBJECT_MATERIAL planeMaterial;								//define the material for the  tabletop/plane
	planeMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);	//ambient color of the light reflected by the tabletop/plane material
	planeMaterial.ambientStrength = 0.2f;						//ambient strenght of the light reflected by the ground material
	planeMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);	//diffuse color of the object material, will be the color of diffused light reflected by the ground
	planeMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);	//specular color of the object material, will be the color of specular light reflected by the ground
	planeMaterial.shininess = 2.0;								//shininess of the object material, will determing the reflectiveness of the ground material, higher values will be more reflective
	planeMaterial.tag = "tabletop/plane";								//definition of the tag for the ground material, this will be used in the object rendering code to set the material properties in the shader before drawing the object
	m_objectMaterials.push_back(planeMaterial);				//adds the defined material to the collection of materials

	OBJECT_MATERIAL dullMaterial;								
	dullMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);	
	dullMaterial.ambientStrength = 0.5f;						
	dullMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);	
	dullMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);	
	dullMaterial.shininess = 3.5;								
	dullMaterial.tag = "dullShape";								
	m_objectMaterials.push_back(dullMaterial);
}
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("../textures/greenGlassTexture.png", "squirtBottle");
	CreateGLTexture("../textures/greenGlassTextureSquirt.png", "squirtBottleLogo");
	CreateGLTexture("../textures/squirt.jpg", "squirt");
	CreateGLTexture("../textures/greenPenHolderTexture.jpg", "penHolder");
	CreateGLTexture("../textures/marioBricks.png", "bricks");
	CreateGLTexture("../textures/rulerTexture.png", "ruler");
	CreateGLTexture("../textures/pencilWood.png", "pencil");
	CreateGLTexture("../textures/eraser.png", "eraser");
	CreateGLTexture("../textures/eraserMetal.jpg", "eraserMetal");
	CreateGLTexture("../textures/pencilWoodTip.png", "pencilTip");
	CreateGLTexture("../textures/graphite.jpg", "graphite");
	CreateGLTexture("../textures/tableTop.png", "tableTop");
	CreateGLTexture("../textures/redbullTexture.jpg", "redbull");
	CreateGLTexture("../textures/canTop.png", "canTop");
	CreateGLTexture("../textures/rokuRemote.png", "rokuRemoteTop");
	CreateGLTexture("../textures/rokuBlack.png", "rokuRemoteBottom");
	CreateGLTexture("../textures/bookCover.png", "ffxvBookCover");
	CreateGLTexture("../textures/lampShade.png", "lampShade");
	
	BindGLTextures();
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	//light source 1, point light source that will simulate the lightbulb in my lamp once I render it
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 6.0f, -10.0f);			//position of the light source in the 3D scene
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);	//minimal ambient light set to a soft white color
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);		//medium low diffuse light set to a soft white color, this will be the color of the diffused light reflected by the objects in the scene
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.5f);		//medium specular light set to a soft white color, this will be the color of the specular light reflected by the objects in the scene
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f);				//focal strength of the light source, this will determine how focused the light is, higher values will be more focused and lower values will be more spread out
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.75f);			//specular intensity of the light source, higher values will be brighter and lower values will be dimmer


	//light source 2, point light source high above the scene
	m_pShaderManager->setVec3Value("lightSources[1].position", -16.0f, 30.0f, 14.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 25.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f);

	//light source 3,point light source that behaves like ambient light
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 1.0f, 16.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.00f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();

	LoadSceneTextures();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	 //set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -20.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop");
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/****************************************************************/

	//*********************************Soda bottle creation.***************************************




		// Creating the tapered cylinder to make the top half of the soda bottle. 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.75f, 2.5f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 3.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(01.0f, 1.0f);
	SetShaderMaterial("shinyShape");

	SetShaderTexture("squirtBottleLogo");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Creating the cylinder to make the bottom half of the soda bottle. 

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 3.5f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("shinyShape");
	SetShaderTexture("squirtBottle");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	// Creating the cylinder to make the cap of the soda bottle. 

// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.40f, 0.05f, .40f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 6.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, .63, .145, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("shinyShape");
	SetShaderTexture("squirt");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	//*********************************End of soda bottle creation.***************************************
	//*********************************Table creation**************************************************



	// Creating the cylinder to make the table top.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, -1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.0f, 4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop"); 
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//table legs creation.	

		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.50f, -20.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.8f, -0.0f, 11.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop");
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.50f, -20.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.8f, -0.0f, 11.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop");
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.50f, -20.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.8f, -0.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop");
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.50f, -20.0f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.80f, -0.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("tableTop");
	SetShaderMaterial("tabletop/plane");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();






	//******************end of table creation.****************************************************************

	//***************************Pen cup creation.************************************************************





// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 3.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.79, 1, .419, 1);
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderTexture("penHolder");
	SetShaderMaterial("shinyShape");
	//top of pen cup creation.

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, .5f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.79, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("penHolder");
	SetShaderMaterial("shinyShape");

	//bottom of the pen cup 

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.1f, 1.0f, 2.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.88, .38, .078, 1);
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderTexture("bricks");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//hole of the pen cup 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, .5f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.05f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0, .0, 1);
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	//******************end of pen cup creation.*****************************************************

	//******************Stuff inside the pen cup.*****************************************************




	//Ruler creation.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.5f, .01f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(.75f, 3.05f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("ruler");
	SetShaderMaterial("dullShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//pens and pencils creation.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, 5.08f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.05f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0.5, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("pencil");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// eraser creation

	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, 0.05f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.315f, 4.88f, 5.852f);

	//set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.88, .38, .078, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("eraserMetal");
	SetShaderMaterial("shinyShape");

	m_basicMeshes->DrawCylinderMesh();

	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, 0.10f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.327f, 4.926f, 5.86f);

	//set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.88, .38, .078, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("eraser");
	SetShaderMaterial("dullShape");

	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, 5.0f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.05f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0.5, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("pencil");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// wood tip
	scaleXYZ = glm::vec3(.08f, .25f, .08f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.87f, 4.977f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.85f, 0.75f, 0.45f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("pencilTip");
	SetShaderMaterial("dullShape");
	m_basicMeshes->DrawTaperedCylinderMesh();


	// graphite point
	scaleXYZ = glm::vec3(.04f, .15f, .04f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 10.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.913f, 5.22f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.f, 1.f, 1.f, 1.0f);
	SetTextureUVScale(10.0f, 1.0f);
	SetShaderTexture("graphite");
	SetShaderMaterial("shinyShape");
	m_basicMeshes->DrawConeMesh();

	

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, 5.0f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.05f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0.5, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("pencil");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	
	//eraser creation
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .08f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 05.05f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0.5, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("eraserMetal");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//eraser creation
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.08f, .08f, .08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 05.12f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.0, 0.5, .0, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("eraser");
	SetShaderMaterial("dullShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//end of pens and pencils creation.*****************************************************



	//red bull can creation.****************************************************************
	// 
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.70f, 3.2f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.80f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderTexture("redbull");
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// rebull can top creation.
	scaleXYZ = glm::vec3(0.70f, 0.005f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.80f, 3.2f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderTexture("canTop");

	SetShaderMaterial("shinyShape");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	//*******************end of red bull can creation.*****************************************************

	//******************Create roku remote.*******************************************************






	scaleXYZ = glm::vec3(2.40f, .70f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.80f, 0.0f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);

	

	SetShaderMaterial("dullShape");
	SetShaderTexture("rokuRemoteBottom");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//roku remote top creation.

	scaleXYZ = glm::vec3(2.40f, .01f, .70f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.80f, 0.35f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("rokuRemoteTop");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//******************end of roku remote creation.*****************************************************



	//******************Books Creation********************************************************
	//lower book back cover creation.
	scaleXYZ = glm::vec3(7.40f, .11f, 5.500f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.80f, 0.0f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//lower book spine creation.*****************

	scaleXYZ = glm::vec3(7.40f, 1.0f, .11f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 00.0f;
	YrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.0f, 5.84f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//pages creation.
	scaleXYZ = glm::vec3(7.30f, 0.75f, 5.300f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.750f, 0.11f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	
//book top ****************

	scaleXYZ = glm::vec3(7.40f, .11f, 5.50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.80f, .4450f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, 1, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//******************Top Book*******************************


	scaleXYZ = glm::vec3(7.40f, .11f, 5.500f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 55.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.80f, .570f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, .6, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//book spine creation.*****************

	scaleXYZ = glm::vec3(7.40f, .11f, .500f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 125.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5f, .770f, 5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//**************pages of the top book creation.*****************	

	scaleXYZ = glm::vec3(7.350f, .4f, 5.450f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 55.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.78f, .770f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//****************book top creation.*****************
	scaleXYZ = glm::vec3(7.40f, .11f, 5.500f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 55.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.80f, .970f, 6.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.5, .6, .419, 1);
	SetTextureUVScale(1.0f, 1.0f);



	SetShaderMaterial("dullShape");
	SetShaderTexture("ffxvBookCover");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//******************end of books creation.*****************************************************



	//******************lamp creation.*******************************************************


	scaleXYZ = glm::vec3(3.0f, .50f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, .00f, -2.f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	
	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//lamp post creation.	

	scaleXYZ = glm::vec3(0.50f, 15.50f, .50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, .00f, -2.f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("shinyShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//LampShade creation.	

	scaleXYZ = glm::vec3(5.50f, 5.50f, 5.50f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.00f, -2.f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.470, .26, .08, 1);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("lampShade");
	SetShaderMaterial("dullShape");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
}
