/*
 CSCI 420 Computer Graphics, USC
 Assignment 2: Roller Coaster
 C++ starter code
 
 Student username: mjjacobs
 */

#include <iostream>
#include <cstring>
#include <iomanip>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

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
char windowTitle[512] = "CSCI 420 homework II";

ImageIO * heightmapImage;

OpenGLMatrix * matrix;
GLuint buffer;
BasicPipelineProgram * pipelineProgram;
GLuint program;

int num_screenshots = 0;

const int fovy = 45;
const float aspect = 1.7777777;
const float s = 0.5f;

GLuint vbo_splinePoint_positions;
GLuint vao_spline_points;

glm::mat4 basisMatrix;
glm::mat4x3 controlMatrix_raw;
glm::mat3x4 controlMatrix;

std::vector<glm::vec3> spline_points;
std::vector<glm::vec3> splinePoint_positions;
std::vector<GLfloat> spline_point_uvs;

std::vector<GLfloat> ground_vertices;
std::vector<GLfloat> ground_uvs;

GLuint vbo_ground_vertices;
GLuint vbo_ground_uvs;
GLuint vao_ground_texture;
GLuint ground_texture_handle;

std::vector<GLfloat> sky_vertices;
std::vector<GLfloat> sky_uvs;

GLuint vbo_sky_vertices;
GLuint vbo_sky_uvs;
GLuint vao_sky_texture;
GLuint sky_texture_handle;

GLuint red_texture_handle;
GLuint vbo_spline_point_uvs;

std::vector<glm::vec3> tangent_vecs;
std::vector<glm::vec3> binormal_vecs;
std::vector<glm::vec3> normal_vecs;

std::vector<glm::vec3> left_rail_positions;
std::vector<GLfloat> left_rail_position_uvs;

std::vector<glm::vec3> right_rail_positions;
std::vector<GLfloat> right_rail_position_uvs;

GLuint rail_texture_handle;

GLuint vbo_left_rail_vertices;
GLuint vbo_left_rail_uvs;
GLuint vao_left_rail;

GLuint vbo_right_rail_vertices;
GLuint vbo_right_rail_uvs;
GLuint vao_right_rail;

std::vector<glm::vec3> cross_section_positions;
std::vector<GLfloat> cross_section_position_uvs;

GLuint wood_texture_handle;
GLuint vbo_cross_section_vertices;
GLuint vbo_cross_section_uvs;
GLuint vao_cross_section;

int num_camera_steps = 0;

// represents one control point along the spline
struct Point
{
    double x;
    double y;
    double z;
};

// spline struct
// contains how many control points the spline has, and an array of control points
struct Spline
{
    int numControlPoints;
    Point * points;
};

// the spline array
Spline * splines;
// total number of splines
int numSplines;

int loadSplines(char * argv)
{
    char * cName = (char *) malloc(128 * sizeof(char));
    FILE * fileList;
    FILE * fileSpline;
    int iType, i = 0, j, iLength;
    
    // load the track file
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf ("can't open file\n");
        exit(1);
    }
    
    // stores the number of splines in a global variable
    fscanf(fileList, "%d", &numSplines);
    
    splines = (Spline*) malloc(numSplines * sizeof(Spline));
    
    // reads through the spline files
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL)
        {
            printf ("can't open file\n");
            exit(1);
        }
        
        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        // allocate memory for all the points
        splines[j].points = (Point *)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;
        
        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &splines[j].points[i].x,
                      &splines[j].points[i].y, 
                      &splines[j].points[i].z) != EOF) 
        {
            i++;
        }
    }
    
    free(cName);
    
    return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);
    
    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }
    
    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }
    
    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA
    
    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque
            
            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }
    
    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    
    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);
    
    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);
    
    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }
    
    // de-allocate the pixel array -- it is no longer needed
    delete [] pixelsRGBA;
    
    return 0;
}

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

