/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Marshall Jacobs
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include <vector>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;
bool useSoftShadows = false;
bool antialiasing = false;

#define WIDTH 640
#define HEIGHT 480
#define ASPECT_RATIO 640.0/480.0
//the field of view of the camera
#define fov 60.0

#define PI 3.1415926535
#define ALPHA std::tan((fov/2.0) * (PI/180.0))

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

struct Vec3
{
  double x;
  double y;
  double z;

  // default constructor 
  Vec3() {}

  // parameterized constructor
  Vec3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

  // assignment operator modifies object, therefore non-const
  Vec3& operator=(const Vec3& a) {
    x = a.x;
    y = a.y;
    z = a.z;
    return *this;
  }

  // addop. doesn't modify object. therefore const.
  Vec3 operator+(const Vec3& a) const {
    return Vec3(a.x + x, a.y + y , a.z + z);
  }

  // subtract op doesn't modify object. therefore const.
  Vec3 operator-(const Vec3& a) const {
  	return Vec3(x - a.x, y - a.y, z - a.z);
  }

  // negate op takes no parameters
  Vec3 operator-() const {
    return Vec3(-x, -y, -z);
  }

  Vec3 operator*(double a) const {
    return Vec3(x * a, y * a, z * a);
  }

  // equality comparison. doesn't modify object. therefore const.
  bool operator==(const Vec3& a) const {
    return (x == a.x && y == a.y && z == a.z);
  }

};

struct Color 
{
  double r;
  double g;
  double b;

  // default constructor
  Color() {}

  Color(double _r, double _g, double _b) : r(_r), g(_g), b(_b) {}

  //clamps to 1 or 0 if r, g, or b value exceeds 1 or is less than 0
  Color& operator += (const Color& a) {
    r += a.r;
    if (r > 1.0) {
      r = 1.0;
    } else if (r < 0.0) {
      r = 0.0;
    }

    g += a.g;
    if (g > 1.0) {
      g = 1.0;
    } else if (g < 0.0) {
      g = 0.0;
    }

    b += a.b;
    if (b > 1.0) {
      b = 1.0;
    } else if (b < 0.0) {
      b = 0.0;
    }
    return *this;
  }
};

//calculates and returns the unit normal of v1
Vec3 normalize(Vec3 v1)
{
  double denom = std::pow(v1.x, 2) + std::pow(v1.y, 2) + std::pow(v1.z, 2);

  //only normalize if sqrt term > 0 (must be > 0 not >= 0 because can't divide by 0)
  if (denom > 0) {
    double normalizingConstant = 1.0 / std::sqrt(denom);
    v1.x *= normalizingConstant;
    v1.y *= normalizingConstant;
    v1.z *= normalizingConstant;
  }
  return v1;
}

//calculates and returns the cross product of v1 and v2
Vec3 cross(Vec3 v1, Vec3 v2) 
{
  Vec3 result;
  result.x = v1.y * v2.z - v1.z * v2.y;
  result.y = v1.z * v2.x - v1.x * v2.z;
  result.z = v1.x * v2.y - v1.y * v2.x;
  return result;
}

//calculates and returns the dot product of v1 and v2
double dot(Vec3 v1, Vec3 v2)
{
  return ( (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) );
}

//calculates and returns the length of v1
double length(Vec3 v1) 
{
  return (std::pow(v1.x, 2) + std::pow(v1.y, 2) + std::pow(v1.z, 2));
}

class Ray {
  private:
    Vec3 origin;
    Vec3 direction;

  public:
    Ray() {}
    Ray(Vec3 _origin, Vec3 _direction) : origin(_origin), direction(_direction) {}

