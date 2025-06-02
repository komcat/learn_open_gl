#include "llama.h"
#include "shader.h"
#include "texture_loader.h"
#include <glad/glad.h>
#include <iostream>
#include <cmath>

// Vertex shader source
const char* Llama::vertexShaderSource = R"(
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
const char* Llama::fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

Llama::Llama() : rotation(0.0f), animationTime(0.0f), animationSpeed(8.0f), currentFrame(0), VAO(0), VBO(0), EBO(0), texture(0)
{
}

Llama::~Llama()
{
  if (VAO) glDeleteVertexArrays(1, &VAO);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (EBO) glDeleteBuffers(1, &EBO);
  if (texture) glDeleteTextures(1, &texture);
}

bool Llama::initialize(const char* texturePath)
{
  setupMesh();

  // Load texture
  texture = loadTexture(texturePath);
  if (texture == 0)
  {
    std::cout << "Failed to load llama texture: " << texturePath << std::endl;
    return false;
  }

  return true;
}

void Llama::setupMesh()
{
  // Vertex data for llama quad
  float llamaVertices[] = {
    // positions        // texture coords (will be updated dynamically)
     0.3f,  0.3f, 0.0f,  0.5f, 1.0f,     // top right 
     0.3f, -0.3f, 0.0f,  0.5f, 0.667f,   // bottom right
    -0.3f, -0.3f, 0.0f,  0.0f, 0.667f,   // bottom left
    -0.3f,  0.3f, 0.0f,  0.0f, 1.0f      // top left 
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(llamaVertices), llamaVertices, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

unsigned int Llama::loadTexture(const char* path)
{
  return TextureLoader::loadTexture(path);
}

void Llama::update(float deltaTime)
{
  animationTime += deltaTime;

  // Update animation frame (change frame based on animation speed)
  currentFrame = (int)(animationTime * animationSpeed) % 24; // 24 frames total
}

void Llama::getFrameCoords(int frame, float coords[8])
{
  // Sprite sheet: 120x120, 5x5 grid (24 frames total), each frame 24x24
  // Frame layout based on JSON:
  // 0  | 1  | 2  | 3  | 4     (y=0)
  // 5  | 6  | 7  | 8  | 9     (y=24) 
  // 10 | 11 | 12 | 13 | 14    (y=48)
  // 15 | 16 | 17 | 18 | 19    (y=72)
  // 20 | 21 | 22 | 23 | --    (y=96)

  float frameWidth = 24.0f / 120.0f;   // 24/120 = 0.2
  float frameHeight = 24.0f / 120.0f;  // 24/120 = 0.2

  int col = frame % 5;       // 0, 1, 2, 3, or 4
  int row = frame / 5;       // 0, 1, 2, 3, or 4

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

void Llama::render(std::shared_ptr<Shader> shader)
{
  shader->use();

  // Get texture coordinates for current frame
  float frameCoords[8];
  getFrameCoords(currentFrame, frameCoords);

  // Update vertex buffer with new texture coordinates
  float llamaVertices[] = {
    // positions        // texture coords (updated per frame)
     0.3f,  0.3f, 0.0f,  frameCoords[0], frameCoords[1],  // top right
     0.3f, -0.3f, 0.0f,  frameCoords[2], frameCoords[3],  // bottom right
    -0.3f, -0.3f, 0.0f,  frameCoords[4], frameCoords[5],  // bottom left
    -0.3f,  0.3f, 0.0f,  frameCoords[6], frameCoords[7]   // top left
  };

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(llamaVertices), llamaVertices);

  // Create transformation matrix
  float transform[16];
  float cosA = cos(rotation);
  float sinA = sin(rotation);

  // Initialize to identity matrix
  for (int i = 0; i < 16; i++)
    transform[i] = 0.0f;

  // Set rotation matrix values (no translation since llama stays at center)
  transform[0] = cosA;   transform[1] = sinA;   transform[2] = 0.0f;  transform[3] = 0.0f;
  transform[4] = -sinA;  transform[5] = cosA;   transform[6] = 0.0f;  transform[7] = 0.0f;
  transform[8] = 0.0f;   transform[9] = 0.0f;   transform[10] = 1.0f; transform[11] = 0.0f;
  transform[12] = 0.0f;  transform[13] = 0.0f;  transform[14] = 0.0f; transform[15] = 1.0f;

  shader->setMatrix4fv("transform", transform);

  // Bind texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  shader->setInt("ourTexture", 0);

  // Render
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}