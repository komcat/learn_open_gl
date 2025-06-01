// learn_open_gl.cpp : Defines the entry point for the application.
//
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "learn_open_gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cmath>
#include <vector>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Projectile structure
struct Projectile {
  float x, y;          // Position
  float velX, velY;    // Velocity
  float life;          // Time alive (for cleanup and animation)
  int frame;           // Current animation frame (0-3)

  Projectile(float startX, float startY, float vx, float vy)
    : x(startX), y(startY), velX(vx), velY(vy), life(0.0f), frame(0) {
  }
};

// Global variables
unsigned int llamaShaderProgram, ballShaderProgram;
unsigned int llamaVAO, llamaVBO, llamaEBO;
unsigned int ballVAO, ballVBO, ballEBO;
unsigned int llamaTexture, projectileTexture;
double mouseX = 0.0, mouseY = 0.0;
int windowWidth = 800, windowHeight = 600;
std::vector<Projectile> projectiles;
auto lastShotTime = std::chrono::steady_clock::now();

// Vertex data for llama quad
float llamaVertices[] = {
  // positions        // texture coords
   0.3f,  0.3f, 0.0f,  0.5f, 1.0f,     // top right 
   0.3f, -0.3f, 0.0f,  0.5f, 0.667f,   // bottom right
  -0.3f, -0.3f, 0.0f,  0.0f, 0.667f,   // bottom left
  -0.3f,  0.3f, 0.0f,  0.0f, 1.0f      // top left 
};

// Vertex data for projectile ball (with texture coordinates)
float ballVertices[] = {
  // positions        // texture coords (will be updated per frame)
   0.08f,  0.08f, 0.0f,  0.5f, 1.0f,     // top right 
   0.08f, -0.08f, 0.0f,  0.5f, 0.5f,     // bottom right
  -0.08f, -0.08f, 0.0f,  0.0f, 0.5f,     // bottom left
  -0.08f,  0.08f, 0.0f,  0.0f, 1.0f      // top left 
};

unsigned int indices[] = {
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};

// Vertex shader for textured llama
const char* llamaVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader for textured llama
const char* llamaFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

// Vertex shader for textured projectiles
const char* ballVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader for textured projectiles
const char* ballFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D projectileTexture;

void main()
{
    FragColor = texture(projectileTexture, TexCoord);
}
)";

// Function to compile shader
unsigned int compileShader(unsigned int type, const char* source)
{
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  return shader;
}

// Function to create shader program
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
  unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
  unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

  unsigned int program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  int success;
  char infoLog[512];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

