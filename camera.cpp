#include "camera.h"
#include <cmath>

Camera::Camera() : zoomLevel(2.0f) // Start with 2x zoom out for better field of view
{
}

void Camera::createViewMatrix(float matrix[16]) const
{
  // Create a scaling matrix that zooms out the view
  // Higher zoom = smaller objects = can see more area
  float scale = 1.0f / zoomLevel;

  // Initialize to identity matrix
  for (int i = 0; i < 16; i++)
    matrix[i] = 0.0f;

  // Set scaling matrix
  matrix[0] = scale;   // X scale
  matrix[5] = scale;   // Y scale  
  matrix[10] = 1.0f;   // Z scale (unchanged)
  matrix[15] = 1.0f;   // W component
}

void Camera::screenToWorld(float screenX, float screenY, int windowWidth, int windowHeight, float& worldX, float& worldY) const
{
  // Convert screen coordinates to normalized device coordinates (-1 to 1)
  float centerX = windowWidth / 2.0f;
  float centerY = windowHeight / 2.0f;
  float deltaX = screenX - centerX;
  float deltaY = centerY - screenY; // Flip Y coordinate

  // Apply zoom to get world coordinates
  worldX = (deltaX / centerX) * zoomLevel;
  worldY = (deltaY / centerY) * zoomLevel;
}