/*
	calculates the vertices and texture coordinates for the spline.
*/
void generateVertices() 
{
  //get the spline vertices
  for(int i = 0; i < numSplines; i++) {
  	for(int j = 0; j < splines[i].numControlPoints -3; j++) {
		  //get control points
  		Point control1 = splines[i].points[j];
  		Point control2 = splines[i].points[j+1];
  		Point control3 = splines[i].points[j+2];
  		Point control4 = splines[i].points[j+3];

  		//get spline points
  		for(float u = 0.000f; u < 1.0f; u += 0.006f) {
  			
        float v[4] = { pow(u,3), pow(u,2), u, 1 };
  			glm::vec4 uvec = glm::make_vec4(v); //column vector

  			//basis matrix
  			float b[16] = {-1.0f*s, 2.0f-s, s-2.0f,  s,
  							2.0f*s, s-3.0f, 3.0f-2*s,-1*s,
  						   -1.0f*s, 0.0f, 	s, 		 0.0f,
  							0.0f, 	1.0f, 	0.0f, 	 0.0f};

  			basisMatrix = glm::make_mat4(b);

  			//control matrix
  			float c[12] = {control1.x, control1.y, control1.z,
  						   control2.x, control2.y, control2.z,
  						   control3.x, control3.y, control3.z,
  						   control4.x, control4.y, control4.z};

  			controlMatrix_raw = glm::make_mat4x3(c);

  			glm::vec3 splinePoint = controlMatrix_raw * basisMatrix * uvec;

  			spline_points.push_back(splinePoint);
  		}
  	}

    
  	for(int k = 0; k < spline_points.size() - 3; k++) {
  		glm::vec3 point1 = spline_points[k];
  		glm::vec3 point2 = spline_points[k+1];
  		glm::vec3 point3 = spline_points[k+2];
  		glm::vec3 point4 = spline_points[k+3];

  		float u = 0.006f;

  		float b[16] = {-1.0f*s, 2.0f-s, s-2.0f,  s,
  						2.0f*s, s-3.0f, 3.0f-2*s,-1*s,
  					   -1.0f*s, 0.0f, 	s, 		 0.0f,
  						0.0f, 	1.0f, 	0.0f, 	 0.0f};
 		
 		  basisMatrix = glm::make_mat4(b);

  		//control matrix
  		float c[12] = {point1.x, point1.y, point1.z,
  					   point2.x, point2.y, point2.z,
  					   point3.x, point3.y, point3.z,
  					   point4.x, point4.y, point4.z};

  		controlMatrix_raw = glm::make_mat4x3(c);
	
		  //u vec for tangent
      float t[4] = {3 * pow(u,2), 2 * pow(u,1), 1, 0};
      glm::vec4 tvec = glm::make_vec4(t);

      //calculate tangent
      glm::vec3 tangentVec = controlMatrix_raw * basisMatrix * tvec;
      glm::vec3 unitTangentVec = glm::normalize(tangentVec);
      tangent_vecs.push_back(unitTangentVec);

      //Sloan's method
      if(k == 0) {
        float z[3] = {0.0f, 1.0f, 0.0f};
        glm::vec3 y_axis = glm::make_vec3(z);
        //glm::vec3 normalVec = glm::cross(unitTangentVec, point1);
        glm::vec3 normalVec = glm::cross(unitTangentVec, y_axis);
        glm::vec3 unitNormalVec = glm::normalize(normalVec);
        normal_vecs.push_back(unitNormalVec);

        glm::vec3 binormalVec = glm::cross(unitTangentVec, unitNormalVec);
        glm::vec3 unitBinormalVec = glm::normalize(binormalVec);
        binormal_vecs.push_back(unitBinormalVec);
      } else {
        glm::vec3 normalVec = glm::cross(binormal_vecs[k-1], unitTangentVec);
        glm::vec3 unitNormalVec = glm::normalize(normalVec);
        normal_vecs.push_back(unitNormalVec);

        glm::vec3 binormalVec = glm::cross(unitTangentVec, unitNormalVec);
        glm::vec3 unitBinormalVec = glm::normalize(binormalVec);
        binormal_vecs.push_back(unitBinormalVec);
      }
      
      glm::vec3 point = spline_points[k]; 
      glm::vec3 next_point = spline_points[k+3]; 

      float alpha = 0.01f;
      //point 1 triangle coordinates
      //v3
      float bl[3] = {point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
      //v0
      float br[3] = {point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
      //v2
      float tl[3] = {point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
      //v1
      float tr[3] = {point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][1]), point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

      glm::vec3 bl_splinePoint = glm::make_vec3(bl);
      glm::vec3 br_splinePoint = glm::make_vec3(br);
      glm::vec3 tl_splinePoint = glm::make_vec3(tl);
      glm::vec3 tr_splinePoint = glm::make_vec3(tr);

      //triangle 1
      splinePoint_positions.push_back(bl_splinePoint);
      splinePoint_positions.push_back(br_splinePoint);
      splinePoint_positions.push_back(tl_splinePoint);

      //triangle 2
      splinePoint_positions.push_back(tl_splinePoint);
      splinePoint_positions.push_back(br_splinePoint);
      splinePoint_positions.push_back(tr_splinePoint);

      //point 2 triangle coordinates
      //v3
      float next_bl[3] = {next_point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
      //v0
      float next_br[3] = {next_point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
      //v2
      float next_tl[3] = {next_point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
      //v1
      float next_tr[3] = {next_point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][1]), next_point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

      glm::vec3 next_bl_spline_point = glm::make_vec3(next_bl);
      glm::vec3 next_br_spline_point = glm::make_vec3(next_br);
      glm::vec3 next_tl_spline_point = glm::make_vec3(next_tl);
      glm::vec3 next_tr_spline_point = glm::make_vec3(next_tr);      

      //right face triangle one
      splinePoint_positions.push_back(tr_splinePoint);
      splinePoint_positions.push_back(br_splinePoint);
      splinePoint_positions.push_back(next_br_spline_point);

      //right face triangle two
      splinePoint_positions.push_back(tr_splinePoint);
      splinePoint_positions.push_back(next_tr_spline_point);
      splinePoint_positions.push_back(next_br_spline_point);

      //left face triangle one
      splinePoint_positions.push_back(bl_splinePoint);
      splinePoint_positions.push_back(next_br_spline_point);
      splinePoint_positions.push_back(tl_splinePoint);

      //left face triangle two
      splinePoint_positions.push_back(tl_splinePoint);
      splinePoint_positions.push_back(next_bl_spline_point);
      splinePoint_positions.push_back(next_tl_spline_point);

      //top face triangle one
      splinePoint_positions.push_back(tl_splinePoint);
      splinePoint_positions.push_back(next_tl_spline_point);
      splinePoint_positions.push_back(next_tr_spline_point);

      //top face triangle two
      splinePoint_positions.push_back(tl_splinePoint);
      splinePoint_positions.push_back(tr_splinePoint);
      splinePoint_positions.push_back(next_tr_spline_point);

      //bottom face triangle one
      splinePoint_positions.push_back(bl_splinePoint);
      splinePoint_positions.push_back(next_bl_spline_point);
      splinePoint_positions.push_back(next_br_spline_point);

      //bottom face triangle two
      splinePoint_positions.push_back(bl_splinePoint);
      splinePoint_positions.push_back(br_splinePoint);
      splinePoint_positions.push_back(next_br_spline_point);

      GLfloat bl_uv[2] = {0.0f, 0.0f}; //bottom left uv
      GLfloat br_uv[2] = {0.0f, 1.0f}; //bottom right uv
      GLfloat tl_uv[2] = {1.0f, 1.0f}; //top left uv
      GLfloat tr_uv[2] = {1.0f, 0.0f}; //top right uv

      //point 1 triangle 1
      spline_point_uvs.push_back(bl_uv[0]);
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(br_uv[0]);
      spline_point_uvs.push_back(br_uv[1]);
      spline_point_uvs.push_back(tl_uv[0]);
      spline_point_uvs.push_back(tl_uv[1]);

      //point 2 triangle 2
      spline_point_uvs.push_back(tl_uv[0]);
      spline_point_uvs.push_back(tl_uv[1]);
      spline_point_uvs.push_back(br_uv[0]);
      spline_point_uvs.push_back(br_uv[1]);
      spline_point_uvs.push_back(tr_uv[0]);
      spline_point_uvs.push_back(tr_uv[1]);

      //right face triangle one
      spline_point_uvs.push_back(tr_uv[0]); //tr_splinePoint
      spline_point_uvs.push_back(tr_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //br_splinePoint
      spline_point_uvs.push_back(br_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //next_br_spline_point
      spline_point_uvs.push_back(br_uv[1]);

      //right face triangle two
      spline_point_uvs.push_back(tr_uv[0]); //tr_splinePoint
      spline_point_uvs.push_back(tr_uv[1]); 
      spline_point_uvs.push_back(tr_uv[0]); //next_tr_spline_point
      spline_point_uvs.push_back(tr_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //next_br_spline_point
      spline_point_uvs.push_back(br_uv[1]);

      //left face triangle one
      spline_point_uvs.push_back(bl_uv[0]); //point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(bl_uv[0]); //next_point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(tl_uv[0]); //tl_splinePoint
      spline_point_uvs.push_back(tl_uv[1]);

      //left face triangle two
      spline_point_uvs.push_back(tl_uv[0]); //tl_splinePoint
      spline_point_uvs.push_back(tl_uv[1]);
      spline_point_uvs.push_back(bl_uv[0]); //next_point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(tl_uv[0]); //next_tl_spline_point
      spline_point_uvs.push_back(tl_uv[1]); 
      
      //top face triangle one
      spline_point_uvs.push_back(tl_uv[0]); //tl_splinePoint
      spline_point_uvs.push_back(tl_uv[1]);
      spline_point_uvs.push_back(tl_uv[0]); //next_tl_spline_point
      spline_point_uvs.push_back(tl_uv[1]);
      spline_point_uvs.push_back(tr_uv[0]); //next_tr_spline_point
      spline_point_uvs.push_back(tr_uv[1]);

      //top face triangle two
      spline_point_uvs.push_back(tl_uv[0]); //tl_splinePoint
      spline_point_uvs.push_back(tl_uv[1]);
      spline_point_uvs.push_back(tr_uv[0]); //tr_splinePoint
      spline_point_uvs.push_back(tr_uv[1]);
      spline_point_uvs.push_back(tr_uv[0]); //next_tr_spline_point
      spline_point_uvs.push_back(tr_uv[1]);
      
      //bottom face triangle one
      spline_point_uvs.push_back(bl_uv[0]); //point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(bl_uv[0]); //next_point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //next_br_spline_point
      spline_point_uvs.push_back(br_uv[1]);
      
      //bottom face triangle twos
      spline_point_uvs.push_back(bl_uv[0]); //point
      spline_point_uvs.push_back(bl_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //br_splinePoint
      spline_point_uvs.push_back(br_uv[1]);
      spline_point_uvs.push_back(br_uv[0]); //next_br_spline_point
      spline_point_uvs.push_back(br_uv[1]);  
   	}
  }
}

void generateLeftRail()
{
  for(int k = 0; k < spline_points.size() - 10; k++) {
    glm::vec3 left_binormal = binormal_vecs[k];
    left_binormal.x = 0.25f*left_binormal.x;
    left_binormal.y = 0.25f*left_binormal.y;
    left_binormal.z = 0.25f*left_binormal.z;

    glm::vec3 left_binormal2 = binormal_vecs[k+1];
    left_binormal2.x = 0.25*left_binormal2.x;
    left_binormal2.y = 0.25f*left_binormal2.y;
    left_binormal2.z = 0.25f*left_binormal2.z;

    //offset from center to create left rail
    glm::vec3 point = spline_points[k] + left_binormal; 
    glm::vec3 next_point = spline_points[k+1] + left_binormal2;

    float alpha = 0.01f;
    //point 1 triangle coordinates
    //v3
    float bl[3] = {point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
    //v0
    float br[3] = {point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
    //v2
    float tl[3] = {point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
    //v1
    float tr[3] = {point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][0]), point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

    glm::vec3 bl_splinePoint = glm::make_vec3(bl);
    glm::vec3 br_splinePoint = glm::make_vec3(br);
    glm::vec3 tl_splinePoint = glm::make_vec3(tl);
    glm::vec3 tr_splinePoint = glm::make_vec3(tr);

    //triangle 1
    left_rail_positions.push_back(bl_splinePoint);
    left_rail_positions.push_back(br_splinePoint);
    left_rail_positions.push_back(tl_splinePoint);

    //triangle 2
    left_rail_positions.push_back(tl_splinePoint);
    left_rail_positions.push_back(br_splinePoint);
    left_rail_positions.push_back(tr_splinePoint);

    //point 2 triangle coordinates
    //v3
    float next_bl[3] = {next_point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
    //v0
    float next_br[3] = {next_point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
    //v2
    float next_tl[3] = {next_point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
    //v1
    float next_tr[3] = {next_point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][0]), next_point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

    glm::vec3 next_bl_spline_point = glm::make_vec3(next_bl);
    glm::vec3 next_br_spline_point = glm::make_vec3(next_br);
    glm::vec3 next_tl_spline_point = glm::make_vec3(next_tl);
    glm::vec3 next_tr_spline_point = glm::make_vec3(next_tr);      

    //right face triangle one
    left_rail_positions.push_back(tr_splinePoint);
    left_rail_positions.push_back(br_splinePoint);
    left_rail_positions.push_back(next_br_spline_point);

    //right face triangle two
    left_rail_positions.push_back(tr_splinePoint);
    left_rail_positions.push_back(next_tr_spline_point);
    left_rail_positions.push_back(next_br_spline_point);

    //left face triangle one
    left_rail_positions.push_back(bl_splinePoint);
    left_rail_positions.push_back(next_br_spline_point);
    left_rail_positions.push_back(tl_splinePoint);

    //left face triangle two
    left_rail_positions.push_back(tl_splinePoint);
    left_rail_positions.push_back(next_bl_spline_point);
    left_rail_positions.push_back(next_tl_spline_point);

    //top face triangle one
    left_rail_positions.push_back(tl_splinePoint);
    left_rail_positions.push_back(next_tl_spline_point);
    left_rail_positions.push_back(next_tr_spline_point);

    //top face triangle two
    left_rail_positions.push_back(tl_splinePoint);
    left_rail_positions.push_back(tr_splinePoint);
    left_rail_positions.push_back(next_tr_spline_point);

    //bottom face triangle one
    left_rail_positions.push_back(bl_splinePoint);
    left_rail_positions.push_back(next_bl_spline_point);
    left_rail_positions.push_back(next_br_spline_point);

    //bottom face triangle two
    left_rail_positions.push_back(bl_splinePoint);
    left_rail_positions.push_back(br_splinePoint);
    left_rail_positions.push_back(next_br_spline_point);

    GLfloat bl_uv[2] = {0.0f, 0.0f}; //bottom left uv
    GLfloat br_uv[2] = {0.0f, 1.0f}; //bottom right uv
    GLfloat tl_uv[2] = {1.0f, 1.0f}; //top left uv
    GLfloat tr_uv[2] = {1.0f, 0.0f}; //top right uv

    //point 1 triangle 1
    left_rail_position_uvs.push_back(bl_uv[0]);
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]);
    left_rail_position_uvs.push_back(br_uv[1]);
    left_rail_position_uvs.push_back(tl_uv[0]);
    left_rail_position_uvs.push_back(tl_uv[1]);

    //point 2 triangle 2
    left_rail_position_uvs.push_back(tl_uv[0]);
    left_rail_position_uvs.push_back(tl_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]);
    left_rail_position_uvs.push_back(br_uv[1]);
    left_rail_position_uvs.push_back(tr_uv[0]);
    left_rail_position_uvs.push_back(tr_uv[1]);

    //right face triangle one
    left_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    left_rail_position_uvs.push_back(tr_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //br_splinePoint
    left_rail_position_uvs.push_back(br_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    left_rail_position_uvs.push_back(br_uv[1]);

    //right face triangle two
    left_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    left_rail_position_uvs.push_back(tr_uv[1]); 
    left_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    left_rail_position_uvs.push_back(tr_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    left_rail_position_uvs.push_back(br_uv[1]);

    //left face triangle one
    left_rail_position_uvs.push_back(bl_uv[0]); //point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(bl_uv[0]); //next_point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    left_rail_position_uvs.push_back(tl_uv[1]);

    //left face triangle two
    left_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    left_rail_position_uvs.push_back(tl_uv[1]);
    left_rail_position_uvs.push_back(bl_uv[0]); //next_point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(tl_uv[0]); //next_tl_spline_point
    left_rail_position_uvs.push_back(tl_uv[1]); 
    
    //top face triangle one
    left_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    left_rail_position_uvs.push_back(tl_uv[1]);
    left_rail_position_uvs.push_back(tl_uv[0]); //next_tl_spline_point
    left_rail_position_uvs.push_back(tl_uv[1]);
    left_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    left_rail_position_uvs.push_back(tr_uv[1]);

    //top face triangle two
    left_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    left_rail_position_uvs.push_back(tl_uv[1]);
    left_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    left_rail_position_uvs.push_back(tr_uv[1]);
    left_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    left_rail_position_uvs.push_back(tr_uv[1]);
    
    //bottom face triangle one
    left_rail_position_uvs.push_back(bl_uv[0]); //point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(bl_uv[0]); //next_point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    left_rail_position_uvs.push_back(br_uv[1]);
    
    //bottom face triangle twos
    left_rail_position_uvs.push_back(bl_uv[0]); //point
    left_rail_position_uvs.push_back(bl_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //br_splinePoint
    left_rail_position_uvs.push_back(br_uv[1]);
    left_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    left_rail_position_uvs.push_back(br_uv[1]);  
  }
}

void generateRightRail()
{
  for(int k = 0; k < spline_points.size() - 10; k++) {

    glm::vec3 right_binormal = binormal_vecs[k];
    right_binormal.x = 0.25f*right_binormal.x;
    right_binormal.y = 0.25f*right_binormal.y;
    right_binormal.z = 0.25f*right_binormal.z;

    glm::vec3 right_binormal2 = binormal_vecs[k+1];
    right_binormal2.x = 0.25*right_binormal2.x;
    right_binormal2.y = 0.25f*right_binormal2.y;
    right_binormal2.z = 0.25f*right_binormal2.z;

    //offset from center to create right rail
    glm::vec3 point = spline_points[k] - right_binormal; 
    glm::vec3 next_point = spline_points[k+1] - right_binormal2; 

    float alpha = 0.01f;
    //point 1 triangle coordinates
    //v3
    float bl[3] = {point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
    //v0
    float br[3] = {point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
    //v2
    float tl[3] = {point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
    //v1
    float tr[3] = {point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][0]), point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

    glm::vec3 bl_splinePoint = glm::make_vec3(bl);
    glm::vec3 br_splinePoint = glm::make_vec3(br);
    glm::vec3 tl_splinePoint = glm::make_vec3(tl);
    glm::vec3 tr_splinePoint = glm::make_vec3(tr);

    //triangle 1
    right_rail_positions.push_back(bl_splinePoint);
    right_rail_positions.push_back(br_splinePoint);
    right_rail_positions.push_back(tl_splinePoint);

    //triangle 2
    right_rail_positions.push_back(tl_splinePoint);
    right_rail_positions.push_back(br_splinePoint);
    right_rail_positions.push_back(tr_splinePoint);

    //point 2 triangle coordinates
    //v3
    float next_bl[3] = {next_point[0] + alpha*(-normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] - binormal_vecs[k][2])};
    //v0
    float next_br[3] = {next_point[0] + alpha*(-normal_vecs[k][0] + binormal_vecs[k][0]), next_point[1] + alpha*(-normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(-normal_vecs[k][2] + binormal_vecs[k][2])};
    //v2
    float next_tl[3] = {next_point[0] + alpha*(normal_vecs[k][0] - binormal_vecs[k][0]), next_point[1] + alpha*(normal_vecs[k][1] - binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] - binormal_vecs[k][2])};
    //v1
    float next_tr[3] = {next_point[0] + alpha*(normal_vecs[k][0] + binormal_vecs[k][0]), next_point[1] + alpha*(normal_vecs[k][1] + binormal_vecs[k][1]), next_point[2] + alpha*(normal_vecs[k][2] + binormal_vecs[k][2])};

    glm::vec3 next_bl_spline_point = glm::make_vec3(next_bl);
    glm::vec3 next_br_spline_point = glm::make_vec3(next_br);
    glm::vec3 next_tl_spline_point = glm::make_vec3(next_tl);
    glm::vec3 next_tr_spline_point = glm::make_vec3(next_tr);      

    //right face triangle one
    right_rail_positions.push_back(tr_splinePoint);
    right_rail_positions.push_back(br_splinePoint);
    right_rail_positions.push_back(next_br_spline_point);

    //right face triangle two
    right_rail_positions.push_back(tr_splinePoint);
    right_rail_positions.push_back(next_tr_spline_point);
    right_rail_positions.push_back(next_br_spline_point);

    //left face triangle one
    right_rail_positions.push_back(bl_splinePoint);
    right_rail_positions.push_back(next_br_spline_point);
    right_rail_positions.push_back(tl_splinePoint);

    //left face triangle two
    right_rail_positions.push_back(tl_splinePoint);
    right_rail_positions.push_back(next_bl_spline_point);
    right_rail_positions.push_back(next_tl_spline_point);

    //top face triangle one
    right_rail_positions.push_back(tl_splinePoint);
    right_rail_positions.push_back(next_tl_spline_point);
    right_rail_positions.push_back(next_tr_spline_point);

    //top face triangle two
    right_rail_positions.push_back(tl_splinePoint);
    right_rail_positions.push_back(tr_splinePoint);
    right_rail_positions.push_back(next_tr_spline_point);

    //bottom face triangle one
    right_rail_positions.push_back(bl_splinePoint);
    right_rail_positions.push_back(next_bl_spline_point);
    right_rail_positions.push_back(next_br_spline_point);

    //bottom face triangle two
    right_rail_positions.push_back(bl_splinePoint);
    right_rail_positions.push_back(br_splinePoint);
    right_rail_positions.push_back(next_br_spline_point);

    GLfloat bl_uv[2] = {0.0f, 0.0f}; //bottom left uv
    GLfloat br_uv[2] = {0.0f, 1.0f}; //bottom right uv
    GLfloat tl_uv[2] = {1.0f, 1.0f}; //top left uv
    GLfloat tr_uv[2] = {1.0f, 0.0f}; //top right uv

    //point 1 triangle 1
    right_rail_position_uvs.push_back(bl_uv[0]);
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]);
    right_rail_position_uvs.push_back(br_uv[1]);
    right_rail_position_uvs.push_back(tl_uv[0]);
    right_rail_position_uvs.push_back(tl_uv[1]);

    //point 2 triangle 2
    right_rail_position_uvs.push_back(tl_uv[0]);
    right_rail_position_uvs.push_back(tl_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]);
    right_rail_position_uvs.push_back(br_uv[1]);
    right_rail_position_uvs.push_back(tr_uv[0]);
    right_rail_position_uvs.push_back(tr_uv[1]);

    //right face triangle one
    right_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    right_rail_position_uvs.push_back(tr_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //br_splinePoint
    right_rail_position_uvs.push_back(br_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    right_rail_position_uvs.push_back(br_uv[1]);

    //right face triangle two
    right_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    right_rail_position_uvs.push_back(tr_uv[1]); 
    right_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    right_rail_position_uvs.push_back(tr_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    right_rail_position_uvs.push_back(br_uv[1]);

    //left face triangle one
    right_rail_position_uvs.push_back(bl_uv[0]); //point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(bl_uv[0]); //next_point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    right_rail_position_uvs.push_back(tl_uv[1]);

    //left face triangle two
    right_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    right_rail_position_uvs.push_back(tl_uv[1]);
    right_rail_position_uvs.push_back(bl_uv[0]); //next_point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(tl_uv[0]); //next_tl_spline_point
    right_rail_position_uvs.push_back(tl_uv[1]); 
    
    //top face triangle one
    right_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    right_rail_position_uvs.push_back(tl_uv[1]);
    right_rail_position_uvs.push_back(tl_uv[0]); //next_tl_spline_point
    right_rail_position_uvs.push_back(tl_uv[1]);
    right_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    right_rail_position_uvs.push_back(tr_uv[1]);

    //top face triangle two
    right_rail_position_uvs.push_back(tl_uv[0]); //tl_splinePoint
    right_rail_position_uvs.push_back(tl_uv[1]);
    right_rail_position_uvs.push_back(tr_uv[0]); //tr_splinePoint
    right_rail_position_uvs.push_back(tr_uv[1]);
    right_rail_position_uvs.push_back(tr_uv[0]); //next_tr_spline_point
    right_rail_position_uvs.push_back(tr_uv[1]);
    
    //bottom face triangle one
    right_rail_position_uvs.push_back(bl_uv[0]); //point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(bl_uv[0]); //next_point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    right_rail_position_uvs.push_back(br_uv[1]);
    
    //bottom face triangle twos
    right_rail_position_uvs.push_back(bl_uv[0]); //point
    right_rail_position_uvs.push_back(bl_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //br_splinePoint
    right_rail_position_uvs.push_back(br_uv[1]);
    right_rail_position_uvs.push_back(br_uv[0]); //next_br_spline_point
    right_rail_position_uvs.push_back(br_uv[1]);  
  }
  //std::cout << "spline_points.size(): " << spline_points.size() << std::endl;
}

void generateCrossSections()
{
  for(int i=0; i < spline_points.size() - 10; i++) {
    if(i%25 == 0) {
      glm::vec3 left_rail_point = left_rail_positions[i*30];
      glm::vec3 right_rail_point = right_rail_positions[i*30];
      glm::vec3 curr_binormal = binormal_vecs[i];
      glm::vec3 curr_normal = normal_vecs[i];
      glm::vec3 curr_tangent = tangent_vecs[i];

      /*
          the cross-section are wider than the double rails and sit underneath them
          adding the binormal contribution extends their length beyond the rails
          adding the normal contributions places them beneath the rails
          adding the tangent gives them their width
      */
      glm::vec3 bl = left_rail_point + 0.15f*curr_binormal + 0.02f*curr_normal;
      glm::vec3 tl = bl + 0.15f*curr_tangent + 0.02f*curr_normal;
      glm::vec3 br = right_rail_point - 0.15f*curr_binormal + 0.02f*curr_normal;
      glm::vec3 tr = br + 0.15f*curr_tangent + 0.02f*curr_normal;

      //triangle 1
      cross_section_positions.push_back(bl);
      cross_section_positions.push_back(br);
      cross_section_positions.push_back(tl);

      //triangle 2
      cross_section_positions.push_back(tl);
      cross_section_positions.push_back(br);
      cross_section_positions.push_back(tr);


      GLfloat bl_uv[2] = {0.0f, 0.0f}; //bottom left uv
      GLfloat br_uv[2] = {0.0f, 1.0f}; //bottom right uv
      GLfloat tl_uv[2] = {1.0f, 1.0f}; //top left uv
      GLfloat tr_uv[2] = {1.0f, 0.0f}; //top right uv

      //point 1 triangle 1
      cross_section_position_uvs.push_back(bl_uv[0]);
      cross_section_position_uvs.push_back(bl_uv[1]);
      cross_section_position_uvs.push_back(br_uv[0]);
      cross_section_position_uvs.push_back(br_uv[1]);
      cross_section_position_uvs.push_back(tl_uv[0]);
      cross_section_position_uvs.push_back(tl_uv[1]);

      //point 2 triangle 2
      cross_section_position_uvs.push_back(tl_uv[0]);
      cross_section_position_uvs.push_back(tl_uv[1]);
      cross_section_position_uvs.push_back(br_uv[0]);
      cross_section_position_uvs.push_back(br_uv[1]);
      cross_section_position_uvs.push_back(tr_uv[0]);
      cross_section_position_uvs.push_back(tr_uv[1]);
    }
  }
}

void generateGround()
{
  GLfloat bl[3] = {-128, -1, 128};      //bottom left 
  GLfloat br[3] = {128, -1, 128};       //botom right
  GLfloat tl[3] = {-128, -1, -128};     //top left
  GLfloat tr[3] = {128, -1, -128};      //top right

  ground_vertices.push_back(bl[0]);
  ground_vertices.push_back(bl[1]);
  ground_vertices.push_back(bl[2]);
  ground_vertices.push_back(br[0]);
  ground_vertices.push_back(br[1]);
  ground_vertices.push_back(br[2]);
  ground_vertices.push_back(tl[0]);
  ground_vertices.push_back(tl[1]);
  ground_vertices.push_back(tl[2]);

  ground_vertices.push_back(tl[0]);
  ground_vertices.push_back(tl[1]);
  ground_vertices.push_back(tl[2]);
  ground_vertices.push_back(br[0]);
  ground_vertices.push_back(br[1]);
  ground_vertices.push_back(br[2]);
  ground_vertices.push_back(tr[0]);
  ground_vertices.push_back(tr[1]);
  ground_vertices.push_back(tr[2]);

  GLfloat bl_uv[2] = {0.0f, 0.0f};    //bottom left uv
  GLfloat br_uv[2] = {0.0f, 1.0f};    //bottom right uv
  GLfloat tl_uv[2] = {1.0f, 1.0f};    //top left uv
  GLfloat tr_uv[2] = {1.0f, 0.0f};    //top right uv

  ground_uvs.push_back(bl_uv[0]);
  ground_uvs.push_back(bl_uv[1]);
  ground_uvs.push_back(br_uv[0]);
  ground_uvs.push_back(br_uv[1]);
  ground_uvs.push_back(tl_uv[0]);
  ground_uvs.push_back(tl_uv[1]);

  ground_uvs.push_back(tl_uv[0]);
  ground_uvs.push_back(tl_uv[1]);
  ground_uvs.push_back(br_uv[0]);
  ground_uvs.push_back(br_uv[1]);
  ground_uvs.push_back(tr_uv[0]);
  ground_uvs.push_back(tr_uv[1]);

}

void generateSky() 
{
  //back panel of cube
  GLfloat back_bl[3] = {-128, -128, -128};   //bottom left 
  GLfloat back_br[3] = {128, -128, -128};    //bottom right
  GLfloat back_tl[3] = {-128, 128, -128};    //top left
  GLfloat back_tr[3] = {128, 128, -128};     //top right

  sky_vertices.push_back(back_bl[0]);
  sky_vertices.push_back(back_bl[1]);
  sky_vertices.push_back(back_bl[2]);
  sky_vertices.push_back(back_br[0]);
  sky_vertices.push_back(back_br[1]);
  sky_vertices.push_back(back_br[2]);
  sky_vertices.push_back(back_tl[0]);
  sky_vertices.push_back(back_tl[1]);
  sky_vertices.push_back(back_tl[2]);

  sky_vertices.push_back(back_tl[0]);
  sky_vertices.push_back(back_tl[1]);
  sky_vertices.push_back(back_tl[2]);
  sky_vertices.push_back(back_br[0]);
  sky_vertices.push_back(back_br[1]);
  sky_vertices.push_back(back_br[2]);
  sky_vertices.push_back(back_tr[0]);
  sky_vertices.push_back(back_tr[1]);
  sky_vertices.push_back(back_tr[2]);

  GLfloat back_bl_uv[2] = {0.0f, 0.0f};     //bottom left uv
  GLfloat back_br_uv[2] = {0.0f, 1.0f};     //bottom right uv
  GLfloat back_tl_uv[2] = {1.0f, 1.0f};     //top left uv
  GLfloat back_tr_uv[2] = {1.0f, 0.0f};     //top right uv

  sky_uvs.push_back(back_bl_uv[0]);
  sky_uvs.push_back(back_bl_uv[1]);
  sky_uvs.push_back(back_br_uv[0]);
  sky_uvs.push_back(back_br_uv[1]);
  sky_uvs.push_back(back_tl_uv[0]);
  sky_uvs.push_back(back_tl_uv[1]);

  sky_uvs.push_back(back_tl_uv[0]);
  sky_uvs.push_back(back_tl_uv[1]);
  sky_uvs.push_back(back_br_uv[0]);
  sky_uvs.push_back(back_br_uv[1]);
  sky_uvs.push_back(back_tr_uv[0]);
  sky_uvs.push_back(back_tr_uv[1]);

  //right panel of cube
  GLfloat right_bl[3] = {128, -128, 128};
  GLfloat right_br[3] = {128, -128, -128};
  GLfloat right_tl[3] = {128, 128, 128};
  GLfloat right_tr[3] = {128, 128, -128};

  sky_vertices.push_back(right_bl[0]);
  sky_vertices.push_back(right_bl[1]);
  sky_vertices.push_back(right_bl[2]);
  sky_vertices.push_back(right_br[0]);
  sky_vertices.push_back(right_br[1]);
  sky_vertices.push_back(right_br[2]);
  sky_vertices.push_back(right_tl[0]);
  sky_vertices.push_back(right_tl[1]);
  sky_vertices.push_back(right_tl[2]);

  sky_vertices.push_back(right_tl[0]);
  sky_vertices.push_back(right_tl[1]);
  sky_vertices.push_back(right_tl[2]);
  sky_vertices.push_back(right_br[0]);
  sky_vertices.push_back(right_br[1]);
  sky_vertices.push_back(right_br[2]);
  sky_vertices.push_back(right_tr[0]);
  sky_vertices.push_back(right_tr[1]);
  sky_vertices.push_back(right_tr[2]);

  GLfloat right_bl_uv[2] = {0.0f, 0.0f};    //bottom left uv
  GLfloat right_br_uv[2] = {0.0f, 1.0f};    //bottom right uv
  GLfloat right_tl_uv[2] = {1.0f, 1.0f};    //top left uv
  GLfloat right_tr_uv[2] = {1.0f, 0.0f};    //top right uv

  sky_uvs.push_back(right_bl_uv[0]);
  sky_uvs.push_back(right_bl_uv[1]);
  sky_uvs.push_back(right_br_uv[0]);
  sky_uvs.push_back(right_br_uv[1]);
  sky_uvs.push_back(right_tl_uv[0]);
  sky_uvs.push_back(right_tl_uv[1]);

  sky_uvs.push_back(right_tl_uv[0]);
  sky_uvs.push_back(right_tl_uv[1]);
  sky_uvs.push_back(right_br_uv[0]);
  sky_uvs.push_back(right_br_uv[1]);
  sky_uvs.push_back(right_tr_uv[0]);
  sky_uvs.push_back(right_tr_uv[1]);

  //left panel of cube
  GLfloat left_bl[3] = {-128, -128, 128};
  GLfloat left_br[3] = {-128, -128, -128};
  GLfloat left_tl[3] = {-128, 128, 128};
  GLfloat left_tr[3] = {-128, 128, -128};

  sky_vertices.push_back(left_bl[0]);
  sky_vertices.push_back(left_bl[1]);
  sky_vertices.push_back(left_bl[2]);
  sky_vertices.push_back(left_br[0]);
  sky_vertices.push_back(left_br[1]);
  sky_vertices.push_back(left_br[2]);
  sky_vertices.push_back(left_tl[0]);
  sky_vertices.push_back(left_tl[1]);
  sky_vertices.push_back(left_tl[2]);

  sky_vertices.push_back(left_tl[0]);
  sky_vertices.push_back(left_tl[1]);
  sky_vertices.push_back(left_tl[2]);
  sky_vertices.push_back(left_br[0]);
  sky_vertices.push_back(left_br[1]);
  sky_vertices.push_back(left_br[2]);
  sky_vertices.push_back(left_tr[0]);
  sky_vertices.push_back(left_tr[1]);
  sky_vertices.push_back(left_tr[2]);

  GLfloat left_bl_uv[2] = {1.0f, 1.0f};     //bottom left uv
  GLfloat left_br_uv[2] = {1.0f, 0.0f};     //bottom right uv
  GLfloat left_tl_uv[2] = {0.0f, 0.0f};     //top left uv
  GLfloat left_tr_uv[2] = {0.0f, 1.0f};     //top right uv

  sky_uvs.push_back(left_bl_uv[0]);
  sky_uvs.push_back(left_bl_uv[1]);
  sky_uvs.push_back(left_br_uv[0]);
  sky_uvs.push_back(left_br_uv[1]);
  sky_uvs.push_back(left_tl_uv[0]);
  sky_uvs.push_back(left_tl_uv[1]);

  sky_uvs.push_back(left_tl_uv[0]);
  sky_uvs.push_back(left_tl_uv[1]);
  sky_uvs.push_back(left_br_uv[0]);
  sky_uvs.push_back(left_br_uv[1]);
  sky_uvs.push_back(left_tr_uv[0]);
  sky_uvs.push_back(left_tr_uv[1]);

  //front panel of cube
  GLfloat front_bl[3] = {-128, -128, 128};
  GLfloat front_br[3] = {128, -128, 128};
  GLfloat front_tl[3] = {-128, 128, 128};
  GLfloat front_tr[3] = {128, 128, 128};

  sky_vertices.push_back(front_bl[0]);
  sky_vertices.push_back(front_bl[1]);
  sky_vertices.push_back(front_bl[2]);
  sky_vertices.push_back(front_br[0]);
  sky_vertices.push_back(front_br[1]);
  sky_vertices.push_back(front_br[2]);
  sky_vertices.push_back(front_tl[0]);
  sky_vertices.push_back(front_tl[1]);
  sky_vertices.push_back(front_tl[2]);

  sky_vertices.push_back(front_tl[0]);
  sky_vertices.push_back(front_tl[1]);
  sky_vertices.push_back(front_tl[2]);
  sky_vertices.push_back(front_br[0]);
  sky_vertices.push_back(front_br[1]);
  sky_vertices.push_back(front_br[2]);
  sky_vertices.push_back(front_tr[0]);
  sky_vertices.push_back(front_tr[1]);
  sky_vertices.push_back(front_tr[2]);

  GLfloat front_bl_uv[2] = {1.0f, 1.0f};    //bottom left uv
  GLfloat front_br_uv[2] = {1.0f, 0.0f};    //bottom right uv
  GLfloat front_tl_uv[2] = {0.0f, 0.0f};    //top left uv
  GLfloat front_tr_uv[2] = {0.0f, 1.0f};    //top right uv

  sky_uvs.push_back(front_bl_uv[0]);
  sky_uvs.push_back(front_bl_uv[1]);
  sky_uvs.push_back(front_br_uv[0]);
  sky_uvs.push_back(front_br_uv[1]);
  sky_uvs.push_back(front_tl_uv[0]);
  sky_uvs.push_back(front_tl_uv[1]);

  sky_uvs.push_back(front_tl_uv[0]);
  sky_uvs.push_back(front_tl_uv[1]);
  sky_uvs.push_back(front_br_uv[0]);
  sky_uvs.push_back(front_br_uv[1]);
  sky_uvs.push_back(front_tr_uv[0]);
  sky_uvs.push_back(front_tr_uv[1]);

  //top panel of cube
  GLfloat top_bl[3] = {-128, 128, 128};
  GLfloat top_br[3] = {128, 128, 128};
  GLfloat top_tl[3] = {-128, 128, -128};
  GLfloat top_tr[3] = {128, 128, -128};

  sky_vertices.push_back(top_bl[0]);
  sky_vertices.push_back(top_bl[1]);
  sky_vertices.push_back(top_bl[2]);
  sky_vertices.push_back(top_br[0]);
  sky_vertices.push_back(top_br[1]);
  sky_vertices.push_back(top_br[2]);
  sky_vertices.push_back(top_tl[0]);
  sky_vertices.push_back(top_tl[1]);
  sky_vertices.push_back(top_tl[2]);

  sky_vertices.push_back(top_tl[0]);
  sky_vertices.push_back(top_tl[1]);
  sky_vertices.push_back(top_tl[2]);
  sky_vertices.push_back(top_br[0]);
  sky_vertices.push_back(top_br[1]);
  sky_vertices.push_back(top_br[2]);
  sky_vertices.push_back(top_tr[0]);
  sky_vertices.push_back(top_tr[1]);
  sky_vertices.push_back(top_tr[2]);

  GLfloat top_bl_uv[2] = {1.0f, 1.0f};    //bottom left uv
  GLfloat top_br_uv[2] = {1.0f, 0.0f};    //bottom right uv
  GLfloat top_tl_uv[2] = {0.0f, 0.0f};    //top left uv
  GLfloat top_tr_uv[2] = {0.0f, 1.0f};    //top right uv

  sky_uvs.push_back(top_bl_uv[0]);
  sky_uvs.push_back(top_bl_uv[1]);
  sky_uvs.push_back(top_br_uv[0]);
  sky_uvs.push_back(top_br_uv[1]);
  sky_uvs.push_back(top_tl_uv[0]);
  sky_uvs.push_back(top_tl_uv[1]);

  sky_uvs.push_back(top_tl_uv[0]);
  sky_uvs.push_back(top_tl_uv[1]);
  sky_uvs.push_back(top_br_uv[0]);
  sky_uvs.push_back(top_br_uv[1]);
  sky_uvs.push_back(top_tr_uv[0]);
  sky_uvs.push_back(top_tr_uv[1]);
}

void initVBOs()
{
  glGenTextures(1, &red_texture_handle);
  initTexture("textures/red_texture.jpg", red_texture_handle);

  //spline points VAO and VBOs
  glGenVertexArrays(1, &vao_spline_points);
  glBindVertexArray(vao_spline_points); //bind the points VAO

  //upload points position data
  glGenBuffers(1, &vbo_splinePoint_positions);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_splinePoint_positions);
  glBufferData(GL_ARRAY_BUFFER, splinePoint_positions.size() * sizeof(glm::vec3), &splinePoint_positions[0], GL_STATIC_DRAW);
  
  //upload uv data
  glGenBuffers(1, &vbo_spline_point_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_spline_point_uvs);
  glBufferData(GL_ARRAY_BUFFER, spline_point_uvs.size() * sizeof(GLfloat), &spline_point_uvs[0], GL_STATIC_DRAW);
  
  //rail data
  glGenTextures(1, &rail_texture_handle);
  initTexture("textures/shiny_black_metal.jpg", rail_texture_handle);

  //right rail vertices VAO and VBOs
  glGenVertexArrays(1, &vao_left_rail);
  glBindVertexArray(vao_left_rail);

  //upload right rail position data
  glGenBuffers(1, &vbo_left_rail_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_left_rail_vertices);
  glBufferData(GL_ARRAY_BUFFER, left_rail_positions.size() * sizeof(glm::vec3), &left_rail_positions[0], GL_STATIC_DRAW);

  //upload right rail uv data
  glGenBuffers(1, &vbo_left_rail_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_left_rail_uvs);
  glBufferData(GL_ARRAY_BUFFER, left_rail_position_uvs.size() * sizeof(GLfloat), &left_rail_position_uvs[0], GL_STATIC_DRAW);
  
  //right rail vertices VAO and VBOs
  glGenVertexArrays(1, &vao_right_rail);
  glBindVertexArray(vao_right_rail);

  //upload right rail position data
  glGenBuffers(1, &vbo_right_rail_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_right_rail_vertices);
  glBufferData(GL_ARRAY_BUFFER, right_rail_positions.size() * sizeof(glm::vec3), &right_rail_positions[0], GL_STATIC_DRAW);

  //upload right rail uv data
  glGenBuffers(1, &vbo_right_rail_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_right_rail_uvs);
  glBufferData(GL_ARRAY_BUFFER, right_rail_position_uvs.size() * sizeof(GLfloat), &right_rail_position_uvs[0], GL_STATIC_DRAW);

  //cross section data
  glGenTextures(1, &wood_texture_handle);
  initTexture("textures/wood_texture.jpg", wood_texture_handle);

  //cross section vertices VAO and VBOs
  glGenVertexArrays(1, &vao_cross_section);
  glBindVertexArray(vao_cross_section);

  //upload cross section position data
  glGenBuffers(1, &vbo_cross_section_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cross_section_vertices);
  glBufferData(GL_ARRAY_BUFFER, cross_section_positions.size() * sizeof(glm::vec3), &cross_section_positions[0], GL_STATIC_DRAW);

  //upload cross section uv data
  glGenBuffers(1, &vbo_cross_section_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cross_section_uvs);
  glBufferData(GL_ARRAY_BUFFER, cross_section_position_uvs.size() * sizeof(GLfloat), &cross_section_position_uvs[0], GL_STATIC_DRAW);

  //ground
  glGenTextures(1, &ground_texture_handle);
  initTexture("textures/ground.jpg", ground_texture_handle);

  //bind ground texture VAO
  glGenVertexArrays(1, &vao_ground_texture);
  glBindVertexArray(vao_ground_texture);

  //upload vertices data
  glGenBuffers(1, &vbo_ground_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ground_vertices);
  glBufferData(GL_ARRAY_BUFFER, ground_vertices.size() * sizeof(GLfloat), &ground_vertices[0], GL_STATIC_DRAW);

  //upload uv data
  glGenBuffers(1, &vbo_ground_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ground_uvs);
  glBufferData(GL_ARRAY_BUFFER, ground_uvs.size() * sizeof(GLfloat), &ground_uvs[0], GL_STATIC_DRAW);
  
  //bind sky texture VAO
  glGenTextures(1, &sky_texture_handle);
  initTexture("textures/space.jpg", sky_texture_handle);

  //bind sky texture VAO
  glGenVertexArrays(1, &vao_sky_texture);
  glBindVertexArray(vao_sky_texture);

  //upload vertices data
  glGenBuffers(1, &vbo_sky_vertices);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_sky_vertices);
  glBufferData(GL_ARRAY_BUFFER, sky_vertices.size() * sizeof(GLfloat), &sky_vertices[0], GL_STATIC_DRAW);

  //upload uv data
  glGenBuffers(1, &vbo_sky_uvs);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_sky_uvs);
  glBufferData(GL_ARRAY_BUFFER, sky_uvs.size() * sizeof(GLfloat), &sky_uvs[0], GL_STATIC_DRAW);
}

void renderSplines() {
  //position data
  glBindTexture(GL_TEXTURE_2D, red_texture_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_splinePoint_positions); //bind vbo_splinePoint_positions 
  GLuint loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc1); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo_spline_point_uvs);
  GLuint loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Bohan used to debug
  glDrawArrays(GL_TRIANGLES, 0, splinePoint_positions.size()); 

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2); 
}

void renderRails() 
{
  //position data
  glBindTexture(GL_TEXTURE_2D, rail_texture_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_left_rail_vertices); //bind vbo_left_rail_vertices 
  GLuint loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc1); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo_left_rail_uvs);
  GLuint loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_TRIANGLES, 0, left_rail_positions.size()); 

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_right_rail_vertices); //bind vbo_rail_rail_vertices 
  loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc1); //enable the "position" attribute
  offset = (const void*) 0;
  stride = 0;
  normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo_right_rail_uvs);
  loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_TRIANGLES, 0, right_rail_positions.size()); 

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2);
}

void renderCrossSections()
{
  //position data
  glBindTexture(GL_TEXTURE_2D, wood_texture_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cross_section_vertices); //bind vbo_cross_section_vertices 
  GLuint loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variable
  glEnableVertexAttribArray(loc1); //enable the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the “position” attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cross_section_uvs); //bind vbo_cross_section_uvs
  GLuint loc2 = glGetAttribLocation(program, "texCoord"); //get the location of the "texCoord" shader variable
  glEnableVertexAttribArray(loc2);
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);
  glDrawArrays(GL_TRIANGLES, 0, cross_section_positions.size()); 

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2); 
}

