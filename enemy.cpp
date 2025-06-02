#include "enemy.h"
#include "shader.h"
#include "texture_loader.h"
#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Enemy::Enemy(float startX, float startY, float vx, float vy)
  : x(startX), y(startY), velX(vx), velY(vy), life(0.0f), frame(0),
  hitPoints(3), size(1.0f), isAlive(true), spawnEffect(0.0f)
{
}

bool Enemy::takeDamage(int damage)
{
  hitPoints -= damage;
  if (hitPoints <= 0)
  {
    isAlive = false;
    return true; // Enemy died
  }
  return false; // Enemy still alive
}

bool Enemy::containsPoint(float pointX, float pointY) const
{
  if (!isAlive) return false;

  // Simple box collision (enemy size is roughly 0.3 * size)
  float halfSize = 0.15f * size;
  return (pointX >= x - halfSize && pointX <= x + halfSize &&
    pointY >= y - halfSize && pointY <= y + halfSize);
}

EnemyManager::EnemyManager()
  : maxEnemies(20), spawnRate(0.5f), spawnTimer(0.0f), VAO(0), VBO(0), EBO(0), texture(0),
  gen(rd()), posDis(-2.0f, 2.0f), speedDis(0.1f, 0.3f) // Spawn within visible range
{
}

EnemyManager::~EnemyManager()
{
  if (VAO) glDeleteVertexArrays(1, &VAO);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (EBO) glDeleteBuffers(1, &EBO);
  if (texture) glDeleteTextures(1, &texture);
}

bool EnemyManager::initialize(const char* texturePath)
{
  setupMesh();

  // Load texture
  texture = loadTexture(texturePath);
  if (texture == 0)
  {
    std::cout << "Failed to load enemy texture: " << texturePath << std::endl;
    return false;
  }

  return true;
}

