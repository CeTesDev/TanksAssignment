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

// Object sizes
const float cubeSize = 10;
const float coinSize = 1;
const float ballSize = 0.3f;

// Screen size
int screenWidth  = 720;
int screenHeight = 720;

// Maze dimensions and integer matrix
const int N = 8;       // rows
const int M = 10;      // columns
std::vector<int> maze; // MxN matrix

// Objects and texture IDs
Mesh cube, coin, ball, chassis, backWheel, frontWheel, turret;
GLuint cubeTextureID, tankTextureID, ballTextureID, coinTextureID;

// Tank properties
Vector3f tankPosition;
float tankDistanceTravelled = 0;
float tankAngle = 0;
float tankAcceleration = 0;
float tankDeceleration = 0;
float tankVelocity = 0;
float tankAngleVelocity = 0;
float tankFallVelocity = 0;

// Ball properties
Vector3f ballPosition;

// Game properties
SphericalCameraManipulator cameraManip;
bool gameOver = false;

// Shader variables
GLuint shaderProgramID;
GLuint MVMatrixUniformLocation;
GLuint ProjectionUniformLocation;
GLuint TextureMapUniformLocation;

// Vertex attribute locations
GLuint vertexPositionAttribute;
GLuint vertexNormalAttribute;
GLuint vertexTexcoordAttribute;

// Light properties
Vector3f lightPosition = Vector3f(0, 1, 1);
Vector3f ambient       = Vector3f(0.1f, 0.1f, 0.2f);
Vector3f specular      = Vector3f(1.0f, 1.0f, 1.0f);
float specularPower    = 50.0;

// Light uniform locations
GLuint LightPositionUniformLocation;
GLuint AmbientUniformLocation;
GLuint SpecularUniformLocation;
GLuint SpecularPowerUniformLocation;

// Array of key states
bool keyStates[256];

// Converting degrees to radians
float degreesToRadians(float angle)
{
	return angle * (float)M_PI / 180;
}

// Converting radians to degrees
float radiansToDegrees(float angle)
{
	return angle * 180 / (float)M_PI;
}

// Rotating around point
void rotateAroundPoint(Matrix4x4 & matrix, const Vector3f & point, float xAngle, float yAngle)
{
    // Move to point, rotate, and move back
	matrix.translate(point.x, point.y, point.z);
	matrix.rotate(yAngle, 0, 1, 0);
	matrix.rotate(xAngle, 1, 0, 0);
	matrix.translate(-point.x, -point.y, -point.z);
}

// Loading maze from file
bool loadMaze()
{
	// Open file
	std::ifstream file("../models/maze.txt");
	if(!file)
	{
		std::cout << "Error opening ../models/maze.txt" << std::endl;
		return false;
	}

	// Read file
	char c = 0;
	while(file >> c)
	{
		// Convert char to integer
		maze.push_back(c - '0');
	}

	return true;
}

// Checking for blocks
bool isBlock(int i, int j)
{
	// Bound checking
	if(i < 0 || i > N - 1) return false;
	if(j < 0 || j > M - 1) return false;
	int index = i * M + j;
	if(index < 0 || index > (int)maze.size()) return false;

	// 1 or 2 indicates blocks
	return maze[index] == 1 || maze[index] == 2;
}

// Checking for targets
bool isTarget(int i, int j)
{
	// Bound checking
	if(i < 0 || i > N - 1) return false;
	if(j < 0 || j > M - 1) return false;
	int index = i * M + j;
	if(index < 0 || index > (int)maze.size()) return false;

	// 2 indicates targets
	return maze[index] == 2;
}

// Loading shaders
void loadShaders()
{
	// Create shader
	shaderProgramID = Shader::LoadFromFile("../models/shader.vert", "../models/shader.frag");

	// Get vertex attribute locations
	vertexPositionAttribute = glGetAttribLocation(shaderProgramID, "aVertexPosition");
	vertexNormalAttribute = glGetAttribLocation(shaderProgramID,   "aVertexNormal");
	vertexTexcoordAttribute = glGetAttribLocation(shaderProgramID, "aVertexTexcoord");

	// Get uniforms locations
	MVMatrixUniformLocation      = glGetUniformLocation(shaderProgramID, "MVMatrix_uniform");
	ProjectionUniformLocation    = glGetUniformLocation(shaderProgramID, "ProjMatrix_uniform");
	LightPositionUniformLocation = glGetUniformLocation(shaderProgramID, "LightPosition_uniform");
	AmbientUniformLocation       = glGetUniformLocation(shaderProgramID, "Ambient_uniform");
	SpecularUniformLocation      = glGetUniformLocation(shaderProgramID, "Specular_uniform");
	SpecularPowerUniformLocation = glGetUniformLocation(shaderProgramID, "SpecularPower_uniform");
	TextureMapUniformLocation    = glGetUniformLocation(shaderProgramID, "Texture_uniform");
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
	cubeTextureID = Texture::LoadBMP("../models/crate.bmp");
	coinTextureID = Texture::LoadBMP("../models/coin.bmp");
	ballTextureID = Texture::LoadBMP("../models/ball.bmp");
	tankTextureID = Texture::LoadBMP("../models/hamvee.bmp");

	// Load OpenGL shaders
	loadShaders();

	tankPosition = Vector3f(cubeSize * (M - 1) * 0.7f, 0, cubeSize * (N - 1));
	tankAngle = 180;
	ballPosition = Vector3f(cubeSize * (M - 1) * 0.6f, cubeSize * 0.3f, cubeSize * (N - 1));

	// Set up camera manipulator
	cameraManip.setPanTiltRadius(0.f,0.f,5.f);

	// Enter main loop
	glutMainLoop();

	// Delete shader program
	glDeleteProgram(shaderProgramID);

	return 0;
}