void renderGround()
{
  //position data
  glBindTexture(GL_TEXTURE_2D, ground_texture_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ground_vertices); //bind vbo_ground_vertices
  GLuint loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variables
  glEnableVertexAttribArray(loc1); //enalbe the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the "position" attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);

  //texture data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ground_uvs); //bind the vbo_ground_uvs
  GLuint loc2 = glGetAttribLocation(program, "texCoord"); //get the location of the "texCoord" shader variable
  glEnableVertexAttribArray(loc2); //enable the "texCoord" attribute
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_TRIANGLES, 0, ground_vertices.size()/3);

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2);
}

void renderSky()
{
  //position data
  glBindTexture(GL_TEXTURE_2D, sky_texture_handle);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_sky_vertices); //bind vbo_sky_vertices
  GLuint loc1 = glGetAttribLocation(program, "position"); //get the location of the "position" shader variables
  glEnableVertexAttribArray(loc1); //enalbe the "position" attribute
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;
  //set the layout of the "position" attribute data
  glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset);

  //texture data
  glBindBuffer(GL_ARRAY_BUFFER, vbo_sky_uvs); //bind the vbo_sky_uvs
  GLuint loc2 = glGetAttribLocation(program, "texCoord"); //get the location of the "texCoord" shader variable
  glEnableVertexAttribArray(loc2); //enable the "texCoord" attribute
  //set the layout of the "texCoord" attribute data
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);

  glDrawArrays(GL_TRIANGLES, 0, sky_vertices.size()/3); //this is where it segfaults

  glDisableVertexAttribArray(loc1);
  glDisableVertexAttribArray(loc2);
}

