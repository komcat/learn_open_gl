// learn_open_gl.cpp : Defines the entry point for the application.
//
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "learn_open_gl.h"
#include "shader.h"
#include "llama.h"
#include "projectile.h"
#include "enemy.h"
#include "camera.h"

#include <cmath>
#include <chrono>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Global variables
double mouseX = 0.0, mouseY = 0.0;
int windowWidth = 800, windowHeight = 600;

// Game objects
std::unique_ptr<Llama> llama;
std::unique_ptr<ProjectileManager> projectileManager;
std::unique_ptr<EnemyManager> enemyManager;
std::unique_ptr<Camera> camera;
std::shared_ptr<Shader> llamaShader;
std::shared_ptr<Shader> projectileShader;
std::shared_ptr<Shader> enemyShader;

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

// Calculate angle from llama to mouse cursor
float calculateLlamaAngle()
{
  float worldX, worldY;
  camera->screenToWorld((float)mouseX, (float)mouseY, windowWidth, windowHeight, worldX, worldY);
  return -(atan2(worldX, worldY) - M_PI / 2.0f);
}

// Initialize game objects
bool initializeGame()
{
  // Create shader sources
  const char* llamaVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;
uniform mat4 view;

void main()
{
    gl_Position = view * transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

  const char* llamaFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

  const char* projectileVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;
uniform mat4 view;

void main()
{
    gl_Position = view * transform * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

  const char* projectileFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D projectileTexture;

void main()
{
    FragColor = texture(projectileTexture, TexCoord);
}
)";

  // Create shaders
  llamaShader = std::make_shared<Shader>(llamaVertexShader, llamaFragmentShader);
  projectileShader = std::make_shared<Shader>(projectileVertexShader, projectileFragmentShader);
  enemyShader = std::make_shared<Shader>(llamaVertexShader, llamaFragmentShader); // Reuse same shader

  // Create game objects
  llama = std::make_unique<Llama>();
  projectileManager = std::make_unique<ProjectileManager>();
  enemyManager = std::make_unique<EnemyManager>();
  camera = std::make_unique<Camera>();

  // Initialize llama
  if (!llama->initialize("assets/llama.png"))
  {
    // Try alternative path
    if (!llama->initialize("llama.png"))
    {
      std::cout << "Failed to initialize llama!" << std::endl;
      return false;
    }
  }

  // Initialize projectile manager
  if (!projectileManager->initialize("assets/default_projectile.png"))
  {
    // Try alternative path
    if (!projectileManager->initialize("default_projectile.png"))
    {
      std::cout << "Failed to initialize projectile manager!" << std::endl;
      return false;
    }
  }

  // Initialize enemy manager
  if (!enemyManager->initialize("assets/DinoSprites_tard.png"))
  {
    // Try alternative path
    if (!enemyManager->initialize("DinoSprites_tard.png"))
    {
      std::cout << "Failed to initialize enemy manager!" << std::endl;
      return false;
    }
  }

  // Configure enemy spawning (expand spawn area for larger view)
  enemyManager->setMaxEnemies(5000);
  enemyManager->setSpawnRate(5.0f); // 0.5 enemies per second

  // Set camera zoom for better field of view
  camera->setZoom(2.5f); // 2.5x zoom out to see more area

  return true;
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

  // Initialize game objects
  if (!initializeGame())
  {
    std::cout << "Failed to initialize game!" << std::endl;
    glfwTerminate();
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

    // Calculate llama rotation and update
    float llamaAngle = calculateLlamaAngle();
    llama->setRotation(llamaAngle);
    llama->update(deltaTime); // Add animation update

    // Shoot projectiles with timing error and spray
    if (projectileManager->canShoot(200.0f, 2.0f)) // 200ms base interval, 2% timing error
    {
      projectileManager->addProjectile(llama->getX(), llama->getY(), llamaAngle); // Uses 1% spray by default
      projectileManager->updateLastShotTime();
    }

    // Update projectiles with enemy collision detection
    projectileManager->update(deltaTime, enemyManager.get());

    // Update enemies
    enemyManager->update(deltaTime);

    // Clear screen
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Create and set view matrix for all shaders
    float viewMatrix[16];
    camera->createViewMatrix(viewMatrix);

    // Set view matrix for all shaders
    llamaShader->use();
    llamaShader->setViewMatrix(viewMatrix);

    projectileShader->use();
    projectileShader->setViewMatrix(viewMatrix);

    enemyShader->use();
    enemyShader->setViewMatrix(viewMatrix);

    // Render llama
    llama->render(llamaShader);

    // Render projectiles
    projectileManager->render(projectileShader);

    // Render enemies
    enemyManager->render(enemyShader);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup is handled by destructors
  glfwTerminate();
  return 0;
}