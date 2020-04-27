#ifndef VIEWER_H
#define VIEWER_H

// GLEW lib: needs to be included first!!
#include <GL/glew.h> 

// OpenGL library 
#include <GL/gl.h>

// OpenGL Utility library
#include <GL/glu.h>

// OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QGLFormat>
#include <QGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <stack>

#include "camera.h"
#include "shader.h"
#include "grid.h"

class Viewer : public QGLWidget {
 public:
  Viewer(char *filename,const QGLFormat &format=QGLFormat::defaultFormat());
  ~Viewer();
  
 protected :
  virtual void paintGL();
  virtual void initializeGL();
  virtual void resizeGL(int width,int height);
  virtual void keyPressEvent(QKeyEvent *ke);
  virtual void mousePressEvent(QMouseEvent *me);
  virtual void mouseMoveEvent(QMouseEvent *me);

 private:
  // OpenGL objects creation
  void createVAO();
  void deleteVAO();

  void createShaders();
  void deleteShaders();
  void reloadShaders();

  void createFBO();
  void deleteFBO();
  void initFBO();

  void createTextures();
  void enableTexture(const char *filename, int tex_id);
  void deleteTextures();
  void sendTexture(const char * varname, int texid,GLenum texture, GLuint shader_id);
  // drawing functions 
  void drawScene(GLuint id);

  QTimer        *_timer;    // timer that controls the animation

  Grid   *_grid;   // the grid
  Camera *_cam;    // the camera

  glm::vec3 _light;  // light direction
  float _time; //faire bouger la lumière selon le temps
  glm::vec3 _motion; // motion offset for the noise texture 
  bool      _mode;   // camera motion or light motion

  // les shaders 
  Shader *_terrainShader;
  //  Shader *_shaderFirstPass; // shader pour la géométrie fbo
  // Shader *_shaderSecondPass; // shader pour la lumière
  
  // vbo/vao ids 
  GLuint _vaoTerrain;
  GLuint _terrain[2];

  unsigned int _ndResol;

  //GLuint _fbo;
  //Gluint _rendColorId;
   GLuint _texIds[3];
};

#endif // VIEWER_H
