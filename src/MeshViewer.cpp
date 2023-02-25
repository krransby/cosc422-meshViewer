//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2021)
//
//  FILE NAME: MeshViewer.cpp
//  Triangle mesh viewer using OpenMesh and OpenGL-4
//  This program assumes that the mesh consists of only triangles.
//  The model is scaled and translated to the origin to fit within the view frustum
//
//  Use arrow keys to rotate the model
//  Use 'w' key to toggle between wireframe and solid fill modes
//  ========================================================================

#define _USE_MATH_DEFINES // for C++  
#include <cmath>  
#include <iostream>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/freeglut.h>
#include "Shader.h"
#include "loadTGA.h"
#include "saveTGA.h"
using namespace std;


typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;
MyMesh mesh;
float modelScale;
float xc, yc, zc;
float rotn_x = 0.0f, rotn_y = 0.0f;
GLuint vaoID;
GLuint mvpMatrixLoc, mvMatrixLoc, norMatrixLoc, lgtLoc, shadingLoc, creaseLoc, silhouLoc, texLoc, eyePosLoc, tTypeLoc;
glm::mat4 view, projView;
int num_Elems;
int shadingStyle = 0, tType = 0;
float silhouetteThickness = 2.5f, creaseThickness = 1.0f, zoom = 30.0f; //silhouetteThickness = 0.007f, creaseThickness = 0.001f

// For light rotation
glm::vec4 light = glm::vec4(5.0, 5.0, 10.0, 1.0);
float lgtAngle = 0;


// Gets the min max values along x, y, z for scaling and centering the model in the view frustum
void getBoundingBox(float& xmin, float& xmax, float& ymin, float& ymax, float& zmin, float& zmax)
{
	MyMesh::VertexIter vit = mesh.vertices_begin();
	MyMesh::Point pmin, pmax;

	pmin = pmax = mesh.point(*vit);

	//Iterate over the mesh using a vertex iterator
	for (vit = mesh.vertices_begin()+1; vit != mesh.vertices_end(); vit++)
	{
		pmin.minimize(mesh.point(*vit));
		pmax.maximize(mesh.point(*vit));
	}
	xmin = pmin[0];  ymin = pmin[1];  zmin = pmin[2];
	xmax = pmax[0];  ymax = pmax[1];  zmax = pmax[2];
}


//Generates Procedural textures
void generateTextures()
{
	int numLevels = 4;
	int size[4] = {64, 32, 16, 8};
	int scale[4] = { 16, 8, 4, 2 };
	string texNames[4] = { "a_", "b_", "c_", "d_" };
	float val, img64[64], img32[32], img16[16], img8[8];

	for (int i = 0; i < numLevels; i++) 
	{
		// Generate largest image
		for (int j = 63; j >= 0; j--)
		{
			val = (j % scale[i] == 0) ? 0.0f : 255.0f;
			
			img64[j] = val;
			if (j < 32) img32[j] = val;
			if (j < 16) img16[j] = val;
			if (j < 8) img8[j] = val;
		}

		// Save 4 different scale's of the same image
		saveTGA("assets/textures/procedural/" + texNames[i] + "64.tga", img64, 64, 64); // 64 x 64
		saveTGA("assets/textures/procedural/" + texNames[i] + "32.tga", img32, 32, 32); // 32 x 32
		saveTGA("assets/textures/procedural/" + texNames[i] + "16.tga", img16, 16, 16); // 16 x 16
		saveTGA("assets/textures/procedural/" + texNames[i] + "8.tga", img8, 8, 8);		//  8 x 8
	}
}


//Loads pensil texture
void loadTextures()
{
	GLuint texID[8];
	string texNames[8] = { "a_", "b_", "c_", "d_", "procedural/a_", "procedural/b_", "procedural/c_", "procedural/d_" };
	string mipmapLevel[4] = { "64", "32", "16", "8" };

	glGenTextures(size(texID), texID);

	for (int i = 0; i < size(texNames); i++) // texture ID
	{
		for (int j = 0; j < size(mipmapLevel); j++) // mipmap level 
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, texID[i]);
			loadTGA_mipmap("assets/textures/" + texNames[i] + mipmapLevel[j] + ".tga", j);
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
}


