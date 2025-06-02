#pragma once

#include <vector>
#include <memory>
#include <random>

class Shader;

struct Enemy {
  float x, y;              // Position
  float velX, velY;        // Velocity
  float life;              // Time alive
  int frame;               // Current animation frame (0-23)
  int hitPoints;           // Health (starts at 3)
  float size;              // Size multiplier
  bool isAlive;            // Is enemy still alive
  float spawnEffect;       // Spawn animation timer (0.0 = just spawned)

  Enemy(float startX, float startY, float vx = 0.0f, float vy = 0.0f);

  // Take damage and return true if enemy dies
  bool takeDamage(int damage = 1);

  // Check if point is inside enemy (for collision detection)
  bool containsPoint(float pointX, float pointY) const;
};

class EnemyManager
{
public:
  EnemyManager();
  ~EnemyManager();

  // Initialize OpenGL resources
  bool initialize(const char* texturePath);

  // Update all enemies
  void update(float deltaTime);

  // Render all enemies
  void render(std::shared_ptr<Shader> shader);

  // Spawn management
  void trySpawnEnemy(float deltaTime);
  void spawnEnemyAtRandomLocation();

  // Collision detection with projectiles
  bool checkProjectileCollisions(float projX, float projY, float projRadius = 0.08f);

  // Get enemy count
  size_t getEnemyCount() const { return enemies.size(); }
  size_t getAliveEnemyCount() const;

  // Clear all enemies
  void clear();

  // Configuration
  void setMaxEnemies(int max) { maxEnemies = max; }
  void setSpawnRate(float rate) { spawnRate = rate; } // enemies per second

private:
  std::vector<Enemy> enemies;

  // Spawn settings
  int maxEnemies;
  float spawnRate;           // enemies per second
  float spawnTimer;

  // Random number generation
  mutable std::random_device rd;
  mutable std::mt19937 gen;
  mutable std::uniform_real_distribution<float> posDis;      // For spawn positions
  mutable std::uniform_real_distribution<float> speedDis;    // For movement speed

  // OpenGL resources
  unsigned int VAO, VBO, EBO;
  unsigned int texture;

  // Helper functions
  void setupMesh();
  unsigned int loadTexture(const char* path);
  void getFrameCoords(int frame, float coords[8]);
  void removeDeadEnemies();

  // Spawn position calculation
  void getRandomSpawnPosition(float& x, float& y);
};