void transform()
{
  //set model-view matrix
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();

  glm::vec3 pos = spline_points[num_camera_steps] -  normal_vecs[num_camera_steps] * 0.5f;
  glm::vec3 at = spline_points[num_camera_steps] + tangent_vecs[num_camera_steps];
  glm::vec3 up = - normal_vecs[num_camera_steps];

  //first 3: camera position, next 3: where does the camera look at, last 3: up direction
  matrix->LookAt(pos.x, pos.y, pos.z, at.x, at.y, at.z, up.x, up.y, up.z);

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

  renderSky();
  renderGround();
  renderRails();
  renderCrossSections();
  //for double buffering
  glutSwapBuffers();
}

void idleFunc()
{
  if(num_camera_steps < spline_points.size() - 4) {
    num_camera_steps++;
    if(num_camera_steps%4 == 0) {
      char anim_num[5];
      sprintf(anim_num, "%03d", num_screenshots);
      saveScreenshot(("animation/" + std::string(anim_num) + ".jpg").c_str());
      num_screenshots++;
    }
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
      case 'p':
      num_camera_steps += 10;
      break;
 case 'o':
      num_camera_steps -= 10;
      break;
  }
}

void initScene(int argc, char *argv[])
{

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();

  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");
  program = pipelineProgram->GetProgramHandle();

  generateVertices();
  generateSky();
  generateGround();
  generateLeftRail();
  generateRightRail();
  generateCrossSections();
  initVBOs();

  glEnable(GL_LINE_SMOOTH);
}

int main(int argc, char *argv[])
{

  if (argc<2)
  {
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }
    
  // load the splines from the provided filename
  loadSplines(argv[1]);
    
  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
    
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
    
  return 0;
}