// Function to initialise OpenGL
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
	if(glewInit() != GLEW_OK)
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

	// Start timer function after 100 milliseconds
	glutTimerFunc(100, Timer, 0);

	glEnable(GL_DEPTH_TEST);
	return true;
}

// Updating tank properties
void updateTank(float timeStep)
{
	// Tank direction
	Vector3f TankDirection(sinf(degreesToRadians(tankAngle)), 0, cosf(degreesToRadians(tankAngle)));

	// Update tank velocity according to acceleration
	tankVelocity += (tankAcceleration - tankDeceleration) * timeStep;
	if(tankVelocity > 15) tankVelocity = 15;
	if(tankVelocity < -15) tankVelocity = -15;

	// Update tank angle
	tankAngle += tankAngleVelocity * (tankVelocity < 0 ? -1 : 1) * timeStep;

	// Update tank falling velocity
	if(gameOver)
	{
		const float gravity = -30;
		tankFallVelocity += gravity * timeStep;
	}

	// Update tank position according to movement and falling
	tankPosition = tankPosition + TankDirection * tankVelocity * timeStep;
	tankPosition.y = tankPosition.y + tankFallVelocity * timeStep;

	// Update distance travelled
	tankDistanceTravelled += tankVelocity * timeStep;

	// Convert tank position to maze coordinates
	int i = (int)floorf(tankPosition.z / cubeSize + 0.5f);
	int j = (int)floorf(tankPosition.x / cubeSize + 0.5f);

	// If tank position is not over block, it should fall
	if(!isBlock(i, j)) gameOver = true;
}

// Drawing mesh
void drawMesh(Mesh & mesh, Matrix4x4 & matrix, GLuint textureID)
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

	// Set texture and draw mesh
	glBindTexture(GL_TEXTURE_2D, textureID);
	mesh.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
}

// Drawing cubes
void drawCubes()
{
	// For each cube
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		if(isBlock(i, j))
		{
			// Cube position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, -cubeSize * 0.5f + 0.5f, cubeSize * i);
			m.scale(cubeSize * 0.5f, cubeSize * 0.5f, cubeSize * 0.5f);

			// Draw cube
			drawMesh(cube, m, cubeTextureID);
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
		if(isTarget(i, j))
		{
			// Coin position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, coinSize * 6 + 0.5f, cubeSize * i);
			m.scale(coinSize, coinSize, coinSize);

			// Draw coin
			drawMesh(coin, m, coinTextureID);
		}
	}
}

// Drawing ball
void drawBall()
{
	// Ball position and size
	Matrix4x4 m;
	m.translate(ballPosition.x, ballPosition.y, ballPosition.z);
	m.scale(ballSize, ballSize, ballSize);

	// Draw ball
	drawMesh(ball, m, ballTextureID);
}

// Drawing tank
void drawTank()
{
	// Draw chassis
	Matrix4x4 tankMatrix;
	tankMatrix.translate(tankPosition.x, tankPosition.y, tankPosition.z);
	tankMatrix.rotate(tankAngle, 0, 1, 0);
	drawMesh(chassis, tankMatrix, tankTextureID);

	// Wheel angle according to distance travelled
	const float wheelRadius = 0.5f;
	float wheelAngle = radiansToDegrees(tankDistanceTravelled / wheelRadius);

	// Draw front wheel
	Matrix4x4 frontWheelMatrix = tankMatrix;
	rotateAroundPoint(frontWheelMatrix, frontWheel.getMeshCentroid(), wheelAngle, tankAngleVelocity * 0.15f);
	drawMesh(frontWheel, frontWheelMatrix, tankTextureID);

	// Draw back wheel
	Matrix4x4 backWheelMatrix = tankMatrix;
	rotateAroundPoint(backWheelMatrix, backWheel.getMeshCentroid(), wheelAngle, 0);
	drawMesh(backWheel, backWheelMatrix, tankTextureID);

	// Draw turret
	Matrix4x4 turretMatrix = tankMatrix;
	float turretAngle = radiansToDegrees(cameraManip.getPan()) - tankAngle - 90;
	turretMatrix.rotate(turretAngle, 0, 1, 0);
	drawMesh(turret, turretMatrix, tankTextureID);
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
	Matrix4x4 ProjectionMatrix;
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

	// Draw all meshes
	drawCubes();
	drawCoins();
	drawTank();
	//drawBall();

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
	if(key == 27)	// esc key code
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
	// Tank acceleration and deceleration
	tankAcceleration = keyStates['w'] ? 50.0f : 0.0f;
	tankDeceleration = keyStates['s'] ? 50.0f : 0.0f;

	// Tank turning
	tankAngleVelocity = 0;
	if(keyStates['a']) tankAngleVelocity += 100.0f;
	if(keyStates['d']) tankAngleVelocity -= 100.0f;
}

// Mouse Interaction
void mouse(int button, int state, int x, int y)
{
}

// Motion
void motion(int x, int y)
{
	cameraManip.handleMouseMotion(x, y);
}

// Timer Function
void Timer(int value)
{
	float timeStep = 0.01f;
	updateTank(timeStep);

	// Call function again after 10 milliseconds
	glutTimerFunc(10, Timer, 0);
}
