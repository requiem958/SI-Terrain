#include "viewer.h"

#include <math.h>
#include <iostream>
#include <QTime>
#include <QImage>

using namespace std;

Viewer::Viewer(char *,const QGLFormat &format)
  : QGLWidget(format),
    _timer(new QTimer(this)),
    _light(glm::vec3(0,0,1)),
    _time(0.0),
    _motion(glm::vec3(0,0,0)),
    _mode(false),
    _showShadowMap(false),
    _ndResol(512),
    _depthResol(1024){

  setlocale(LC_ALL,"C");

  _grid = new Grid(_ndResol,-10.0f,10.0f);
  _cam  = new Camera(1.0f,glm::vec3(0.0f,0.0f,0.0f));

  _timer->setInterval(10);
  connect(_timer,SIGNAL(timeout()),this,SLOT(updateGL()));
}

Viewer::~Viewer() {
  delete _timer;
  delete _grid;
  delete _cam;

  // delete all GPU objects
  deleteShaders();
  deleteTextures();
  deleteVAO();
  deleteFBO();
}

/***
 * VAO THINGS
 *
 */

void Viewer::createVAO() {

    // data associated with the screen quad
  const GLfloat quadData[] = { 
    -1.0f,-1.0f,0.0f,
    1.0f,-1.0f,0.0f,
    -1.0f,1.0f,0.0f,
    -1.0f,1.0f,0.0f,
    1.0f,-1.0f,0.0f,
    1.0f,1.0f,0.0f
  }; 
  // cree les buffers associés au terrain 

  glGenBuffers(2,_terrain);
  glGenVertexArrays(1,&_vaoTerrain);
  glGenVertexArrays(1,&_vaoQuad);

  // create the VBO associated with the grid (the terrain)
  glBindVertexArray(_vaoTerrain);
  glBindBuffer(GL_ARRAY_BUFFER,_terrain[0]); // vertices 
  glBufferData(GL_ARRAY_BUFFER,_grid->nbVertices()*3*sizeof(float),_grid->vertices(),GL_STATIC_DRAW);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void *)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,_terrain[1]); // indices 
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,_grid->nbFaces()*3*sizeof(int),_grid->faces(),GL_STATIC_DRAW);

    // create the VBO associated with the screen quad 
  glGenBuffers(1,&_quad);
  glBindVertexArray(_vaoQuad);
  glBindBuffer(GL_ARRAY_BUFFER,_quad);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadData),quadData,GL_STATIC_DRAW);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void *)0);
  glEnableVertexAttribArray(0);

  
  glBindVertexArray(0);
}

void Viewer::deleteVAO() {
  glDeleteBuffers(2,_terrain);
  glDeleteVertexArrays(1,&_vaoTerrain);
  
  glDeleteBuffers(1,&_quad);
  glDeleteVertexArrays(1,&_vaoQuad);
  
}
/***
 * FBO THINGS
 *
 */

void Viewer::createFBO() {
  // generate fbo and associated textures
  glGenFramebuffers(2,&(_fbo[0]));
  glGenTextures(1,&_texDepth);
  glGenTextures(1,&_texTerrain);
}

void Viewer::initFBO(){
  // create the texture for rendering depth values
  glBindTexture(GL_TEXTURE_2D,_texDepth);
  glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,_depthResol,_depthResol,0,GL_DEPTH_COMPONENT,GL_FLOAT,NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

  // create the texture for rendering colors
  glBindTexture(GL_TEXTURE_2D,_texTerrain);
  glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F,width(),height(),0,GL_RGBA,GL_FLOAT,NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // attach textures to framebuffer object 
  glBindFramebuffer(GL_FRAMEBUFFER,_fbo[0]);
  glBindTexture(GL_TEXTURE_2D,_texDepth);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,_texDepth,0);

    // test if everything is ok
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    cout << "Warning: FBO not complete!" << endl;

  glBindFramebuffer(GL_FRAMEBUFFER,_fbo[1]);
  glBindTexture(GL_TEXTURE_2D,_texTerrain);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,_texTerrain,0);

  // test if everything is ok
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    cout << "Warning: FBO not complete!" << endl;

  // disable FBO
  glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void Viewer::deleteFBO() {
  // delete all FBO Ids
  glDeleteFramebuffers(2,&(_fbo[0]));
  glDeleteTextures(1,&_texDepth);
  glDeleteTextures(1,&_texTerrain);
}

/***
 * SHADER THINGS
 *
 */