// Function to load texture
unsigned int loadTexture(const char* path)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

  if (data)
  {
    GLenum format;
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }
  else
  {
    std::cout << "Failed to load texture: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}

// Create transformation matrix
void createTransformMatrix(float matrix[16], float angle, float translateX = 0.0f, float translateY = 0.0f)
{
  float cosA = cos(angle);
  float sinA = sin(angle);

  // Initialize to identity matrix
  for (int i = 0; i < 16; i++)
    matrix[i] = 0.0f;

  // Set rotation + translation matrix values
  matrix[0] = cosA;   matrix[1] = sinA;   matrix[2] = 0.0f;  matrix[3] = 0.0f;
  matrix[4] = -sinA;  matrix[5] = cosA;   matrix[6] = 0.0f;  matrix[7] = 0.0f;
  matrix[8] = 0.0f;   matrix[9] = 0.0f;   matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = translateX; matrix[13] = translateY; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

// Update projectiles and animation
void updateProjectiles(float deltaTime)
{
  for (auto& proj : projectiles)
  {
    proj.x += proj.velX * deltaTime;
    proj.y += proj.velY * deltaTime;
    proj.life += deltaTime;

    // Update animation frame (change frame every 0.1 seconds)
    proj.frame = (int)(proj.life * 10.0f) % 4; // 4 frames, cycle every 0.4 seconds
  }

  // Remove projectiles that are off-screen or too old
  projectiles.erase(
    std::remove_if(projectiles.begin(), projectiles.end(),
      [](const Projectile& p) {
    return p.x < -2.0f || p.x > 2.0f ||
      p.y < -2.0f || p.y > 2.0f ||
      p.life > 5.0f; // Remove after 5 seconds
  }),
    projectiles.end()
  );
}

// Get texture coordinates for projectile frame
void getProjectileFrameCoords(int frame, float coords[8])
{
  // Sprite sheet: 64x64, 2x2 grid, each frame 32x32
  // Frame layout:
  // 0 | 1
  // -----
  // 2 | 3

  float frameWidth = 0.5f;  // 32/64 = 0.5
  float frameHeight = 0.5f; // 32/64 = 0.5

  int col = frame % 2;       // 0 or 1
  int row = frame / 2;       // 0 or 1

  float left = col * frameWidth;
  float right = left + frameWidth;
  float top = 1.0f - (row * frameHeight);      // Flip Y for OpenGL
  float bottom = top - frameHeight;

  // Texture coordinates for quad (top-right, bottom-right, bottom-left, top-left)
  coords[0] = right; coords[1] = top;      // top right
  coords[2] = right; coords[3] = bottom;   // bottom right  
  coords[4] = left;  coords[5] = bottom;   // bottom left
  coords[6] = left;  coords[7] = top;      // top left
}

// Shoot projectile
void shootProjectile(float angle)
{
  float speed = 1.5f; // Projectile speed
  float startX = 0.0f; // Start from llama center
  float startY = 0.0f;

  // Calculate velocity based on angle - remove the +PI/2 offset
  float velX = cos(angle) * speed;
  float velY = sin(angle) * speed;

  projectiles.emplace_back(startX, startY, velX, velY);
}

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
  mouseX = xpos;
  mouseY = ypos;
}

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

// Process input
void processInput(GLFWwindow* window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

int main()
{
  // Initialize GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "LearnOpenGL - Shooting Llama", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Set callbacks
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Enable blending for transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Create shader programs
  llamaShaderProgram = createShaderProgram(llamaVertexShaderSource, llamaFragmentShaderSource);
  ballShaderProgram = createShaderProgram(ballVertexShaderSource, ballFragmentShaderSource);

  // Set up llama vertex data
  glGenVertexArrays(1, &llamaVAO);
  glGenBuffers(1, &llamaVBO);
  glGenBuffers(1, &llamaEBO);

  glBindVertexArray(llamaVAO);
  glBindBuffer(GL_ARRAY_BUFFER, llamaVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(llamaVertices), llamaVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, llamaEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Set up ball vertex data
  glGenVertexArrays(1, &ballVAO);
  glGenBuffers(1, &ballVBO);
  glGenBuffers(1, &ballEBO);

  glBindVertexArray(ballVAO);
  glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertices), ballVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ballEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Load llama texture
  llamaTexture = loadTexture("assets/llama.png");
  if (llamaTexture == 0)
  {
    llamaTexture = loadTexture("llama.png");
  }
  if (llamaTexture == 0)
  {
    std::cout << "Failed to load llama texture!" << std::endl;
    return -1;
  }

  // Load projectile texture
  projectileTexture = loadTexture("assets/default_projectile.png");
  if (projectileTexture == 0)
  {
    projectileTexture = loadTexture("default_projectile.png");
  }
  if (projectileTexture == 0)
  {
    std::cout << "Failed to load projectile texture!" << std::endl;
    return -1;
  }

  glViewport(0, 0, windowWidth, windowHeight);

  // Timing variables
  auto currentTime = std::chrono::steady_clock::now();
  auto lastTime = currentTime;

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
    // Calculate delta time
    currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    processInput(window);

    // Calculate llama rotation
    float centerX = windowWidth / 2.0f;
    float centerY = windowHeight / 2.0f;
    float deltaX = (float)mouseX - centerX;
    float deltaY = centerY - (float)mouseY;
    float llamaAngle = -(atan2(deltaX, deltaY) - M_PI / 2.0f);

    // Shoot projectiles every 200ms
    auto timeSinceLastShot = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastShotTime);
    if (timeSinceLastShot.count() >= 200)
    {
      shootProjectile(llamaAngle);
      lastShotTime = currentTime;
    }

    // Update projectiles
    updateProjectiles(deltaTime);

    // Clear screen
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render llama
    glUseProgram(llamaShaderProgram);
    float llamaTransform[16];
    createTransformMatrix(llamaTransform, llamaAngle);
    unsigned int llamaTransformLoc = glGetUniformLocation(llamaShaderProgram, "transform");
    glUniformMatrix4fv(llamaTransformLoc, 1, GL_FALSE, llamaTransform);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, llamaTexture);
    glUniform1i(glGetUniformLocation(llamaShaderProgram, "ourTexture"), 0);

    glBindVertexArray(llamaVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Render projectiles with animation
    glUseProgram(ballShaderProgram);
    glBindVertexArray(ballVAO);

    // Bind projectile texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, projectileTexture);
    glUniform1i(glGetUniformLocation(ballShaderProgram, "projectileTexture"), 0);

    for (const auto& proj : projectiles)
    {
      // Get texture coordinates for current frame
      float frameCoords[8];
      getProjectileFrameCoords(proj.frame, frameCoords);

      // Update vertex buffer with new texture coordinates
      float tempVertices[] = {
        // positions        // texture coords (updated per frame)
         0.08f,  0.08f, 0.0f,  frameCoords[0], frameCoords[1],  // top right
         0.08f, -0.08f, 0.0f,  frameCoords[2], frameCoords[3],  // bottom right
        -0.08f, -0.08f, 0.0f,  frameCoords[4], frameCoords[5],  // bottom left
        -0.08f,  0.08f, 0.0f,  frameCoords[6], frameCoords[7]   // top left
      };

      glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tempVertices), tempVertices);

      // Set transformation matrix
      float ballTransform[16];
      createTransformMatrix(ballTransform, 0.0f, proj.x, proj.y);
      unsigned int ballTransformLoc = glGetUniformLocation(ballShaderProgram, "transform");
      glUniformMatrix4fv(ballTransformLoc, 1, GL_FALSE, ballTransform);

      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &llamaVAO);
  glDeleteBuffers(1, &llamaVBO);
  glDeleteBuffers(1, &llamaEBO);
  glDeleteVertexArrays(1, &ballVAO);
  glDeleteBuffers(1, &ballVBO);
  glDeleteBuffers(1, &ballEBO);
  glDeleteProgram(llamaShaderProgram);
  glDeleteProgram(ballShaderProgram);
  glDeleteTextures(1, &llamaTexture);
  glDeleteTextures(1, &projectileTexture);

  glfwTerminate();
  return 0;
}