//Initialisation function for OpenMesh, shaders and OpenGL
void initialize()
{
	float xmin, xmax, ymin, ymax, zmin, zmax;
	float CDR = M_PI / 180.0f;


	//============= Load mesh ==================
	if (!OpenMesh::IO::read_mesh(mesh, "assets/models/Dolphin.obj"))
	{
		cerr << "Mesh file read error.\n";
	}
	getBoundingBox(xmin, xmax, ymin, ymax, zmin, zmax);

	xc = (xmin + xmax)*0.5f;
	yc = (ymin + ymax)*0.5f;
	zc = (zmin + zmax)*0.5f;
	OpenMesh::Vec3f box = { xmax - xc,  ymax - yc, zmax - zc };
	modelScale = 1.0 / box.max();


	//-------- Load textures -----------
	generateTextures();
	loadTextures();

	//============= Load shaders & Create program object ============
	GLuint program = createShaderProg(
		"src/shaders/MeshViewer.vert", 
		"src/shaders/MeshViewer.geom", 
		"src/shaders/MeshViewer.frag"
	);


	//============== Get vertex, normal data from mesh =========
	int num_verts = mesh.n_vertices();
	int num_faces = mesh.n_faces();
	float* vertPos = new float[num_verts * 3];
	float* vertNorm = new float[num_verts * 3];
	num_Elems = num_faces * 6;
	short* elems = new short[num_Elems];   //Asumption: Triangle mesh

	if (!mesh.has_vertex_normals())
	{
		mesh.request_face_normals();
		mesh.request_vertex_normals();
		mesh.update_normals();
	}

	MyMesh::VertexIter vit;  //A vertex iterator
	MyMesh::FaceIter fit;    //A face iterator
	MyMesh::FaceVertexIter fvit; //Face-vertex iterator
	OpenMesh::VertexHandle verH1, verH2;
	OpenMesh::HalfedgeHandle heH;
	OpenMesh::FaceHandle facH;
	MyMesh::Point pos;
	MyMesh::Normal norm;
	int indx;


	//Use a vertex iterator to get vertex positions and vertex normal vectors
	indx = 0;
	for (vit = mesh.vertices_begin(); vit != mesh.vertices_end(); vit++)
	{
		verH1 = *vit;
		pos = mesh.point(verH1);
		norm = mesh.normal(verH1);
		for (int j = 0; j < 3; j++)
		{
			vertPos[indx] = pos[j];
			vertNorm[indx] = norm[j];
			indx++;
		}
	}


	//Use a face iterator to get the vertex indices for each face
	indx = 0;
	for (fit = mesh.faces_begin(); fit != mesh.faces_end(); fit++)
	{
		facH = *fit;
		heH = mesh.halfedge_handle(facH);
		for (int i = 0; i < 3; i++) 
		{
			elems[indx] = mesh.from_vertex_handle(heH).idx();
			indx++;
			elems[indx] = mesh.opposite_he_opposite_vh(heH).idx();
			indx++;

			heH = mesh.next_halfedge_handle(heH);
		}
	}

	mesh.release_vertex_normals();


	//============== Load buffer data ==============
	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);

	GLuint vboID[3];
	glGenBuffers(3, vboID);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* num_verts * 3, vertPos, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);  // Vertex position

	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_verts * 3, vertNorm, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);  // Vertex normal

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboID[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * num_Elems, elems, GL_STATIC_DRAW);

	glBindVertexArray(0);


	//============== Create uniform variables ==============
	mvpMatrixLoc = glGetUniformLocation(program, "mvpMatrix");
	mvMatrixLoc = glGetUniformLocation(program, "mvMatrix");
	norMatrixLoc = glGetUniformLocation(program, "norMatrix");
	shadingLoc = glGetUniformLocation(program, "shadingMode");
	lgtLoc = glGetUniformLocation(program, "lightPos");
	creaseLoc = glGetUniformLocation(program, "creaseWidth");
	silhouLoc = glGetUniformLocation(program, "silhoueWidth");
	eyePosLoc = glGetUniformLocation(program, "EyePos");
	tTypeLoc = glGetUniformLocation(program, "tType");
	texLoc = glGetUniformLocation(program, "pencilStroke");  //uniform variables of type Sampler2D in fragment shader;
	int texVals[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	glUniform1iv(texLoc, 8, texVals);


	//glm::vec4 light = glm::vec4(5.0, 5.0, 10.0, 1.0);
	glm::mat4 proj;
	proj = glm::perspective(zoom * CDR, 1.0f, 2.0f, 10.0f);  //perspective projection matrix
	view = glm::lookAt(glm::vec3(0, 0, 4.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)); //view matrix
	projView = proj * view;
	glm::vec4 lightEye = view * light;
	glUniform4fv(lgtLoc, 1, &lightEye[0]);
	glUniform3fv(eyePosLoc, 1, &glm::vec3(0, 0, 4.0)[0]);

	//============== Initialize OpenGL state ==============
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

//Callback function for special keyboard events
void special(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		rotn_y -= 5.0;
		break;

	case GLUT_KEY_RIGHT:
		rotn_y += 5.0;
		break;

	case GLUT_KEY_UP:
		rotn_x -= 5.0;
		break;

	case GLUT_KEY_DOWN:
		rotn_x += 5.0;
		break;

	case GLUT_KEY_PAGE_UP: // zoom in
		zoom -= 1.0f;
		if (zoom < 10.0f)
		{
			zoom = 10.0f;
		}
		break;

	case GLUT_KEY_PAGE_DOWN: // zoom out
		zoom += 1.0f;
		if (zoom > 100.0f)
		{
			zoom = 100.0f;
		}
		break;

	default:
		break;
	}

	glutPostRedisplay();
}