void Viewer::createShaders() {
  _terrainShader = new Shader();
  _shadowMapShader = new Shader(); // will create the shadow map
  _debugMapShader = new Shader();
  _shaderSecondPass = new Shader();

  _terrainShader->load("shaders/terrain.vert","shaders/terrain.frag");
  _shadowMapShader->load("shaders/shadow-map.vert","shaders/shadow-map.frag");
  _debugMapShader->load("shaders/show-shadow-map.vert","shaders/show-shadow-map.frag");
  _shaderSecondPass->load("shaders/second-pass.vert","shaders/second-pass.frag");
}

void Viewer::deleteShaders() {
  delete _terrainShader;
  delete _shadowMapShader;
  delete _debugMapShader;
  delete _shaderSecondPass;

  _terrainShader = NULL;
  _shadowMapShader = NULL;
  _debugMapShader = NULL;
  _shaderSecondPass = NULL;
}

void Viewer::reloadShaders() {
  if(_terrainShader)
    _terrainShader->reload("shaders/terrain.vert","shaders/terrain.frag");
  if(_shadowMapShader)
    _shadowMapShader->reload("shaders/shadow-map.vert","shaders/shadow-map.frag");
  if(_debugMapShader)
    _debugMapShader->reload("shaders/show-shadow-map.vert","shaders/show-shadow-map.frag");
  if(_shaderSecondPass)
    _shaderSecondPass->reload("shaders/second-pass.vert","shaders/second-pass.frag");
}

/***
 * TEXTURE THINGS
 *
 */

void Viewer::createTextures() {
 
  
  // enable the use of 2D textures 
  glEnable(GL_TEXTURE_2D);

  
  // create three textures on the GPU
  glGenTextures(4,_texIds);

  // load and enable all textures (CPU side)
  enableTexture("textures/forest.jpg",_texIds[0]);
  enableTexture("textures/sol.jpg",_texIds[1]);
  enableTexture("textures/snow.jpg",_texIds[2]);
  //enableTexture("textures/water.jpg",_texIds[3]);  
}

void Viewer::enableTexture(const char * filename, int tex_id){
   QImage texture = QGLWidget::convertToGLFormat(QImage(filename)); 
  // activate color texture
  glBindTexture(GL_TEXTURE_2D,tex_id);

  // set texture parameters 
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  // transfer data from CPU to GPU memory
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
	       texture.width(),texture.height(),0,
  	       GL_RGBA,GL_UNSIGNED_BYTE,
	       (const GLvoid *)texture.bits());
  // generate mipmaps 
  glGenerateMipmap(GL_TEXTURE_2D);
}

void Viewer::deleteTextures() {
  glDeleteTextures(4,_texIds);
}

//Helper to send a texture to graphic card
void Viewer::sendTexture(const char * varname, int texid,GLenum texture, GLuint shader_id){
  glActiveTexture(texture);
  glBindTexture(GL_TEXTURE_2D,_texIds[texid]);
  glUniform1i(glGetUniformLocation(shader_id,varname),texid);
}

/***
 * DRAWERS
 *
 */

void Viewer::drawScene(GLuint id) {

  const float size = 10.0;
  const glm::vec3 l   = glm::transpose(_cam->normalMatrix())*_light;
  const glm::mat4 p   = glm::ortho<float>(-size,size,-size,size,-size,2*size);
  const glm::mat4 v   = glm::lookAt(l, glm::vec3(0,0,0), glm::vec3(0,1,0));
  const glm::mat4 m   = glm::mat4(1.0);
  const glm::mat4 mv  = v*m;
  const glm::mat4 mvpDepth = p*mv;

  // send uniform variables 
  glUniformMatrix4fv(glGetUniformLocation(id,"mdvMat"),1,GL_FALSE,&(_cam->mdvMatrix()[0][0]));
  glUniformMatrix4fv(glGetUniformLocation(id,"projMat"),1,GL_FALSE,&(_cam->projMatrix()[0][0]));
  glUniformMatrix3fv(glGetUniformLocation(id,"normalMat"),1,GL_FALSE,&(_cam->normalMatrix()[0][0]));
  glUniform3fv(glGetUniformLocation(id,"light"),1,&(_light[0]));
  glUniform1f(glGetUniformLocation(id,"time"),_time);
  glUniform3fv(glGetUniformLocation(id,"motion"),1,&(_motion[0]));
  //Water parameters
  glUniform1f(glGetUniformLocation(id,"water_low"),-0.5);
  glUniform1f(glGetUniformLocation(id,"flow_low"),-0.21);
  glUniform1f(glGetUniformLocation(id,"flow_high"),-0.2);

  sendTexture("forest",0,GL_TEXTURE0,id);
  sendTexture("sol",1,GL_TEXTURE1,id);
  sendTexture("snow",2,GL_TEXTURE2,id);
  //sendTexture("water",3,GL_TEXTURE3,id);

  
  
  glUniformMatrix4fv(glGetUniformLocation(id,"mvpDepthMat"),1,GL_FALSE,&mvpDepth[0][0]);
  
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,_texDepth);
  glUniform1i(glGetUniformLocation(id,"shadowmap"),3);

  // draw faces 
  glBindVertexArray(_vaoTerrain);
  glDrawElements(GL_TRIANGLES,3*_grid->nbFaces(),GL_UNSIGNED_INT,(void *)0);
  glBindVertexArray(0);
  _time += 0.01;
}

