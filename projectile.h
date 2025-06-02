#pragma once

#include <vector>
#include <memory>
#include <random>
#include <chrono>

class Shader;

struct Projectile {
  float x, y;          // Position
  float velX, velY;    // Velocity
  float life;          // Time alive (for cleanup and animation)
  int frame;           // Current animation frame (0-3)

  Projectile(float startX, float startY, float vx, float vy);
};

class ProjectileManager
{
public:
  ProjectileManager();
  ~ProjectileManager();

  // Initialize OpenGL resources
  bool initialize(const char* texturePath);

  // Add a new projectile with spray and timing variations
  void addProjectile(float startX, float startY, float angle, float speed = 1.5f);

  // Add projectile with custom spray settings
  void addProjectileWithSpray(float startX, float startY, float angle, float speed = 1.5f,
    float sprayPercent = 1.0f);

  // Check if enough time has passed for next shot (with timing error)
  bool canShoot(float baseIntervalMs = 200.0f, float timingErrorPercent = 2.0f);

  // Update the last shot time (call when actually shooting)
  void updateLastShotTime();

  // Update all projectiles
  void update(float deltaTime);

  // Render all projectiles
  void render(std::shared_ptr<Shader> shader);

  // Get projectile count
  size_t getProjectileCount() const { return projectiles.size(); }

  // Clear all projectiles
  void clear();

private:
  std::vector<Projectile> projectiles;

  // Random number generation for spray and timing
  mutable std::random_device rd;
  mutable std::mt19937 gen;
  mutable std::uniform_real_distribution<float> dis;

  // Timing for shot intervals
  std::chrono::steady_clock::time_point lastShotTime;

  // OpenGL resources
  unsigned int VAO, VBO, EBO;
  unsigned int texture;

  // Shader sources
  static const char* vertexShaderSource;
  static const char* fragmentShaderSource;

  // Helper functions
  void setupMesh();
  unsigned int loadTexture(const char* path);
  void getFrameCoords(int frame, float coords[8]);
};