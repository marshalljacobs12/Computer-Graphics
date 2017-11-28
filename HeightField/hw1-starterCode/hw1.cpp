/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: mjjacobs
*/

#include <iostream>
#include <cstring>
#include <iomanip>
#include <vector>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum { POINTS, WIREFRAME, TRIANGLES, OVERLAY} DISPLAY_TYPE;
DISPLAY_TYPE displayType = POINTS;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

OpenGLMatrix * matrix;
GLuint buffer;
BasicPipelineProgram * pipelineProgram;
GLuint program;
GLuint vao;

GLuint vbo_points_vertices;
GLuint vbo_points_colors;
GLuint vao_points;

GLuint vbo_wireframe_vertices;
GLuint vbo_wireframe_colors;
GLuint vao_wireframe;

GLuint vbo_triangle_vertices;
GLuint vbo_triangle_colors;
GLuint vao_triangle;

GLuint vbo_wireframe_overlay_vertices;
GLuint vbo_wireframe_overlay_colors;
GLuint vao_wireframe_overlay;

std::vector<GLfloat> point_positions;
std::vector<GLfloat> point_colors;
std::vector<GLfloat> wireframe_positions;
std::vector<GLfloat> wireframe_colors;
std::vector<GLfloat> triangle_positions;
std::vector<GLfloat> triangle_colors;
std::vector<GLfloat> wireframe_overlay_positions;
std::vector<GLfloat> wireframe_overlay_colors;


float heightfieldScale = 0.025f;

int num_screenshots = 0;

const int fovy = 45;
const float aspect = 1.7777777;
const float zStudent = 3.3115792608;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

//render triangle for Milestone
void renderTriangle()
{
  GLint first = 0;
  GLsizei numberOfVertices = 3;
  glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
}