//Callback function for keyboard events
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case ' ':
		shadingStyle = remainder(shadingStyle + 1, 2);
		break;
	
	case 'q':	// increase thickness of silhouette edges
		silhouetteThickness += 0.01f;
		if (silhouetteThickness > 10.0f)
		{
			silhouetteThickness = 10.0f;
		}
		break;

	case 'a':	// decrease thickness of silhouette edges
		silhouetteThickness -= 0.01f;
		if (silhouetteThickness < 0.0f)
		{
			silhouetteThickness = 0.0f;
		}
		break;

	case 'w':	// increase thickness of crease edges
		creaseThickness += 0.01f;
		if (creaseThickness > 10.0)
		{
			creaseThickness = 10.0f;
		}
		break;

	case 's':	// decrease thickness of crease edges
		creaseThickness -= 0.01f;
		if (creaseThickness < 0.0f)
		{
			creaseThickness = 0.0f;
		}
		break;

	case 'e': // Rotate light left
		lgtAngle++;
		if (lgtAngle >= 360.0)
		{
			lgtAngle = 0.0;
		}
		break;

	case 'r': // Rotate light right
		lgtAngle--;
		if (lgtAngle <= 0)
		{
			lgtAngle = 360.0;
		}
		break;

	case 't':
		tType = remainder(tType + 1, 2);
		break;
		

	default:
		break;
	}

	glutPostRedisplay();
}

//The main display callback function
void display()
{
	float CDR = M_PI / 180.0;
	glm::mat4 matrix = glm::mat4(1.0);
	matrix = glm::rotate(matrix, rotn_x * CDR, glm::vec3(1.0, 0.0, 0.0));  //rotation about x
	matrix = glm::rotate(matrix, rotn_y * CDR, glm::vec3(0.0, 1.0, 0.0));  //rotation about y
	matrix = glm::scale(matrix, glm::vec3(modelScale, modelScale, modelScale));
	matrix = glm::translate(matrix, glm::vec3(-xc, -yc, -zc));

	glm::mat4 viewMatrix = view * matrix;		//The model-view matrix
	glUniformMatrix4fv(mvMatrixLoc, 1, GL_FALSE, &viewMatrix[0][0]);

	glm::mat4 proj = glm::perspective(zoom * CDR, 1.0f, 2.0f, 10.0f);  //perspective projection matrix
	projView = proj * view;

	glm::mat4 prodMatrix = projView * matrix;   //The model-view-projection matrix
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, &prodMatrix[0][0]);

	glm::mat4 invMatrix = glm::inverse(viewMatrix);  //Inverse of model-view matrix
	glUniformMatrix4fv(norMatrixLoc, 1, GL_TRUE, &invMatrix[0][0]);

	// Light rotation
	glm::mat4 rotate = glm::translate(view, glm::vec3(0.0, 0.0, -10.0));
	rotate = glm::rotate(rotate, lgtAngle * CDR, glm::vec3(0.0, 1.0, 0.0));
	rotate = glm::translate(rotate, glm::vec3(0.0, 0.0, 10.0));
	glm::vec4 lightEye = rotate * light;     //Light position in eye coordinates
	glUniform4fv(lgtLoc, 1, &lightEye[0]);

	glUniform1i(tTypeLoc, tType);
	glUniform1i(shadingLoc, shadingStyle);
	glUniform1f(creaseLoc, creaseThickness);
	glUniform1f(silhouLoc, silhouetteThickness);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(vaoID);
	glDrawElements(GL_TRIANGLES_ADJACENCY, num_Elems, GL_UNSIGNED_SHORT, NULL);
	glFlush();
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB |GLUT_DEPTH);
	glutInitWindowSize(1000, 1000);
	glutInitWindowPosition(10, 10);
	glutCreateWindow("Assignment02 (OpenMesh) - Kayle Ransby | 34043590");
	glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	if (glewInit() == GLEW_OK)
	{
		cout << "GLEW initialization successful! " << endl;
		cout << " Using GLEW version " << glewGetString(GLEW_VERSION) << endl;
	}
	else
	{
		cerr << "Unable to initialize GLEW  ...exiting." << endl;
		exit(EXIT_FAILURE);
	}

	initialize();
	glutDisplayFunc(display);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	return 0;
}

