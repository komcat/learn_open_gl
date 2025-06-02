#include "projectile.h"
#include "shader.h"
#include "texture_loader.h"
#include "enemy.h"
#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Vertex shader source
const char* ProjectileManager::vertexShaderSource = R"(
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

// Fragment shader source
const char* ProjectileManager::fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D projectileTexture;

void main()
{
    FragColor = texture(projectileTexture, TexCoord);
}
)";

Projectile::Projectile(float startX, float startY, float vx, float vy)
  : x(startX), y(startY), velX(vx), velY(vy), life(0.0f), frame(0)
{
}

ProjectileManager::ProjectileManager() : VAO(0), VBO(0), EBO(0), texture(0),
gen(rd()), dis(-1.0f, 1.0f), lastShotTime(std::chrono::steady_clock::now())
{
}

ProjectileManager::~ProjectileManager()
{
  if (VAO) glDeleteVertexArrays(1, &VAO);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (EBO) glDeleteBuffers(1, &EBO);
  if (texture) glDeleteTextures(1, &texture);
}

bool ProjectileManager::initialize(const char* texturePath)
{
  setupMesh();

  // Load texture
  texture = loadTexture(texturePath);
  if (texture == 0)
  {
    std::cout << "Failed to load projectile texture: " << texturePath << std::endl;
    return false;
  }

  return true;
}

void ProjectileManager::setupMesh()
{
  // Vertex data for projectile ball
  float ballVertices[] = {
    // positions        // texture coords
     0.08f,  0.08f, 0.0f,  0.5f, 1.0f,     // top right 
     0.08f, -0.08f, 0.0f,  0.5f, 0.5f,     // bottom right
    -0.08f, -0.08f, 0.0f,  0.0f, 0.5f,     // bottom left
    -0.08f,  0.08f, 0.0f,  0.0f, 1.0f      // top left 
  };

  unsigned int indices[] = {
      0, 1, 3,   // first triangle
      1, 2, 3    // second triangle
  };

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertices), ballVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

unsigned int ProjectileManager::loadTexture(const char* path)
{
  return TextureLoader::loadTexture(path);
}

void ProjectileManager::addProjectile(float startX, float startY, float angle, float speed)
{
  // Use default 1% spray
  addProjectileWithSpray(startX, startY, angle, speed, 1.0f);
}

void ProjectileManager::addProjectileWithSpray(float startX, float startY, float angle, float speed, float sprayPercent)
{
  // Calculate spray angle in radians
  // 1% spray = 1% of a full circle = 0.01 * 2π radians = ~0.0628 radians (~3.6 degrees)
  float maxSprayRadians = (sprayPercent / 100.0f) * 2.0f * M_PI;

  // Generate random spray offset
  float sprayOffset = dis(gen) * maxSprayRadians;
  float finalAngle = angle + sprayOffset;

  // Calculate velocity with spray applied
  float velX = cos(finalAngle) * speed;
  float velY = sin(finalAngle) * speed;

  projectiles.emplace_back(startX, startY, velX, velY);
}

bool ProjectileManager::canShoot(float baseIntervalMs, float timingErrorPercent)
{
  auto currentTime = std::chrono::steady_clock::now();
  auto timeSinceLastShot = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastShotTime);

  // Calculate timing error
  // 2% error means interval can vary by ±2% (e.g., 200ms ±4ms = 196-204ms range)
  float errorRange = (timingErrorPercent / 100.0f) * baseIntervalMs;
  float timingError = dis(gen) * errorRange;
  float adjustedInterval = baseIntervalMs + timingError;

  return timeSinceLastShot.count() >= adjustedInterval;
}

void ProjectileManager::updateLastShotTime()
{
  lastShotTime = std::chrono::steady_clock::now();
}

void ProjectileManager::update(float deltaTime, EnemyManager* enemyManager)
{
  for (auto it = projectiles.begin(); it != projectiles.end();)
  {
    auto& proj = *it;

    proj.x += proj.velX * deltaTime;
    proj.y += proj.velY * deltaTime;
    proj.life += deltaTime;

    // Update animation frame (change frame every 0.1 seconds)
    proj.frame = (int)(proj.life * 10.0f) % 4; // 4 frames, cycle every 0.4 seconds

    // Check collision with enemies if enemy manager is provided
    bool hitEnemy = false;
    if (enemyManager)
    {
      hitEnemy = enemyManager->checkProjectileCollisions(proj.x, proj.y, 0.08f);
    }

    // Remove projectiles that hit enemies, are off-screen, or too old
    if (hitEnemy ||
      proj.x < -5.0f || proj.x > 5.0f ||
      proj.y < -5.0f || proj.y > 5.0f ||
      proj.life > 5.0f)
    {
      it = projectiles.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void ProjectileManager::getFrameCoords(int frame, float coords[8])
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

void ProjectileManager::render(std::shared_ptr<Shader> shader)
{
  shader->use();
  glBindVertexArray(VAO);

  // Bind projectile texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  shader->setInt("projectileTexture", 0);

  for (const auto& proj : projectiles)
  {
    // Get texture coordinates for current frame
    float frameCoords[8];
    getFrameCoords(proj.frame, frameCoords);

    // Update vertex buffer with new texture coordinates
    float tempVertices[] = {
      // positions        // texture coords (updated per frame)
       0.08f,  0.08f, 0.0f,  frameCoords[0], frameCoords[1],  // top right
       0.08f, -0.08f, 0.0f,  frameCoords[2], frameCoords[3],  // bottom right
      -0.08f, -0.08f, 0.0f,  frameCoords[4], frameCoords[5],  // bottom left
      -0.08f,  0.08f, 0.0f,  frameCoords[6], frameCoords[7]   // top left
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tempVertices), tempVertices);

    // Create transformation matrix
    float transform[16];
    float cosA = cos(0.0f);
    float sinA = sin(0.0f);

    // Initialize to identity matrix
    for (int i = 0; i < 16; i++)
      transform[i] = 0.0f;

    // Set rotation + translation matrix values
    transform[0] = cosA;   transform[1] = sinA;   transform[2] = 0.0f;  transform[3] = 0.0f;
    transform[4] = -sinA;  transform[5] = cosA;   transform[6] = 0.0f;  transform[7] = 0.0f;
    transform[8] = 0.0f;   transform[9] = 0.0f;   transform[10] = 1.0f; transform[11] = 0.0f;
    transform[12] = proj.x; transform[13] = proj.y; transform[14] = 0.0f; transform[15] = 1.0f;

    shader->setMatrix4fv("transform", transform);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }
}

void ProjectileManager::clear()
{
  projectiles.clear();
}