void Viewer::drawSceneFromLight(GLuint id) {

  const float size = 10.0;
  const glm::vec3 l   = glm::transpose(_cam->normalMatrix())*_light;
  const glm::mat4 p   = glm::ortho<float>(-size,size,-size,size,-size,2*size);
  const glm::mat4 v   = glm::lookAt(l, glm::vec3(0,0,0), glm::vec3(0,1,0));
  const glm::mat4 m   = glm::mat4(1.0);
  const glm::mat4 mv  = v*m;
  const glm::mat4 mvp = p*mv;
  
  glUniform3fv(glGetUniformLocation(id,"motion"),1,&(_motion[0]));
  //Water parameters
  glUniform1f(glGetUniformLocation(id,"water_low"),-0.5);
  glUniformMatrix4fv(glGetUniformLocation(id,"mvpMat"),1,GL_FALSE,&mvp[0][0]);

  glBindVertexArray(_vaoTerrain);
  glDrawElements(GL_TRIANGLES,3*_grid->nbFaces(),GL_UNSIGNED_INT,(void *)0);
  glBindVertexArray(0);
}

void Viewer::drawShadowMap(GLuint id) {
  // send depth texture 
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,_texDepth);
  glUniform1i(glGetUniformLocation(id,"shadowmap"),0);
}

void Viewer::drawQuad() {

  // Draw the 2 triangles !
  glBindVertexArray(_vaoQuad);
  glDrawArrays(GL_TRIANGLES,0,6);
  glBindVertexArray(0);
}

/***
 * GL THINGS
 *
 */

void Viewer::paintGL() {

  GLenum DrawBuffers[1];
  /*** SHADOW MAPPING HERE **/

  glBindFramebuffer(GL_FRAMEBUFFER,_fbo[0]);


  // we only want to write in the depth texture (automatic thanks to the depth OpenGL test)
  glDrawBuffer(GL_NONE);

  // set the viewport at the good texture resolution
  glViewport(0,0,_depthResol,_depthResol);

  // clear depth buffer 
  glClear(GL_DEPTH_BUFFER_BIT);

  // activate the shadow map shader
  glUseProgram(_shadowMapShader->id());

  // create the shadow map 
  drawSceneFromLight(_shadowMapShader->id());

  // desactivate fbo
  glBindFramebuffer(GL_FRAMEBUFFER,0);

  /** END OF SHADOW MAPPING HERE ***/

  glBindFramebuffer(GL_FRAMEBUFFER,_fbo[1]);
  // Set the list of draw buffers.
  DrawBuffers[0] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);
  // allow opengl depth test 
  glEnable(GL_DEPTH_TEST);
  
  // screen viewport
  glViewport(0,0,width(),height());

  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // activate the buffer shader 
  glUseProgram(_terrainShader->id());

  // generate the map
  drawScene(_terrainShader->id());

  glBindFramebuffer(GL_FRAMEBUFFER,0);
  // show the shadow map (press O key) 
  if(_showShadowMap) {
    // activate the test shader  
    glUseProgram(_debugMapShader->id());

    // clear buffers 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // display the shadow map 
    drawShadowMap(_debugMapShader->id());
    drawQuad();
  }else{
    // activate the shader 
    glUseProgram(_shaderSecondPass->id());
    
    // clear everything
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // send textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,_texTerrain);
    glUniform1i(glGetUniformLocation(_shaderSecondPass->id(),"colormap"),0);
    drawQuad();
  
  }
  // disable shader 
  glUseProgram(0);
}