/*
	calculates the height of all vertices and populates vectors 
	with position and color data
*/
void generateVertices() 
{
  heightfieldScale = heightfieldScale * (float) (heightmapImage->getWidth() / 100);

  int image_width = heightmapImage->getWidth();
  int image_height = heightmapImage->getHeight();

  GLfloat vertex[3];
  GLfloat vertex_right[3];
  GLfloat vertex_above[3];
  GLfloat vertex_above_right[3];

  //GLfloat vertex_color[4];
  GLfloat vertex_color[4];
  GLfloat vertex_color_right[4];
  GLfloat vertex_color_above[4];
  GLfloat vertex_color_above_right[4];

  //center the heightfield at the origin
  for (int i = -image_width / 2; i < image_width / 2 - 1; i++) {
    for (int j = -image_height / 2; j < image_height / 2 - 1; j++) {

      //need to significantly scale down height using heightfieldScale so that it renders properly in 1280x720 display
      GLfloat height = heightfieldScale * heightmapImage->getPixel(i + image_width / 2, j + image_height / 2, 0);

      //populate point_positions
      vertex[0] = (float)i;
      vertex[1] = height;
      vertex[2] = -(float)j;
      point_positions.insert(point_positions.end(), vertex, vertex + 3);

      /*
      	color interpolation done with the following formula:

			k*low.red + (1-k)*hi.red,
			k*low.green + (1-k)*hi.green,
			k*low.blue + (1-k)*hi.blue)

			where k = height / maxHeight
	  */

      float max_height = (float) heightmapImage->getHeight();
      GLfloat color_height = heightmapImage->getPixel(i + image_width / 2, j + image_height / 2, 0);
      float k = (float) (color_height / max_height);
      float r_val = (float)((k*255.0f + (1.0f-k)*.0f)/255.0f);
      float g_val = (float)((k*255.0f + (1.0f-k)*0.0f)/255.0f);
      float b_val = (float)((k*255.0f + (1.0f-k)*255.0f)/255.0f);
      vertex_color[0] = (float)r_val;
      vertex_color[1] = (float)g_val;
      vertex_color[2] = (float)b_val;
      vertex_color[3] = 1.0f;
      point_colors.insert(point_colors.end(), vertex_color, vertex_color + 4);

     
      //calculate points above, to the right, and above and to the right of current vertex for wireframe and triangles

      //pixel above current pixel (i, j, 0) is at location (i, j + 1, 0)
      GLfloat top_height = heightfieldScale * heightmapImage->getPixel(i + image_width /2, j + image_height / 2 + 1, 0);
      vertex_above[0] = (GLfloat) i; 
      vertex_above[1] = top_height;
      vertex_above[2] = -(GLfloat)(j+1);

      //pixel to the right of current pixel (i, j, 0) is at location (i + 1, j, 0)
      GLfloat right_height = heightfieldScale * heightmapImage->getPixel(i + image_width / 2 + 1, j + image_height / 2, 0);
      vertex_right[0] = (GLfloat)(i+1);
      vertex_right[1] = right_height;
      vertex_right[2] = -(GLfloat)j;

      //pixel above and to the right of current pixel (i, j, 0) is at location (i + 1, j + 1, 0)
      GLfloat above_right_height = heightfieldScale * heightmapImage->getPixel(i + image_width / 2 + 1, j + image_height / 2 + 1, 0);
      vertex_above_right[0] = (GLfloat)(i+1);
      vertex_above_right[1] = above_right_height;
      vertex_above_right[2] = -(GLfloat)(j+1);

      /*
			To render "box" wireframes:
				 __
				|__|  (as an illustration)

			Create two lines: 
				First create a line between the current vertex and the one above it
				Then create a line between the current vertex and the vertex to its right
      */

      wireframe_positions.insert(wireframe_positions.end(), vertex, vertex + 3);
      wireframe_positions.insert(wireframe_positions.end(), vertex_above, vertex_above +3);
      wireframe_positions.insert(wireframe_positions.end(), vertex, vertex + 3);
      wireframe_positions.insert(wireframe_positions.end(), vertex_right, vertex_right + 3);
      

      //calculate the colors of vertex_above, vertex_right, and vertex_above_right using the formula specfied above
      GLfloat color_top_height = heightmapImage->getPixel(i + image_width / 2, j + image_height / 2 + 1, 0);
      GLfloat k_above = (GLfloat) (color_top_height / max_height);
      GLfloat r_val_above = (GLfloat)((k_above * 255.0f + (1.0f-k_above)*0.0f)/255.0f);
      GLfloat g_val_above = (GLfloat)((k_above * 255.0f + (1.0f-k_above)*0.0f)/255.0f);
      GLfloat b_val_above = (GLfloat)((k_above * 255.0f + (1.0f-k_above)*255.0f)/255.0f);
      vertex_color_above[0] = r_val_above;
      vertex_color_above[1] = g_val_above;
      vertex_color_above[2] = b_val_above;
      vertex_color_above[3] = 1.0f;

      GLfloat color_right_height = heightmapImage->getPixel(i + image_width / 2 + 1, j + image_height / 2, 0);
      GLfloat k_right = (GLfloat) (color_right_height / max_height);
      GLfloat r_val_right = (GLfloat)((k_right * 255.0f + (1.0f-k_right)*0.0f)/255.0f);
      GLfloat g_val_right = (GLfloat)((k_right * 255.0f + (1.0f-k_right)*0.0f)/255.0f);
      GLfloat b_val_right = (GLfloat)((k_right * 255.0f + (1.0f-k_right)*255.0f)/255.0f);
      vertex_color_right[0] = r_val_right;
      vertex_color_right[1] = g_val_right;
      vertex_color_right[2] = b_val_right;
      vertex_color_right[3] = 1.0f;

      GLfloat color_above_right_height = heightmapImage->getPixel(i + image_width / 2 + 1, j + image_height / 2 + 1, 0);
      GLfloat k_above_right = (GLfloat) (color_above_right_height / max_height);
      GLfloat r_val_above_right = (GLfloat)((k_above_right * 255.0f + (1.0f-k_above_right)*0.0f)/255.0f);
      GLfloat g_val_above_right = (GLfloat)((k_above_right * 255.0f + (1.0f-k_above_right)*0.0f)/255.0f);
      GLfloat b_val_above_right = (GLfloat)((k_above_right * 255.0f + (1.0f-k_above_right)*255.0f)/255.0f);
      vertex_color_above_right[0] = r_val_above_right;
      vertex_color_above_right[1] = g_val_above_right;
      vertex_color_above_right[2] = b_val_above_right;
      vertex_color_above_right[3] = 1.0f;

      //insert the calculated colors in the same order as vertices were inserted into wireframe_positions
      wireframe_colors.insert(wireframe_colors.end(), vertex_color, vertex_color + 4);
      wireframe_colors.insert(wireframe_colors.end(), vertex_color_above, vertex_color_above + 4);
      wireframe_colors.insert(wireframe_colors.end(), vertex_color, vertex_color + 4);
      wireframe_colors.insert(wireframe_colors.end(), vertex_color_right, vertex_color_right + 4);
      
      /*
			Two triangles are needed to approximate a surface:
			 _  __
			| \ \ |  (Crude approximation for illustration purposes)
			|__\ \|

			Therefore, the vector containing triangle positions needs 6 vertices per pixel
      */

      //triangles
      triangle_positions.insert(triangle_positions.end(), vertex, vertex + 3);
      triangle_positions.insert(triangle_positions.end(), vertex_right, vertex_right + 3);
      triangle_positions.insert(triangle_positions.end(), vertex_above, vertex_above + 3);
      triangle_positions.insert(triangle_positions.end(), vertex_above, vertex_above + 3);      
      triangle_positions.insert(triangle_positions.end(), vertex_right, vertex_right + 3);
      triangle_positions.insert(triangle_positions.end(), vertex_above_right, vertex_above_right + 3);

      //insert the calculated colors in the same order as vertices were inserted into triangle_positions
      triangle_colors.insert(triangle_colors.end(), vertex_color, vertex_color + 4);
      triangle_colors.insert(triangle_colors.end(), vertex_color_right, vertex_color_right + 4);
      triangle_colors.insert(triangle_colors.end(), vertex_color_above, vertex_color_above + 4);
      triangle_colors.insert(triangle_colors.end(), vertex_color_above, vertex_color_above + 4);
      triangle_colors.insert(triangle_colors.end(), vertex_color_right, vertex_color_right + 4);
      triangle_colors.insert(triangle_colors.end(), vertex_color_above_right, vertex_color_above_right + 4);

      /*
      		I made a separate vector to store the wireframe overlay positions because they needeed a new color
      		even though it was probably possible to use the old wireframe_positions with the new colors. I thought
      		this would be simpler
      */
      wireframe_overlay_positions.insert(wireframe_overlay_positions.end(), vertex, vertex + 3);
      wireframe_overlay_positions.insert(wireframe_overlay_positions.end(), vertex_above, vertex_above + 3);
      wireframe_overlay_positions.insert(wireframe_overlay_positions.end(), vertex, vertex + 3);
      wireframe_overlay_positions.insert(wireframe_overlay_positions.end(), vertex_right, vertex_right + 3);

      //the wireframes in the overlay display will simply be red with no color interpolation
      GLfloat wireframe_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
      wireframe_overlay_colors.insert(wireframe_overlay_colors.end(), wireframe_color, wireframe_color + 4);
      wireframe_overlay_colors.insert(wireframe_overlay_colors.end(), wireframe_color, wireframe_color + 4);
      wireframe_overlay_colors.insert(wireframe_overlay_colors.end(), wireframe_color, wireframe_color + 4);
      wireframe_overlay_colors.insert(wireframe_overlay_colors.end(), wireframe_color, wireframe_color + 4);

    }
  }
}