    //calculates if this ray intersects sphere. If so, returns true and 
    //intersection is assigned the location of this intersection
    bool intersects(const Sphere& sphere, Vec3& intersection) 
    {
      double t0, t1, tf; //tf = t final

      Vec3 position(sphere.position[0], sphere.position[1], sphere.position[2]);
      Vec3 distance = origin - position;

      double b = 2 * dot(direction, distance);
      double c = dot(distance, distance) - std::pow(sphere.radius, 2);

      double discriminant = std::pow(b, 2) - 4 * c;

      //if discriminant is less than 0, then there is no intersection (imaginary roots)
      if (discriminant < 0) {
        return false;
      }

      //if the discriminant = 0, then repeated real roots
      else if (discriminant == 0) {
        double root = -b / 2.0;
        t0 = root;
        t1 = root;
      }

      //two real roots
      else {
        t0 = ( -b + std::sqrt(discriminant) ) / 2.0;
        t1 = ( -b - std::sqrt(discriminant) ) / 2.0;
      }

      //if both roots are negative, then no intersection
      if(t0 < 0 && t1 < 0) {
        return false;
      }  

      //store the smaller positive root in tf 
      if(t0 > t1 && t1 > 0) {
        tf = t1;
      }

      else if (t1 > t0 && t0 > 0) {
        tf = t0;
      }

      else {
        tf = t0;
      }

      intersection = origin + (direction * tf);
      return true;

    }