void Viewer::resizeGL(int width,int height) {
  _cam->initialize(width,height,false);
  glViewport(0,0,width,height);
  updateGL();
}

void Viewer::initializeGL() {
  // make this window the current one
  makeCurrent();

  // init and chack glew
  if(glewInit()!=GLEW_OK) {
    cerr << "Warning: glewInit failed!" << endl;
  }

  if(!GLEW_ARB_vertex_program   ||
     !GLEW_ARB_fragment_program ||
     !GLEW_ARB_texture_float    ||
     !GLEW_ARB_draw_buffers     ||
     !GLEW_ARB_framebuffer_object) {
    cerr << "Warning: Shaders not supported!" << endl;
  }

  // init OpenGL settings
  glClearColor(0.0,0.0,0.0,1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glViewport(0,0,width(),height());

  // initialize camera
  _cam->initialize(width(),height(),true);

  // init shaders 
  createShaders();

  // init VAO/VBO
  createVAO();

  //create and load texture
  createTextures();

  // init create FBO
  createFBO();
  initFBO();

  // starts the timer 
  //_timer->start();
}



/***
 * EVENT THINGS
 *
 */

void Viewer::mousePressEvent(QMouseEvent *me) {
  const glm::vec2 p((float)me->x(),(float)(height()-me->y()));

  if(me->button()==Qt::LeftButton) {
    _cam->initRotation(p);
    _mode = false;
  } else if(me->button()==Qt::MidButton) {
    _cam->initMoveZ(p);
    _mode = false;
  } else if(me->button()==Qt::RightButton) {
    _light[0] = (p[0]-(float)(width()/2))/((float)(width()/2));
    _light[1] = (p[1]-(float)(height()/2))/((float)(height()/2));
    _light[2] = 1.0f-std::max(fabs(_light[0]),fabs(_light[1]));
    _light = glm::normalize(_light);
    _mode = true;
  } 

  updateGL();
}

void Viewer::mouseMoveEvent(QMouseEvent *me) {
  const glm::vec2 p((float)me->x(),(float)(height()-me->y()));
 
  if(_mode) {
    // light mode
    _light[0] = (p[0]-(float)(width()/2))/((float)(width()/2));
    _light[1] = (p[1]-(float)(height()/2))/((float)(height()/2));
    _light[2] = 1.0f-std::max(fabs(_light[0]),fabs(_light[1]));
    _light = glm::normalize(_light);
  } else {
    // camera mode
    _cam->move(p);
  }

  updateGL();
}

void Viewer::keyPressEvent(QKeyEvent *ke) {
  const float step = 0.05;
  _time-=0.01;
  if(ke->key()==Qt::Key_Z) {
    glm::vec2 v = glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1))*step;
    if(v[0]!=0.0 && v[1]!=0.0) v = glm::normalize(v)*step;
    else v = glm::vec2(0,1)*step;
    _motion[0] += v[0];
    _motion[1] += v[1];
  }

  if(ke->key()==Qt::Key_S) {
    glm::vec2 v = glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1))*step;
    if(v[0]!=0.0 && v[1]!=0.0){
      v = glm::normalize(v)*step;
    }
    else {
      v = glm::vec2(0,1)*step;
    }
    _motion[0] -= v[0];
    _motion[1] -= v[1];
  }

  if(ke->key()==Qt::Key_Q) {
    _motion[1] += step;
  }

  if(ke->key()==Qt::Key_D) {
    _motion[1] -= step;
  }

  



  // key a: play/stop animation
  if(ke->key()==Qt::Key_A) {
    if(_timer->isActive()) 
      _timer->stop();
    else 
      _timer->start();
  }

  // key i: init camera
  if(ke->key()==Qt::Key_I) {
    _cam->initialize(width(),height(),true);
  }
  
  // // key f: compute FPS
  // if(ke->key()==Qt::Key_F) {
  //   int elapsed;
  //   QTime timer;
  //   timer.start();
  //   unsigned int nb = 500;
  //   for(unsigned int i=0;i<nb;++i) {
  //     paintGL();
  //   }
  //   elapsed = timer.elapsed();
  //   double t = (double)nb/((double)elapsed);
  //   cout << "FPS : " << t*1000.0 << endl;
  // }

  // key r: reload shaders 
  if(ke->key()==Qt::Key_R) {
    reloadShaders();
  }

  if(ke->key()==Qt::Key_O) {
    _showShadowMap = !_showShadowMap;
  }

  updateGL();
}