void initVBOs()
{

  //points VAO and VBOs
  glGenVertexArrays(1, &vao_points);
  glBindVertexArray(vao_points); //bind the points VAO

  //upload points position data
  glGenBuffers(1, &vbo_points_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_points_vertices);
  glBufferData(GL_ARRAY_BUFFER, point_positions.size() * sizeof(GLfloat), &point_positions[0], GL_STATIC_DRAW);

  //upload points color data
  glGenBuffers(1, &vbo_points_colors);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_points_colors);
  glBufferData(GL_ARRAY_BUFFER, point_colors.size() * sizeof(GLfloat), &point_colors[0], GL_STATIC_DRAW);

  //wireframe VAO and VBOs
  glGenVertexArrays(1, &vao_wireframe);
  glBindVertexArray(vao_wireframe); //bind the wireframe VAO

  //upload wireframe position data
  glGenBuffers(1, &vbo_wireframe_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_vertices);
  glBufferData(GL_ARRAY_BUFFER, wireframe_positions.size() * sizeof(GLfloat), &wireframe_positions[0], GL_STATIC_DRAW);

  //upload wireframe color data
  glGenBuffers(1, &vbo_wireframe_colors);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_colors);
  glBufferData(GL_ARRAY_BUFFER, wireframe_colors.size() * sizeof(GLfloat), &wireframe_colors[0], GL_STATIC_DRAW);

  //triangle VAO and VBOs
  glGenVertexArrays(1, &vao_triangle);
  glBindVertexArray(vao_triangle); //bind the triangle VAO

  //upload triangle position data
  glGenBuffers(1, &vbo_triangle_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_vertices);
  glBufferData(GL_ARRAY_BUFFER, triangle_positions.size() * sizeof(GLfloat), &triangle_positions[0], GL_STATIC_DRAW);

  //upload triangle color data
  glGenBuffers(1, &vbo_triangle_colors);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_colors);
  glBufferData(GL_ARRAY_BUFFER, triangle_colors.size() * sizeof(GLfloat), &triangle_colors[0], GL_STATIC_DRAW);

  //overlay wireframe VAO and VBOs
  glGenVertexArrays(1, &vao_wireframe_overlay);
  glBindVertexArray(vao_wireframe_overlay); //bind the wireframe VAO

  //upload the wireframe position data
  glGenBuffers(1, &vbo_wireframe_overlay_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_overlay_vertices);
  glBufferData(GL_ARRAY_BUFFER, wireframe_overlay_positions.size() * sizeof(GLfloat), &wireframe_overlay_positions[0], GL_STATIC_DRAW);

  //upload the wireframe color data
  glGenBuffers(1, &vbo_wireframe_overlay_colors);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_overlay_colors);
  glBufferData(GL_ARRAY_BUFFER, wireframe_overlay_colors.size() * sizeof(GLfloat), &wireframe_overlay_colors[0], GL_STATIC_DRAW);

}

