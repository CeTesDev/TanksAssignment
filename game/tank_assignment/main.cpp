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
#include <sstream>

// Maze dimensions and integer matrix
const int N = 8;       // Rows
const int M = 10;      // Columns
std::vector<int> maze; // MxN matrix

// Camera properties
const float cameraHeight = 5;
const float cameraDistance = 7;

// Tank properties
const float tankAcceleration = 100;
const float tankDeceleration = 100;
const float tankMaxVelocity = 25;
const float tankFriction = 30;
const float tankTurningRate = 100;

// Cube properties
const float cubeSize = 15;

// Coin properties
const float coinSize = 1;
const float coinHeight = 2;

// Ball properties
const float ballSize = 0.4f;
const float ballSpeed = 50;

// Game properties
int screenWidth  = 630;
int screenHeight = 630;
const float gravity = -30;

// Light properties
Vector3f lightPosition = Vector3f(0, 1, 1);
Vector3f ambient       = Vector3f(0.1f, 0.1f, 0.2f);
Vector3f specular      = Vector3f(1.0f, 1.0f, 1.0f);
float specularPower    = 50.0f;

// Tank variables
Vector3f tankPosition;
float tankDistanceTravelled = 0;
float tankAngle = 0;
float tankVelocity = 0;
float tankFallVelocity = 0;
float tankFallingTime = 0;
bool tankAccelerating = false;
bool tankDecelerating = false;
bool tankTurningLeft = false;
bool tankTurningRight = false;
bool tankFalling = false;

// Coin variables
int totalCoins = 0;
int collectedCoins = 0;

// Ball variables
bool shooting = false;
Vector3f ballPosition, ballVelocity;

// Game variables
SphericalCameraManipulator cameraManip;
float timeRemaining = 0;
bool gameOver = false;
std::string gameOverMessage;

// Objects and texture IDs
Mesh cube, coin, ball, chassis, backWheel, frontWheel, turret;
GLuint cubeTextureID, tankTextureID, ballTextureID, coinTextureID;

// Shader variables
GLuint shaderProgramID;
GLuint MVMatrixUniformLocation;
GLuint ProjectionUniformLocation;
GLuint TextureMapUniformLocation;

// Vertex attribute locations
GLuint vertexPositionAttribute;
GLuint vertexNormalAttribute;
GLuint vertexTexcoordAttribute;

// Light uniform locations
GLuint LightPositionUniformLocation;
GLuint AmbientUniformLocation;
GLuint SpecularUniformLocation;
GLuint SpecularPowerUniformLocation;

// Array of key states
bool keyStates[256];

// Function Prototypes
bool initGL(int argc, char** argv);
void display(void);
void keyboard(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void handleKeys();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);

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
void rotateAroundPoint(Matrix4x4 & matrix, const Vector3f & point, float angle)
{
	// Move to point, rotate, and move back
	matrix.translate(point.x, point.y, point.z);
	matrix.rotate(angle, 1, 0, 0);
	matrix.translate(-point.x, -point.y, -point.z);
}