void EnemyManager::setupMesh()
{
  // Vertex data for enemy quad
  float enemyVertices[] = {
    // positions        // texture coords (will be updated dynamically)
     0.15f,  0.15f, 0.0f,  0.2f, 1.0f,     // top right 
     0.15f, -0.15f, 0.0f,  0.2f, 0.8f,     // bottom right
    -0.15f, -0.15f, 0.0f,  0.0f, 0.8f,     // bottom left
    -0.15f,  0.15f, 0.0f,  0.0f, 1.0f      // top left 
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(enemyVertices), enemyVertices, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

unsigned int EnemyManager::loadTexture(const char* path)
{
  return TextureLoader::loadTexture(path);
}

void EnemyManager::update(float deltaTime)
{
  // Try to spawn new enemies
  trySpawnEnemy(deltaTime);

  // Update all enemies
  for (auto& enemy : enemies)
  {
    if (!enemy.isAlive) continue;

    // Update position
    enemy.x += enemy.velX * deltaTime;
    enemy.y += enemy.velY * deltaTime;
    enemy.life += deltaTime;
    enemy.spawnEffect += deltaTime; // Update spawn effect timer

    // Update animation frame (change frame based on time)
    enemy.frame = (int)(enemy.life * 8.0f) % 24; // 8 fps, 24 frames

    // Wrap around screen edges (larger boundaries)
    if (enemy.x > 5.0f) enemy.x = -5.0f;
    if (enemy.x < -5.0f) enemy.x = 5.0f;
    if (enemy.y > 5.0f) enemy.y = -5.0f;
    if (enemy.y < -5.0f) enemy.y = 5.0f;
  }

  // Remove dead enemies periodically
  removeDeadEnemies();
}

void EnemyManager::trySpawnEnemy(float deltaTime)
{
  spawnTimer += deltaTime;

  // Check if we should spawn a new enemy
  float spawnInterval = 1.0f / spawnRate; // Convert rate to interval
  if (spawnTimer >= spawnInterval && getAliveEnemyCount() < maxEnemies)
  {
    spawnEnemyAtRandomLocation();
    spawnTimer = 0.0f;
  }
}

void EnemyManager::spawnEnemyAtRandomLocation()
{
  float x, y;
  getRandomSpawnPosition(x, y);

  // Give enemy more varied movement patterns
  float speed = speedDis(gen);

  // 50% chance to move toward center, 50% chance for random movement
  float velX, velY;
  if (gen() % 2 == 0)
  {
    // Move toward center with some randomness
    float angleToCenter = atan2(-y, -x) + (posDis(gen) * 0.3f);
    velX = cos(angleToCenter) * speed;
    velY = sin(angleToCenter) * speed;
  }
  else
  {
    // Random movement direction
    float randomAngle = posDis(gen) * M_PI; // Random angle
    velX = cos(randomAngle) * speed;
    velY = sin(randomAngle) * speed;
  }

  enemies.emplace_back(x, y, velX, velY);
}

void EnemyManager::getRandomSpawnPosition(float& x, float& y)
{
  // Spawn enemies within visible range but not too close to center (player area)
  // With 2.5x zoom, visible area is roughly -2.5 to +2.5

  // Create a safe spawn zone: visible but not in center
  std::uniform_real_distribution<float> spawnDis(-2.2f, 2.2f);

  // Try to avoid spawning too close to center (player is at 0,0)
  do {
    x = spawnDis(gen);
    y = spawnDis(gen);
  } while (abs(x) < 0.8f && abs(y) < 0.8f); // Avoid center area where player is

  // Give enemy random movement direction (not necessarily toward center)
  // This makes spawning more dynamic and less predictable
}

bool EnemyManager::checkProjectileCollisions(float projX, float projY, float projRadius)
{
  for (auto& enemy : enemies)
  {
    if (enemy.containsPoint(projX, projY))
    {
      if (enemy.takeDamage(1))
      {
        // Enemy died
        std::cout << "Enemy destroyed!" << std::endl;
      }
      else
      {
        std::cout << "Enemy hit! HP remaining: " << enemy.hitPoints << std::endl;
      }
      return true; // Hit detected
    }
  }
  return false; // No hit
}

size_t EnemyManager::getAliveEnemyCount() const
{
  return std::count_if(enemies.begin(), enemies.end(),
    [](const Enemy& e) { return e.isAlive; });
}

void EnemyManager::removeDeadEnemies()
{
  enemies.erase(
    std::remove_if(enemies.begin(), enemies.end(),
      [](const Enemy& e) { return !e.isAlive; }),
    enemies.end()
  );
}

void EnemyManager::getFrameCoords(int frame, float coords[8])
{
  // Sprite sheet: 120x120, 5x5 grid (24 frames total), each frame 24x24
  // Same layout as the player dinosaur

  float frameWidth = 24.0f / 120.0f;   // 24/120 = 0.2
  float frameHeight = 24.0f / 120.0f;  // 24/120 = 0.2

  int col = frame % 5;       // 0, 1, 2, 3, or 4
  int row = frame / 5;       // 0, 1, 2, 3, or 4

  float left = col * frameWidth;
  float right = left + frameWidth;
  float top = 1.0f - (row * frameHeight);      // Flip Y for OpenGL
  float bottom = top - frameHeight;

  // Texture coordinates for quad
  coords[0] = right; coords[1] = top;      // top right
  coords[2] = right; coords[3] = bottom;   // bottom right  
  coords[4] = left;  coords[5] = bottom;   // bottom left
  coords[6] = left;  coords[7] = top;      // top left
}

void EnemyManager::render(std::shared_ptr<Shader> shader)
{
  shader->use();
  glBindVertexArray(VAO);

  // Bind enemy texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  shader->setInt("ourTexture", 0);

  for (const auto& enemy : enemies)
  {
    if (!enemy.isAlive) continue;

    // Get texture coordinates for current frame
    float frameCoords[8];
    getFrameCoords(enemy.frame, frameCoords);

    // Calculate size based on health and spawn effect
    float healthScale = 0.8f + (enemy.hitPoints / 3.0f) * 0.2f; // 0.8-1.0 scale

    // Add spawn effect - enemies grow from small to normal size over 0.5 seconds
    float spawnScale = 1.0f;
    if (enemy.spawnEffect < 0.5f)
    {
      spawnScale = enemy.spawnEffect / 0.5f; // 0.0 to 1.0 over 0.5 seconds
    }

    float size = 0.15f * enemy.size * healthScale * spawnScale;

    // Update vertex buffer with new texture coordinates and size
    float enemyVertices[] = {
      // positions                          // texture coords
       size,  size, 0.0f,  frameCoords[0], frameCoords[1],  // top right
       size, -size, 0.0f,  frameCoords[2], frameCoords[3],  // bottom right
      -size, -size, 0.0f,  frameCoords[4], frameCoords[5],  // bottom left
      -size,  size, 0.0f,  frameCoords[6], frameCoords[7]   // top left
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(enemyVertices), enemyVertices);

    // Create transformation matrix for position
    float transform[16];
    for (int i = 0; i < 16; i++)
      transform[i] = 0.0f;

    // Identity rotation with translation
    transform[0] = 1.0f;  transform[5] = 1.0f;  transform[10] = 1.0f; transform[15] = 1.0f;
    transform[12] = enemy.x; transform[13] = enemy.y;

    shader->setMatrix4fv("transform", transform);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }
}

void EnemyManager::clear()
{
  enemies.clear();
  spawnTimer = 0.0f;
}