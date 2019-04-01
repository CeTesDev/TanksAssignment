// Includes
#include <GL/glew.h>
#include <GL/glut.h>
#include <Shader.h>
#include <Vector.h>
#include <Matrix.h>
#include <Mesh.h>
#include <Texture.h>
#include <SphericalCameraManipulator.h>
#include <iostream>
#include <math.h>
#include <string>

// Function Prototypes
bool initGL(int argc, char** argv);
void display(void);
void keyboard(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void handleKeys();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);

// Screen size
int screenWidth  = 720;
int screenHeight = 720;

// Array of key states
bool keyStates[256];

// Maze dimensions and matrix
const int N = 8;        // rows
const int M = 10;       // columns
std::vector<char> maze; // MxN matrix

const float cubeSize = 8;
const float coinSize = 1.5f;
const float ballSize = 0.3f;

Matrix4x4 ProjectionMatrix;                 // Projection Matrix

// Shader variables
GLuint shaderProgramID;                     // Shader Program ID
GLuint MVMatrixUniformLocation;             // ModelView Matrix Uniform
GLuint ProjectionUniformLocation;           // Projection Matrix Uniform
GLuint TextureMapUniformLocation;           // Texture Map

// Vertex attributes
GLuint vertexPositionAttribute;             // Vertex Position Attribute
GLuint vertexNormalAttribute;               // Vertex Normal Attribute
GLuint vertexTexcoordAttribute;             // Vertex Texcoord Attribute

// Light properties
Vector3f lightPosition = Vector3f(0, 1, 1);
Vector3f ambient       = Vector3f(0.1f, 0.1f, 0.2f);
Vector3f specular      = Vector3f(1.0f, 1.0f, 1.0f);
float specularPower    = 50.0;

// Light uniforms
GLuint LightPositionUniformLocation;
GLuint AmbientUniformLocation;
GLuint SpecularUniformLocation;
GLuint SpecularPowerUniformLocation;

// Objects
Mesh cube, coin, ball;
Mesh chassis, backWheel, frontWheel, turret;

// Texture IDs
GLuint cubeTextureID, tankTextureID, ballTextureID, coinTextureID;

float time = 0;         // current time in seconds

// Tank properties
Vector3f tankPosition;              // current tank position
float tankAngle = 0;                // current tank angle

// Ball properties
Vector3f ballPosition;              // current ball position

// Loading maze from a file
bool loadMaze()
{
	// Open file
	std::ifstream file("maze.txt");
	if(!file)
	{
		std::cout << "Error opening maze.txt" << std::endl;
		return false;
	}

	// Read file
	char c = 0;
	while(file >> c)
	{
		// Add matrix element
		maze.push_back(c);
	}

	return true;
}


// Loading shaders
void loadShaders()
{
	// Create shaders
	shaderProgramID = Shader::LoadFromFile("shader.vert", "shader.frag");

	// Get vertex attributes
	vertexPositionAttribute = glGetAttribLocation(shaderProgramID, "aVertexPosition");
	vertexNormalAttribute   = glGetAttribLocation(shaderProgramID, "aVertexNormal");
	vertexTexcoordAttribute = glGetAttribLocation(shaderProgramID, "aVertexTexcoord");

	// Get uniforms
	MVMatrixUniformLocation      = glGetUniformLocation(shaderProgramID, "MVMatrix_uniform");
	ProjectionUniformLocation    = glGetUniformLocation(shaderProgramID, "ProjMatrix_uniform");
	LightPositionUniformLocation = glGetUniformLocation(shaderProgramID, "LightPosition_uniform"); 
	AmbientUniformLocation       = glGetUniformLocation(shaderProgramID, "Ambient_uniform"); 
	SpecularUniformLocation      = glGetUniformLocation(shaderProgramID, "Specular_uniform"); 
	SpecularPowerUniformLocation = glGetUniformLocation(shaderProgramID, "SpecularPower_uniform");
	TextureMapUniformLocation    = glGetUniformLocation(shaderProgramID, "Texture_uniform");
}

void initTexture(std::string filename, GLuint & textureID)
{
	// Generate texture and bind
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

	// Get texture Data
	int width = 0, height = 0;
	char* data = 0;
	Texture::LoadBMP(filename, width, height, data);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Cleanup data - copied to GPU
	delete[] data;
}


// Applying matrix 
void applyMatrix(Matrix4x4 matrix)
{
	// View matrix
	Matrix4x4 m;
	m.translate(cubeSize * -6, cubeSize * 2, cubeSize * -8);
	m.rotate(20, 1, 0, 0);

	// Model matrix
	m = m * matrix;

	// Set modelview matrix
	glUniformMatrix4fv(
		MVMatrixUniformLocation, // Uniform location
		1,                       // Number of Uniforms
		false,                   // Transpose Matrix
		m.getPtr());             // Pointer to Matrix Values
}


// Main Program Entry
int main(int argc, char** argv)
{	
	// Init OpenGL
	if(!initGL(argc, argv))
		return -1;

	// Init Key States to false;
	for(int i = 0 ; i < 256; i++)
		keyStates[i] = false;

	// Set up your program

	loadMaze();
	cube.loadOBJ("../models/cube.obj");
	coin.loadOBJ("../models/coin.obj");
	ball.loadOBJ("../models/ball.obj");
	chassis.loadOBJ("../models/chassis.obj");
	backWheel.loadOBJ("../models/back_wheel.obj");
	frontWheel.loadOBJ("../models/front_wheel.obj");
	turret.loadOBJ("../models/turret.obj");

	// Load textures
	initTexture("../models/crate.bmp", cubeTextureID);
	initTexture("../models/coin.bmp", coinTextureID);
	initTexture("../models/ball.bmp", ballTextureID);
	initTexture("../models/hamvee.bmp", tankTextureID);

	// Load OpenGL shaders
	loadShaders();

	tankPosition = Vector3f(cubeSize * (M - 1) * 0.7f, 0, cubeSize * (N - 1));
	tankAngle = -90;
	ballPosition = Vector3f(cubeSize * (M - 1) * 0.6f, cubeSize * 0.3f, cubeSize * (N - 1));

	// Enter main loop
	glutMainLoop();

	// Delete shader program
	glDeleteProgram(shaderProgramID);

	return 0;
}