// Loading maze from file
bool loadMaze()
{
	maze.clear();
	totalCoins = 0;

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
		// Convert char to integer, count coins, add to matrix
		int type = c - '0';
		if(type == 2) totalCoins++;
		maze.push_back(type);
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

// Removing targets
void removeTarget(int i, int j)
{
	// Bound checking
	if(i < 0 || i > N - 1) return;
	if(j < 0 || j > M - 1) return;
	int index = i * M + j;
	if(index < 0 || index > (int)maze.size()) return;

	// 1 indicates blocks without target
	maze[index] = 1;
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

void restart()
{
	loadMaze();

	// Reset tank
	tankPosition = Vector3f(cubeSize * (M - 1), -0.5f, cubeSize * 0);
	tankDistanceTravelled = 0;
	tankAngle = -90;
	tankVelocity = 0;
	tankFallVelocity = 0;
	tankFallingTime = 0;
	tankAccelerating = false;
	tankDecelerating = false;
	tankTurningLeft = false;
	tankTurningRight = false;
	tankFalling = false;

	// Reset game
	timeRemaining = 60;
	collectedCoins = 0;
	shooting = false;
	gameOver = false;
	gameOverMessage.clear();
};

// Main Program Entry
int main(int argc, char** argv)
{
	// Init OpenGL
	if(!initGL(argc, argv))
		return -1;

	// Init Key States to false;
	for(int i = 0 ; i < 256; i++)
		keyStates[i] = false;

	// Load objects
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

	// Set up camera manipulator with initial turret centering
	cameraManip.setPanTiltRadius(0, 0, 10);
	cameraManip.handleMouseMotion(screenWidth / 2, screenHeight / 2);

	// Start game
	restart();

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

// Colliding coins
void collideCoins(Vector3f position)
{
	// For each grid cell
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		// Ignore cells without coins
		if(!isTarget(i, j)) continue;

		// Coin position
		Vector3f coinPosition(cubeSize * j, coinHeight, cubeSize * i);

		// If coin is close to position
		if((position - coinPosition).length() < 2)
		{
			// Remove target
			removeTarget(i, j);
			collectedCoins++;

			// Winning condition
			if(collectedCoins == totalCoins)
			{
				gameOver = true;
				gameOverMessage = "YOU WIN!";
			}
		}
	}
}

// Updating tank variables
void updateTank(float timeStep)
{
	// Tank direction
	Vector3f TankDirection(sinf(degreesToRadians(tankAngle)), 0, cosf(degreesToRadians(tankAngle)));

	// Total tank acceleration according to driving, braking, and friction
	float totalAcceleration = 0;
	if(tankAccelerating) totalAcceleration += tankAcceleration;
	if(tankDecelerating) totalAcceleration -= tankDeceleration;
	if(tankVelocity < 0) totalAcceleration += tankFriction;
	if(tankVelocity > 0) totalAcceleration -= tankFriction;

	// Update tank velocity according to total acceleration
	tankVelocity += totalAcceleration * timeStep;

	// Limit maximum and minimum velocity
	if(tankVelocity > tankMaxVelocity) tankVelocity = tankMaxVelocity;
	if(tankVelocity < -tankMaxVelocity) tankVelocity = -tankMaxVelocity;
	if(fabsf(tankVelocity) < 1) tankVelocity = 0;

	// Tank angular velocity according to left/right turning
	float tankOmega = 0;
	if(tankTurningLeft) tankOmega += tankTurningRate;
	if(tankTurningRight) tankOmega -= tankTurningRate;

	// Update tank angle according to forward/backward motion
	if(tankVelocity < 0) tankAngle -= tankOmega * timeStep;
	if(tankVelocity > 0) tankAngle += tankOmega * timeStep;

	// Update tank falling velocity
	if(tankFalling)
	{
		tankFallVelocity += gravity * timeStep;
		tankFallingTime += timeStep;

		// Losing condition
		if(tankFallingTime > 0.6f)
		{
			gameOver = true;
			gameOverMessage = "YOU LOSE!";
		}
	}

	// Update tank position according to movement and falling
	tankPosition = tankPosition + TankDirection * tankVelocity * timeStep;
	tankPosition.y = tankPosition.y + tankFallVelocity * timeStep;

	// Update distance travelled
	tankDistanceTravelled += tankVelocity * timeStep;

	// Collision detection between coins and tank top
	Vector3f tankTop = tankPosition;
	tankTop.y += turret.getMeshCentroid().y;
	collideCoins(tankTop);

	// Convert tank position to maze coordinates
	int i = (int)floorf(tankPosition.z / cubeSize + 0.5f);
	int j = (int)floorf(tankPosition.x / cubeSize + 0.5f);

	// If tank position is not over block, it should fall
	if(!isBlock(i, j)) tankFalling = true;
}

// Updating ball variables
void updateBall(float timeStep)
{
	if(!shooting) return;

	// Falling under gravity
	ballVelocity.y += gravity * timeStep;
	ballPosition = ballPosition + ballVelocity * timeStep;

	// End of falling
	if(ballPosition.y < 0) shooting = false;

	// Collision detection between coins and ball
	collideCoins(ballPosition);
}

// Shooting ball
void shootBall()
{
	// One ball at time
	if(shooting) return;

	// Initial ball velocity in turret direction
	float turretAngle = radiansToDegrees(cameraManip.getPan()) + 90;
	float ballAngle = tankAngle + turretAngle;
	Vector3f ballDirection(sinf(degreesToRadians(ballAngle)), 0, cosf(degreesToRadians(ballAngle)));
	ballVelocity = ballDirection * ballSpeed;

	// Initial ball position at turret muzzle
	ballPosition = tankPosition + ballDirection * 4;
	ballPosition.y += turret.getMeshCentroid().y;

	shooting = true;
}

// Drawing mesh
void drawMesh(Mesh & mesh, Matrix4x4 & matrix, GLuint textureID)
{
	// View matrix with camera following tank from fixed distance
	Matrix4x4 view;
	view.translate(0, -cameraHeight, -cameraDistance);
	view.rotate(180 - tankAngle, 0, 1, 0);
	view.translate(-tankPosition.x, -tankPosition.y, -tankPosition.z);

	// Modelview matrix
	Matrix4x4 modelview = view * matrix;

	// Set modelview matrix
	glUniformMatrix4fv(
		MVMatrixUniformLocation, // Uniform location
		1,                       // Number of uniforms
		false,                   // Transpose matrix
		modelview.getPtr());     // Pointer to matrix values

	// Set texture and draw mesh
	glBindTexture(GL_TEXTURE_2D, textureID);
	mesh.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
}

// Drawing cubes
void drawCubes()
{
	// For each grid cell
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		if(isBlock(i, j))
		{
			// Cube position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, -cubeSize * 0.5f, cubeSize * i);
			m.scale(cubeSize * 0.5f, cubeSize * 0.5f, cubeSize * 0.5f);

			// Draw cube
			drawMesh(cube, m, cubeTextureID);
		}
	}
}

// Drawing coins
void drawCoins()
{
	// For each grid cell
	for(int i = 0; i < N; i++)
	for(int j = 0; j < M; j++)
	{
		if(isTarget(i, j))
		{
			// Coin position and size
			Matrix4x4 m;
			m.translate(cubeSize * j, coinHeight, cubeSize * i);
			m.scale(coinSize, coinSize, coinSize);

			// Draw coin
			drawMesh(coin, m, coinTextureID);
		}
	}
}

// Drawing ball
void drawBall()
{
	if(!shooting) return;

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
	rotateAroundPoint(frontWheelMatrix, frontWheel.getMeshCentroid(), wheelAngle);
	drawMesh(frontWheel, frontWheelMatrix, tankTextureID);

	// Draw back wheel
	Matrix4x4 backWheelMatrix = tankMatrix;
	rotateAroundPoint(backWheelMatrix, backWheel.getMeshCentroid(), wheelAngle);
	drawMesh(backWheel, backWheelMatrix, tankTextureID);

	// Draw turret
	Matrix4x4 turretMatrix = tankMatrix;
	float turretAngle = radiansToDegrees(cameraManip.getPan()) + 90;
	turretMatrix.rotate(turretAngle, 0, 1, 0);
	drawMesh(turret, turretMatrix, tankTextureID);
}

void drawHUD(float x, float y, std::string text)
{
	// Position and size
	glLoadIdentity();
	glTranslatef(x, y, 0);
	glScalef(0.0008f, 0.001f, 1);
	glLineWidth(3);

	// Draw text
	int length = text.length();
	for(int i = 0; i < length; i++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
	}
}

// Display loop
void display(void)
{
	// Handle keys
	handleKeys();

	// Set viewport
	glViewport(0, 0, screenWidth, screenHeight);

	// Clear screen
	glClearColor(0.4f, 0.5f, 0.6f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Enable shaders
	glUseProgram(shaderProgramID);

	// Set perspective projection matrix
	Matrix4x4 ProjectionMatrix;
	float fieldOfView = 90;
	float aspectRatio = float(screenWidth) / screenHeight;
	ProjectionMatrix.perspective(fieldOfView, aspectRatio, 0.1f, 1000);
	glUniformMatrix4fv(
		ProjectionUniformLocation,  // Uniform location
		1,                          // Number of uniforms
		false,                      // Transpose matrix
		ProjectionMatrix.getPtr()); // Pointer to matrix values

	// Enable texture mapping
	glUniform1i(TextureMapUniformLocation, 0);

	// Set non-shiny light for cubes
	glUniform3f(LightPositionUniformLocation, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform4f(AmbientUniformLocation, ambient.x, ambient.y, ambient.z, 1);
	glUniform4f(SpecularUniformLocation, 0, 0, 0, 1);
	glUniform1f(SpecularPowerUniformLocation, specularPower);

	// Draw non-shiny objects
	drawCubes();

	// Set shiny light for other objects
	glUniform4f(SpecularUniformLocation, specular.x, specular.y, specular.z, 1);

	// Draw shiny objects
	drawCoins();
	drawTank();
	drawBall();

	// Disable shaders
	glUseProgram(0);

	// Show time and score
	std::stringstream stream1, stream2;
	stream1.precision(2);
	stream1 << std::fixed << "Time: " << timeRemaining;
	stream2 << "Score: " << collectedCoins << "/" << totalCoins;
	drawHUD(-0.8f, 0.8f, stream1.str());
	drawHUD(+0.1f, 0.8f, stream2.str());

	// Show win/lose message
	if(gameOver)
	{
		drawHUD(-0.20f, 0.5f, gameOverMessage);
		drawHUD(-0.55f, 0.3f, "press Space to continue");
	}

	// Swap buffers and post redisplay
	glutSwapBuffers();
	glutPostRedisplay();
}

// Keyboard Interaction
void keyboard(unsigned char key, int x, int y)
{
	// Quits program when esc is pressed
	if(key == 27) exit(0);

	// Restart game
	if(key == ' ') restart();

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
	tankAccelerating = keyStates['w'];
	tankDecelerating = keyStates['s'];

	// Tank turning
	tankTurningLeft = keyStates['a'];
	tankTurningRight = keyStates['d'];
}

// Mouse interaction
void mouse(int button, int state, int x, int y)
{
	// Shoot ball on left mouse button
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !gameOver) shootBall();
}

// Motion
void motion(int x, int y)
{
	if(!gameOver) cameraManip.handleMouseMotion(x, y);
}

// Timer Function
void Timer(int value)
{
	// Update tank and ball
	float timeStep = 0.01f;
	if(!gameOver) updateTank(timeStep);
	updateBall(timeStep);

	if(!gameOver)
	{
		// Update timer
		timeRemaining -= timeStep;

		// Losing condition
		if(timeRemaining < 0)
		{
			timeRemaining = 0;
			gameOver = true;
			gameOverMessage = "YOU LOSE!";
		}
	}

	// Call function again after 10 milliseconds
	glutTimerFunc(10, Timer, 0);
}
