#pragma once

#include <string>

class Shader
{
public:
  unsigned int ID;

  // Constructor reads and builds the shader
  Shader(const char* vertexSource, const char* fragmentSource);

  // Use/activate the shader
  void use();

  // Utility uniform functions
  void setBool(const std::string& name, bool value) const;
  void setInt(const std::string& name, int value) const;
  void setFloat(const std::string& name, float value) const;
  void setMatrix4fv(const std::string& name, const float* matrix) const;

  // Cleanup
  ~Shader();

private:
  // Utility function for checking shader compilation/linking errors
  unsigned int compileShader(unsigned int type, const char* source);
  void checkCompileErrors(unsigned int shader, const std::string& type);
};