// Function to Initlise OpenGL
bool initGL(int argc, char** argv)
{
	// Init GLUT
	glutInit(&argc, argv);

	// Set Display Mode
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);

	// Set Window Size
	glutInitWindowSize(screenWidth, screenHeight);

	// Window Position
	glutInitWindowPosition(200, 200);

	// Create Window
	glutCreateWindow("Tank Assignment");

	// Init GLEW
	if (glewInit() != GLEW_OK) 
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return false;
	}
	
	// Set Display function
	glutDisplayFunc(display);
	
	// Set Keyboard Interaction Functions
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyUp); 

	// Set Mouse Interaction Functions
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(motion);
	glutMotionFunc(motion);

	// Start start timer function after 100 milliseconds
	glutTimerFunc(100, Timer, 0);

	glEnable(GL_DEPTH_TEST);
	return true;
}

// Drawing cubes
void drawCubes()
{
	// For each cube
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		// '1' or '2' indicates blocks
		if(maze[i * M + j] > '0')
		{
			// Set cube position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, -cubeSize * 0.5f + 0.5f, cubeSize * i);
			m.scale(cubeSize * 0.5f, cubeSize * 0.5f, cubeSize * 0.5f);
			applyMatrix(m);

			// Draw cube
			glBindTexture(GL_TEXTURE_2D, cubeTextureID);
			cube.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
		}
	}
}

// Drawing coins
void drawCoins()
{
	// For each cube
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		// '2' indicates targets
		if(maze[i * M + j] == '2')
		{
			// Set coin position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, cubeSize, cubeSize * i);
			m.scale(coinSize, coinSize, coinSize);
			applyMatrix(m);

			// Draw coin
			glBindTexture(GL_TEXTURE_2D, coinTextureID);
			coin.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
		}
	}
}

// Drawing ball
void drawBall()
{
	// Set ball position and size
	Matrix4x4 ballMatrix;
	ballMatrix.translate(ballPosition.x, ballPosition.y, ballPosition.z);
	ballMatrix.scale(ballSize, ballSize, ballSize);
	applyMatrix(ballMatrix);

	// Draw ball
	glBindTexture(GL_TEXTURE_2D, ballTextureID);
	ball.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
}

// Drawing tank
void drawTank()
{
	// Set tank position and direction
	Matrix4x4 tankMatrix;
	tankMatrix.translate(tankPosition.x, tankPosition.y, tankPosition.z);
	tankMatrix.rotate(tankAngle, 0, 1, 0);
	applyMatrix(tankMatrix);

	// Draw chassis, back wheel, front wheel, turret
	glBindTexture(GL_TEXTURE_2D, tankTextureID);
	chassis.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
	backWheel.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
	frontWheel.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
	turret.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
}

// Display Loop
void display(void)
{
	// Handle keys
	handleKeys();

	// Set Viewport
	glViewport(0, 0, screenWidth, screenHeight);
	
	// Clear screen
	glClearColor(0.4f, 0.5f, 0.6f, 1);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Enable shader
	glUseProgram(shaderProgramID);

	// Set perspective projection matrix
	ProjectionMatrix.perspective(90, 1.0f, 0.01f, 1000.0f);
	glUniformMatrix4fv(
		ProjectionUniformLocation,  // Uniform location
		1,                          // Number of Uniforms
		false,                      // Transpose Matrix
		ProjectionMatrix.getPtr()); // Pointer to ModelViewMatrixValues

	// Set light properties
	glUniform3f(LightPositionUniformLocation, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform4f(AmbientUniformLocation, ambient.x, ambient.y, ambient.z, 1.0);
	glUniform4f(SpecularUniformLocation, specular.x, specular.y, specular.z, 1.0);
	glUniform1f(SpecularPowerUniformLocation, specularPower);

	// Enable texture mapping
	glUniform1i(TextureMapUniformLocation, 0);

	// Draw scene
	drawCubes();
	drawCoins();
	drawTank();
	drawBall();

	// Disable shader
	glUseProgram(0);

	// Swap Buffers and post redisplay
	glutSwapBuffers();
	glutPostRedisplay();
}

// Keyboard Interaction
void keyboard(unsigned char key, int x, int y)
{
	// Quits program when esc is pressed
	if (key == 27)	// esc key code
	{
		exit(0);
	}

	// Set key status
	keyStates[key] = true;
}

// Handle key up situation
void keyUp(unsigned char key, int x, int y)
{
	keyStates[key] = false;
}


// Handle Keys
void handleKeys()
{
	// Keys should be handled here
	if(keyStates['a'])
	{
	}
}

// Mouse Interaction
void mouse(int button, int state, int x, int y)
{
	glutPostRedisplay();
}

// Motion
void motion(int x, int y)
{
	glutPostRedisplay();
}

// Timer Function
void Timer(int value)
{

	// Call function again after 10 milli seconds
	glutTimerFunc(10, Timer, 0);
}