//renders the POINTS heightfield
void renderPoints()
{
  //position data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_points_vertices); //bind vbo_points_vertices
  GLuint loc = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  //color data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_points_colors); //bind vbo_points_colors
  loc = glGetAttribLocation(program, "color"); //get the location of the "color" shader variable
  glEnableVertexAttribArray(loc); //enable the "color" attribute
  //set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_POINTS, 0, point_positions.size() / 3);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

//renders the WIREFRAME heightfield
void renderWireframe() {
  //position data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_vertices); //bind vbo_wireframe_vertices
  GLuint loc = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  //color data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_colors); //bind vbo_wireframe_colors
  loc  = glGetAttribLocation(program, "color"); //get the location of the "color" shader variable
  glEnableVertexAttribArray(loc); //enable the "color" attribute
  //set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_LINES, 0, wireframe_positions.size() / 3);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

}

//renders the TRIANGLES heightfeild
void renderTriangles() {
  //position data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_vertices); //bind vbo_triangle_vertices
  GLuint loc = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  //color data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_colors); //bind vbo_triangle_colors
  loc = glGetAttribLocation(program, "color"); //get the location of the "color" shader variable
  glEnableVertexAttribArray(loc); //enable the "color" attribute
  //set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_TRIANGLES, 0, triangle_positions.size() / 3);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

//renders the wireframe heightfield for OVERELAY display
void renderWireframeOverlay() 
{
  //position data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_overlay_vertices); //bind vbo_wireframe_overlay_vertices
  GLuint loc = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  //color data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_wireframe_overlay_colors); //bind vbo_wireframe_overlay_colors
  loc = glGetAttribLocation(program, "color"); //get the location of the "color" shader variable
  glEnableVertexAttribArray(loc); //enable the “color” attribute
  //set the layout of the "color" attribute data
  glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_LINES, 0, wireframe_overlay_positions.size() / 3);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

//renders the OVERLAY heightfields
void renderOverlay() 
{
  //render wireframe in GL_LINE mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(1.0f, 1.0f); //avoids z-buffer fighting

  renderWireframeOverlay();

  glDisable(GL_POLYGON_OFFSET_LINE);

  //render triangles in GL_FILL mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  renderTriangles();
;
}

void transform()
{
  //set model-view matrix
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();

  //first 3: camera position, next 3: where does the camera look at, last 3: up direction
  //matrix->LookAt(0,0,zStudent,0,0,-1,0,1,0); for Milestone
  matrix->LookAt(256, 256, 256, 0, 0, 0, 0, 1, 0);

  //transformation
  matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix->Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
  matrix->Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
  matrix->Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
  matrix->Scale(landScale[0], landScale[1], landScale[2]);

  //upload model-view matrix to GPU
  float m[16]; // column-major
  matrix->GetMatrix(m);
  pipelineProgram->SetModelViewMatrix(m);

  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(45, aspect, 0.01, 1000.0);

  //upload projection matrix to GPU
  float p[16]; // column-major
  matrix->GetMatrix(p);
  pipelineProgram->SetProjectionMatrix(p);
}

void displayFunc()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  pipelineProgram->Bind();

  transform();

  //switch on displayType which can be changed with keyboard input
  switch(displayType) 
  {
  	case POINTS:
  		renderPoints();
  		break;

  	case WIREFRAME:
  		renderWireframe();
  		break;

  	case TRIANGLES:
  		renderTriangles();
  		break;

    case OVERLAY:
      renderOverlay();
      break;

  }

  //for double buffering
  glutSwapBuffers();
}

void idleFunc()
{
  if(num_screenshots < 300) {
  	//first 75 screenshots in with POINTS rendering
    if(num_screenshots < 75) {
      displayType = POINTS;
      //translate on x, y, and z axes
      if(num_screenshots < 12) {
        landTranslate[0] += 2.0f;
      } else if (num_screenshots < 25) {
        landTranslate[0] -= 2.0f;
      } else if (num_screenshots < 37) {
        landTranslate[1] += 2.0f;
      } else if (num_screenshots < 50) {
        landTranslate[1] -= 2.0f;
      } else if (num_screenshots < 62) {
        landTranslate[2] += 2.0f;
      } else {
        landTranslate[2] -= 2.0f;
      }
    }
    //next 75 screenshots with WIREFRAME rendering
    //rotate around the x axis and begin rotation around y axis
    else if (num_screenshots < 150) {
      displayType = WIREFRAME;
      landScale[0] = 1.2f;
      landScale[1] = 1.2f;
      landScale[2] = 1.2f;
      if(num_screenshots < 125) {
        landRotate[0] += (float)(360/50);
      } else {
        landRotate[1] += (float)(360/50);
      }
    } 
    //third set of 75 screenshots with TRIANGLES rendering
    //continue rotating around y axis and rotate around z axis
    else if (num_screenshots < 225) {
      displayType = TRIANGLES;
      if(num_screenshots < 175) {
        landRotate[1] += (float)(360/50);
      } else {
        landRotate[2] += (float)(360/50);
      }
    } 
    //final 75 screenshots with OVERLAY rendering
    else {
      displayType = OVERLAY;
      //scale up and down to show the wireframe overlay on top of triangles
      if(num_screenshots < 263) {
        landScale[0] += 0.1f;
        landScale[1] += 0.1f;
        landScale[2] += 0.1f;
      } else {
        landScale[0] -= 0.1f;
        landScale[1] -= 0.1f;
        landScale[2] -= 0.1f;
      }
    }
    //take a screenshot
    char anim_num[5];
    sprintf(anim_num, "%03d", num_screenshots);
    saveScreenshot(("animation/" + std::string(anim_num) + ".jpg").c_str());
    num_screenshots++;
  }
  // make the screen update
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{

  glViewport(0, 0, w, h);

  // setup perspective matrix...
  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(45, aspect, 0.01, 1000.0);
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  
  
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.1f;
        landTranslate[1] -= mousePosDelta[1] * 0.1f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.1f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
  	//glut_left_button when clicking trackpad
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    //glut_middle_button when clicking OPTION + trackpad
    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;
    //glut_right_button when clicking CTRL + trackpad 
    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
  	//CTRL key doesn't work on my Mac so I also have this functionality using 
  	//keyboard input (key: 't') with mouse drag
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
    	exit(0); // exit the program
   		break;

    case ' ':
    	cout << "You pressed the spacebar." << endl;
    	break;

    case 'x':
    	// take a screenshot
    	saveScreenshot("screenshot.jpg");
    	break;

    //keyboard input '1' for POINTS display
    case '1':
    	displayType = POINTS;
      break;

    //keyboard input '2' for WIREFRAME display
    case '2': 
    	displayType = WIREFRAME;
      break;

    //keyboard input '3' for TRIANGLES display
    case '3': 
    	displayType = TRIANGLES;
      break;

    //keyboard input '4' for OVERLAY display
    //OVERLAY display renders WIREFRAME on top of TRIANGLES
    case '4':
      displayType = OVERLAY;
      break;
      
    //CTRL key not working on Mac so hold 't' and drag mouse to translate
    case 't':
      controlState = TRANSLATE;
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();

  //need to get a handle to pipelineProgram before initVBO because VAO needs program handle
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");
  program = pipelineProgram->GetProgramHandle();

  generateVertices();
  initVBOs();

}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}