    //calculates if this ray intersects triangle. If so, intersects returns true
    //true and intersection is assigned the location of this intersection
    bool intersects(const Triangle& triangle, Vec3& intersection)
    {
      Vec3 p1, p2;

      //calculate interpolated normal of 3 triangle vertices
      Vec3 vertexA = Vec3(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
      Vec3 vertexB = Vec3(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
      Vec3 vertexC = Vec3(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);

      Vec3 normal = cross(vertexB - vertexA, vertexC - vertexA); //formula for calculating interpolated normal
      Vec3 unitNormal = normalize(normal); //must normalize interpolated normal

      //calculate d from implicit form of plane equation: ax + by + cz + d = 0
      float d = dot(unitNormal, - vertexA);

      //ensure ray is not parallel to plane (need it to be less than an error term not 0 because of double floating point precision)
      if( std::abs(dot(unitNormal, direction)) < 1e-6) {
        return false;
      }

      //calculate t
      //double t = -( dot(unitNormal, origin) + d) / ( dot(normal, direction) );
      double t = -( dot((origin - vertexA), unitNormal)/ (dot(unitNormal, direction)) );

      if (t <= 0.001) {
        return false;
      }

      intersection = origin + direction * t;

      //CITE: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
      //inside out test
      p1 = vertexB - vertexA;
      p2 = intersection - vertexA;
      if ( dot( cross(p1, p2), normal) < 0 ) {
        return false;
      } 

      p1 = vertexC - vertexB;
      p2 = intersection - vertexB;

      if ( dot( cross(p1, p2), normal) < 0) {
        return false;
      }

      p1 = vertexA - vertexC;
      p2 = intersection - vertexC;

      if ( dot( cross(p1, p2), normal) < 0) {
        return false;
      }

      //this ray hits the triangle 
      return true;
    }
  };

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

//CITE: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays
Ray generateRay(double x, double y)
{
  double pixel_NDC_x = (x+0.5)/WIDTH;
  double pixel_NDC_y = (y+0.5)/HEIGHT;

  double pixel_screen_x = 2 * pixel_NDC_x -1; //Bohan said to subtract 1 for proper bounds
  double pixel_screen_y = 2 * pixel_NDC_y -1; //Bohan said to subtract 1 for proper bounds

  double pixel_camera_x = pixel_screen_x * ASPECT_RATIO * ALPHA;
  double pixel_camera_y = pixel_screen_y * ALPHA;

  Vec3 origin(0.0, 0.0, 0.0);
  Vec3 direction = normalize(Vec3(pixel_camera_x, pixel_camera_y, -1.0));

  return Ray(origin, direction);
}

//overloaded version of generateRay used for antialiasing
//calculates for rays per pixel as opposed to 1
std::vector<Ray> generateRay(double x, double y, bool antialiasing) {
  std::vector<Ray> rays;

  //r1
  double pixel_NDC_x = (x+0.25)/WIDTH;
  double pixel_NDC_y = (y+0.25)/HEIGHT;

  double pixel_screen_x = 2 * pixel_NDC_x -1; //Bohan said to subtract 1 for proper bounds
  double pixel_screen_y = 2 * pixel_NDC_y -1; //Bohan said to subtract 1 for proper bounds

  double pixel_camera_x = pixel_screen_x * ASPECT_RATIO * ALPHA;
  double pixel_camera_y = pixel_screen_y * ALPHA;

  Vec3 origin(0.0, 0.0, 0.0);
  //Vec3 direction = normalize(Vec3(pixel_screen_x, pixel_camera_y, -1.0));
  Vec3 direction = normalize(Vec3(pixel_camera_x, pixel_camera_y, -1.0));
  Ray r1(origin, direction);

  rays.push_back(r1);

  //r2
  pixel_NDC_x = (x+0.75)/WIDTH;
  pixel_NDC_y = (y+0.25)/HEIGHT;

  pixel_screen_x = 2 * pixel_NDC_x -1; //Bohan said to subtract 1 for proper bounds
  pixel_screen_y = 2 * pixel_NDC_y -1; //Bohan said to subtract 1 for proper bounds

  pixel_camera_x = pixel_screen_x * ASPECT_RATIO * ALPHA;
  pixel_camera_y = pixel_screen_y * ALPHA;

  direction = normalize(Vec3(pixel_camera_x, pixel_camera_y, -1.0));
  Ray r2(origin, direction);  

  rays.push_back(r2);

  //r3
  pixel_NDC_x = (x+0.25)/WIDTH;
  pixel_NDC_y = (y+0.75)/HEIGHT;

  pixel_screen_x = 2 * pixel_NDC_x -1; //Bohan said to subtract 1 for proper bounds
  pixel_screen_y = 2 * pixel_NDC_y -1; //Bohan said to subtract 1 for proper bounds

  pixel_camera_x = pixel_screen_x * ASPECT_RATIO * ALPHA;
  pixel_camera_y = pixel_screen_y * ALPHA;

  direction = normalize(Vec3(pixel_camera_x, pixel_camera_y, -1.0));
  Ray r3(origin, direction);  

  rays.push_back(r3);

  //r4
  pixel_NDC_x = (x+0.75)/WIDTH;
  pixel_NDC_y = (y+0.75)/HEIGHT;

  pixel_screen_x = 2 * pixel_NDC_x -1; //Bohan said to subtract 1 for proper bounds
  pixel_screen_y = 2 * pixel_NDC_y -1; //Bohan said to subtract 1 for proper bounds

  pixel_camera_x = pixel_screen_x * ASPECT_RATIO * ALPHA;
  pixel_camera_y = pixel_screen_y * ALPHA;

  direction = normalize(Vec3(pixel_camera_x, pixel_camera_y, -1.0));
  Ray r4(origin, direction);  

  rays.push_back(r4);

  return rays;
}

/*
    Calculates the contribution of light to the pixel on sphere according to Phong shading
*/
Color phongShading(Light light, Sphere sphere, Vec3 intersection) {
  Vec3 normal = intersection - Vec3(sphere.position[0], sphere.position[1], sphere.position[2]);
  Vec3 unitNormal = normalize(normal);

  Vec3 lightPosition(light.position[0], light.position[1], light.position[2]);
  Vec3 lightDirection = lightPosition - intersection;
  Vec3 unitLightDirection = normalize(lightDirection);

  //calculate light magnitude and reflection magnitude
  double lightMagnitude = dot(unitLightDirection, unitNormal);
  //clamp light magnitude if > 1 or < 0
  if (lightMagnitude > 1.0) {
    lightMagnitude = 1.0;
  } else if (lightMagnitude < 0.0) {
    lightMagnitude = 0.0;
  }

  Vec3 reflectionMagnitude(2 * lightMagnitude * unitNormal.x - unitLightDirection.x, 2 * lightMagnitude * unitNormal.y - unitLightDirection.y, 2 * lightMagnitude * unitNormal.z - unitLightDirection.z);
  Vec3 unitReflectionMagnitude = normalize(reflectionMagnitude);
  Vec3 direction = -intersection;
  Vec3 unitDirection = normalize(direction);
  double reflection = dot(unitReflectionMagnitude, unitDirection);

  if(reflection > 1.0) {
    reflection = 1.0;
  } else if (reflection < 0.0) {
    reflection = 0.0;
  }

  double r = light.color[0] * (sphere.color_diffuse[0] * lightMagnitude + (sphere.color_specular[0] * std::pow(reflection, sphere.shininess)) );
  double g = light.color[1] * (sphere.color_diffuse[1] * lightMagnitude + (sphere.color_specular[1] * std::pow(reflection, sphere.shininess)) );
  double b = light.color[2] * (sphere.color_diffuse[2] * lightMagnitude + (sphere.color_specular[2] * std::pow(reflection, sphere.shininess)) );

  return Color(r, g, b);
}

/*
    Calculates the contribution of light to the pixel on triangle according to Phong shading
*/
Color phongShading(Light light, Triangle triangle, Vec3 intersection) {

  Vec3 vertexA(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
  Vec3 vertexB(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
  Vec3 vertexC(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);

  //calculate barycentric coordinates 
  Vec3 planeNormal = cross( (vertexB - vertexA), (vertexC - vertexA) );
  double alpha = dot(planeNormal, cross(vertexC-vertexB, intersection-vertexB)) / dot(planeNormal, planeNormal);
  double beta = dot(planeNormal, cross(vertexA-vertexC, intersection-vertexC)) / dot(planeNormal, planeNormal);
  double gamma = 1.0 - alpha - beta;

  //interpolate vertex normals to calculate triangle normals (weighted using barycentric coordinates)
  Vec3 triangleNormal(alpha * triangle.v[0].normal[0] + beta * triangle.v[1].normal[0] + gamma * triangle.v[2].normal[0],
  					  alpha * triangle.v[0].normal[1] + beta * triangle.v[1].normal[1] + gamma * triangle.v[2].normal[1],
  					  alpha * triangle.v[0].normal[2] + beta * triangle.v[1].normal[2] + gamma * triangle.v[2].normal[2]);
  Vec3 unitTriangleNormal = normalize(triangleNormal);

  Vec3 lightPosition(light.position[0], light.position[1], light.position[2]);
  Vec3 lightDirection = lightPosition - intersection;
  Vec3 unitLightDirection = normalize(lightDirection);

  //calculate light magnitude and reflection magnitude
  double lightMagnitude = dot(unitLightDirection, unitTriangleNormal);
  //clamp light magnitude if > 1 or < 0
  if (lightMagnitude > 1.0) {
    lightMagnitude = 1.0;
  } else if (lightMagnitude < 0.0) {
    lightMagnitude = 0.0;
  }

  Vec3 reflectionMagnitude(2 * lightMagnitude * unitTriangleNormal.x - unitLightDirection.x, 
  						   2 * lightMagnitude * unitTriangleNormal.y - unitLightDirection.y, 
  						   2 * lightMagnitude * unitTriangleNormal.z - unitLightDirection.z);
  Vec3 unitReflectionMagnitude = normalize(reflectionMagnitude);
  Vec3 direction = -intersection;
  Vec3 unitDirection = normalize(direction);
  double reflection = dot(unitReflectionMagnitude, unitDirection);

  //clamp reflection if > 1 or < 0
  if(reflection > 1.0) {
    reflection = 1.0;
  } else if (reflection < 0.0) {
    reflection = 0.0;
  }

  //interpolate diffuse, specular, and shininess of vertices of triangle using barycentric coordinates
  Color diffuse(alpha * triangle.v[0].color_diffuse[0] + beta * triangle.v[1].color_diffuse[0] + gamma * triangle.v[2].color_diffuse[0],
  				alpha * triangle.v[0].color_diffuse[1] + beta * triangle.v[1].color_diffuse[1] + gamma * triangle.v[2].color_diffuse[1],
  				alpha * triangle.v[0].color_diffuse[2] + beta * triangle.v[1].color_diffuse[2] + gamma * triangle.v[2].color_diffuse[2]);

  Color specular(alpha * triangle.v[0].color_specular[0] + beta * triangle.v[1].color_specular[0] + gamma * triangle.v[2].color_specular[0],
  				 alpha * triangle.v[0].color_specular[1] + beta * triangle.v[1].color_specular[1] + gamma * triangle.v[2].color_specular[1],
  				 alpha * triangle.v[0].color_specular[2] + beta * triangle.v[1].color_specular[2] + gamma * triangle.v[2].color_specular[2]);

  double shininess = alpha * triangle.v[0].shininess + beta * triangle.v[1].shininess + gamma * triangle.v[2].shininess;

  double r = light.color[0] * (diffuse.r * lightMagnitude + (specular.r * std::pow(reflection, shininess)) );
  double g = light.color[1] * (diffuse.g * lightMagnitude + (specular.g * std::pow(reflection, shininess)) );
  double b = light.color[2] * (diffuse.b * lightMagnitude + (specular.b * std::pow(reflection, shininess)) );

  return Color(r, g, b);
}

std::vector<Light> sampleLight(Light light) {
  std::vector<Light> lightSamples;
  double sumR = 0.0;
  for (int i=0; i < 48; i++) {
  	//randomly sample r (r adjusted from [0,1] to [0, .1] )
  	double radius = ((double) rand() / (RAND_MAX)) / 10.0;
  	//randomly sample theta
  	double theta = ((double) (rand() % 360) * PI/180.0 );
  	//randomly sample phi
  	double phi = ((double) (rand() % 360) * PI/180.0);

  	double xOffset = radius * std::sin(theta) * std::cos(phi);
  	double yOffset = radius * std::sin(theta) * std::sin(phi);
  	double zOffset = radius * std::cos(theta);
  	
    //calculate random offsets = rsin(theta)cos(phi), rsin(theta)sin(phi), rcos(theta)
  	//calculate random point Pi = (light.position[0] + xOffset, light.position[1] + yOffset, light.position[2] + zOffset)
  	double xPos = light.position[0] + xOffset;
  	double yPos = light.position[1] + yOffset;
  	double zPos = light.position[2] + zOffset;

    //normalize the contribution from each sampled light
  	double r = light.color[0] / 48.0;
  	double g = light.color[1] / 48.0;
  	double b = light.color[2] / 48.0;

  	//create a light with the above position and color/intensity and insert into lightSamples
  	Light randomLight;
  	randomLight.position[0] = xPos;
  	randomLight.position[1] = yPos;
  	randomLight.position[2] = zPos;
  	randomLight.color[0] = r;
  	randomLight.color[1] = g;
  	randomLight.color[2] = b;
  	lightSamples.push_back(randomLight);
  }

  return lightSamples;
}

//traces ray
Color trace(Ray& ray) {

  double closestIntersectionZ = -1e10;
  Color ambient(ambient_light[0], ambient_light[1], ambient_light[2]);
  Color pixelColor(1.0, 1.0, 1.0);

  for(int i=0; i < num_spheres; i++) {
    Vec3 intersection(0,0,-1e-10); //camera at 0,0,0 so need to move slightly off origin (in -z direction)
    if( ray.intersects(spheres[i], intersection) && intersection.z > closestIntersectionZ) {
      //if shadow ray can reach light source without hitting object then do phong shading
      //otherwise make it black
      pixelColor = Color(0.0, 0.0, 0.0);

      for(int j=0; j < num_lights; j++) {
        Vec3 lightPosition(lights[j].position[0], lights[j].position[1], lights[j].position[2]);

        Vec3 direction = lightPosition - intersection;
        Vec3 unitDirection = normalize(direction);

        //create shadow ray from point of intersection in direction of light source j
        Ray shadow = Ray(intersection, unitDirection);

        bool isCoveredByShadow = false;

        //check spheres
        for(int k=0; k < num_spheres; k++) {
          Vec3 shadowIntersection(0, 0, -1e-10);
          //check for spheres in between light source and current sphere not including the current sphere
          if( (i != k) && shadow.intersects(spheres[k], shadowIntersection) ){
			    // Make sure that the intersection point is not past the light
			      Vec3 distToSphere = shadowIntersection - intersection;
			      Vec3 distToLight = lightPosition - intersection;
			      if ( length(distToSphere) < length(distToLight) ) {
			        isCoveredByShadow = true;
			        break;
			      }
          }
        }

        //check triangles
        for(int k=0; k < num_triangles; k++) {
        	Vec3 shadowIntersection(0, 0, -1e-10);
        	//check for triangles in between light source and current sphere
        	if( shadow.intersects(triangles[k], shadowIntersection) ) {
        	  //make sure that the intersection point is not past the light
        	  Vec3 distToTriangle = shadowIntersection - intersection;
			      Vec3 distToLight = lightPosition - intersection;
			      if ( length(distToTriangle) < length(distToLight) ) {
			        isCoveredByShadow = true;
				      break;
			      }
        	}
        }

        if (!isCoveredByShadow) {
          pixelColor += phongShading(lights[j], spheres[i], intersection);
        }
      }

      //update closest intersection to current intersection
      closestIntersectionZ = intersection.z;
    }
  }

  for(int i=0; i < num_triangles; i++) {
    Vec3 intersection(0,0,-1e-10); //camera at 0,0,0 so need ot move slightly off origin (in -z direction)
    if( ray.intersects(triangles[i], intersection) && intersection.z > closestIntersectionZ) { 
      //if shadow ray can reach light source without hitting object then do phong shading
      //otherwise make it black
      pixelColor = Color(0.0, 0.0, 0.0);

      for(int j=0; j < num_lights; j++) {
        Vec3 lightPosition(lights[j].position[0], lights[j].position[1], lights[j].position[2]);

        Vec3 direction = lightPosition - intersection;
        Vec3 unitDirection = normalize(direction);

        //create shadow ray from point of intersection in direction of light source j
        Ray shadow = Ray(intersection, unitDirection);

        bool isCoveredByShadow = false;

        //check spheres
        for(int k=0; k < num_spheres; k++) {
          Vec3 shadowIntersection(0, 0, -1e-10);
          //check for spheres in between light source and current sphere not including the current sphere
          if( shadow.intersects(spheres[k], shadowIntersection) ){
            //make sure that the intersection point is not past the light
            Vec3 distToSphere = shadowIntersection - intersection;
			      Vec3 distToLight = lightPosition - intersection;
			      if ( length(distToSphere) < length(distToLight) ) {
			        isCoveredByShadow = true;
			        break;
			      }
          }
        }

        //check triangles
        for(int k=0; k < num_triangles; k++) {
        	Vec3 shadowIntersection(0, 0, -1e-10);
        	//check for triangles in between light source and current sphere
        	if( (i != k) && shadow.intersects(triangles[k], shadowIntersection) ) {
        	  //make sure that the intersection point is not past the light?
        	  Vec3 distToTriangle = shadowIntersection - intersection;
			      Vec3 distToLight = lightPosition - intersection;
			      if ( length(distToTriangle) < length(distToLight) ) {
			        isCoveredByShadow = true;
				      break;
			      }
        	}
        }

        if (!isCoveredByShadow) {
          pixelColor += phongShading(lights[j], triangles[i], intersection);
        }
      }

      //update closest intersection to current intersection
      closestIntersectionZ = intersection.z;
    }
  }

  //add ambient color
  pixelColor += ambient;

  return pixelColor;
}

/* 
    overloaded version of trace for soft shadows
*/
Color trace(Ray& ray, bool softShadows) {

  double closestIntersectionZ = -1e10;
  Color ambient(ambient_light[0], ambient_light[1], ambient_light[2]);
  Color pixelColor(1.0, 1.0, 1.0);

  for(int i=0; i < num_spheres; i++) {
    Vec3 intersection(0,0,-1e-10); //camera at 0,0,0 so need to move slightly off origin (in -z direction)
    if( ray.intersects(spheres[i], intersection) && intersection.z > closestIntersectionZ) {
      //if shadow ray can reach light source without hitting object then do phong shading
      //otherwise make it black
      pixelColor = Color(0.0, 0.0, 0.0);

      for(int j=0; j < num_lights; j++) {
      	//call sample light
      	std::vector<Light> samples = sampleLight(lights[j]);
      	//put all the code below in a for loop
      	for(int l=0; l < samples.size(); l++) {
	        Vec3 lightPosition(samples[l].position[0], samples[l].position[1], samples[l].position[2]);

	        Vec3 direction = lightPosition - intersection;
	        Vec3 unitDirection = normalize(direction);

	        //create shadow ray from point of intersection in direction of light source j
	        Ray shadow = Ray(intersection, unitDirection);

	        bool isCoveredByShadow = false;

	        //check spheres
	        for(int k=0; k < num_spheres; k++) {
	          Vec3 shadowIntersection(0, 0, -1e-10);
	          //check for spheres in between light source and current sphere not including the current sphere
	          if( (i != k) && shadow.intersects(spheres[k], shadowIntersection) ){
				      // Make sure that the intersection point is not past the light?
				      Vec3 distToSphere = shadowIntersection - intersection;
				      Vec3 distToLight = lightPosition - intersection;
				      if ( length(distToSphere) < length(distToLight) ) {
				        isCoveredByShadow = true;
				        break;
				      }
	          }
        	}

	        //check triangles
	        for(int k=0; k < num_triangles; k++) {
	        	Vec3 shadowIntersection(0, 0, -1e-10);
	        	//check for triangles in between light source and current sphere
	        	if( shadow.intersects(triangles[k], shadowIntersection) ) {
	        	  //make sure that the intersection point is not past the light?
	        	  Vec3 distToTriangle = shadowIntersection - intersection;
				      Vec3 distToLight = lightPosition - intersection;
				      if ( length(distToTriangle) < length(distToLight) ) {
				        isCoveredByShadow = true;
					      break;
				      }
	        	}
	        }

	        if (!isCoveredByShadow) {
	          pixelColor += phongShading(samples[l], spheres[i], intersection);
	        }
    	  }
      }

      //update closest intersection to current intersection
      closestIntersectionZ = intersection.z;
    }
  }

  for(int i=0; i < num_triangles; i++) {
    Vec3 intersection(0,0,-1e-10); //camera at 0,0,0 so need ot move slightly off origin (in -z direction)
    if( ray.intersects(triangles[i], intersection) && intersection.z > closestIntersectionZ) { 
      //if shadow ray can reach light source without hitting object then do phong shading
      //otherwise make it black
      pixelColor = Color(0.0, 0.0, 0.0);

      for(int j=0; j < num_lights; j++) {
      	//call sample light
      	std::vector<Light> samples = sampleLight(lights[j]);
      	for(int l=0; l < samples.size(); l++) {

	      	//put all the code below in a for loop
	        Vec3 lightPosition(samples[l].position[0], samples[l].position[1], samples[l].position[2]);

	        Vec3 direction = lightPosition - intersection;
	        Vec3 unitDirection = normalize(direction);

	        //create shadow ray from point of intersection in direction of light source j
	        Ray shadow = Ray(intersection, unitDirection);

	        bool isCoveredByShadow = false;

	        //check spheres
	        for(int k=0; k < num_spheres; k++) {
	          Vec3 shadowIntersection(0, 0, -1e-10);
	          //check for spheres in between light source and current sphere not including the current sphere
	          if( shadow.intersects(spheres[k], shadowIntersection) ){
	            //make sure that the intersection point is not past the light?
	            Vec3 distToSphere = shadowIntersection - intersection;
				      Vec3 distToLight = lightPosition - intersection;
				      if ( length(distToSphere) < length(distToLight) ) {
				        isCoveredByShadow = true;
				        break;
				      }
	          }
	        }

	        //check triangles
	        for(int k=0; k < num_triangles; k++) {
	        	Vec3 shadowIntersection(0, 0, -1e-10);
	        	//check for triangles in between light source and current sphere
	        	if( (i != k) && shadow.intersects(triangles[k], shadowIntersection) ) {
	        	  //make sure that the intersection point is not past the light?
	        	  Vec3 distToTriangle = shadowIntersection - intersection;
				      Vec3 distToLight = lightPosition - intersection;
				      if ( length(distToTriangle) < length(distToLight) ) {
				        isCoveredByShadow = true;
					      break;
				      }
	        	}
	        }

	        if (!isCoveredByShadow) {
	          pixelColor += phongShading(samples[l], triangles[i], intersection);
	        }
	      }
      }

      //update closest intersection to current intersection
      closestIntersectionZ = intersection.z;
    }
  }

  //add ambient color
  pixelColor += ambient;

  return pixelColor;
}

//traces the scene file according to command line options
void draw_scene()
{
  bool isDebugPixel = false;
  //a simple test output
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      if (antialiasing) {
      	std::vector<Ray> rays = generateRay(x, y, true);
        double r = 0;
        double g = 0;
        double b = 0;
        for(int i=0; i < 4; i++) {
          Color color;
          if (useSoftShadows) {
      	  	color = trace(rays[i], useSoftShadows);
      	  } else {
      	  	color = trace(rays[i]);
      	  }
      	  r += color.r;
      	  g += color.g;
      	  b += color.b;
        }
        r /= 4.0;
        b /= 4.0;
        g /= 4.0;
        plot_pixel(x, y, r * 255, g * 255, b * 255);
      } else {
      	Color color;
      	Ray ray = generateRay(x, y);
      	if (useSoftShadows) {
      		color = trace(ray, useSoftShadows);
      	} else {
      		color = trace(ray);
      	}
      	plot_pixel(x, y, color.r * 255, color.g * 255, color.b * 255);
      }
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch(key) 
  {
    case 27:
	  exit(0);
	  break;
  }
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 5))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  if(argc == 4) {
  	mode = MODE_JPEG;
  	filename = argv[2];
  	char* ss = argv[3];
  	if (*ss == 'y' || *ss == 'Y') {
  		useSoftShadows = true;
  		std::cout << "using softShadows" << std::endl;
  	}
  }

  if(argc == 5) {
  	mode = MODE_JPEG;
  	filename = argv[2];
  	char* ss = argv[3];
  	char* aa = argv[4];
  	if(*ss == 'y' || *ss == 'Y') {
  		useSoftShadows = true;
  		std::cout << "using soft shadows" << std::endl;
  	}
  	if(*aa == 'y' || *aa == 'Y') {
  		antialiasing = true;
  		std::cout << "using antialiasing" << std::endl;
  	} 
  }
  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboardFunc);
  init();
  glutMainLoop();
}

