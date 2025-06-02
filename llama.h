#pragma once

#include <memory>

class Shader;

class Llama
{
public:
  Llama();
  ~Llama();

  // Initialize OpenGL resources
  bool initialize(const char* texturePath);

  // Set llama rotation angle
  void setRotation(float angle) { rotation = angle; }

  // Get current rotation
  float getRotation() const { return rotation; }

  // Update animation
  void update(float deltaTime);

  // Render the llama
  void render(std::shared_ptr<Shader> shader);

  // Get position (center of llama)
  float getX() const { return 0.0f; }  // Llama is always at center
  float getY() const { return 0.0f; }

  // Animation control
  void setAnimationSpeed(float speed) { animationSpeed = speed; }
  void setCurrentFrame(int frame) { currentFrame = frame % 24; } // 24 frames total

private:
  float rotation;
  float animationTime;
  float animationSpeed;
  int currentFrame;

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