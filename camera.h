#pragma once

class Camera
{
public:
  Camera();

  // Set the zoom level (higher = more zoomed out, can see more)
  void setZoom(float zoom) { zoomLevel = zoom; }
  float getZoom() const { return zoomLevel; }

  // Create view matrix that can be applied to all objects
  void createViewMatrix(float matrix[16]) const;

  // Convert screen coordinates to world coordinates
  void screenToWorld(float screenX, float screenY, int windowWidth, int windowHeight, float& worldX, float& worldY) const;

private:
  float zoomLevel; // 1.0 = normal, 2.0 = see twice as much area
};