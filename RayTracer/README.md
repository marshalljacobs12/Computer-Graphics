# Ray Tracing

### Description
 In this assignment, I built a ray tracer that handles opaque surfaces with lighting and shadows. This assignment has minimal amounts of OpenGL (just GLUT for a windowing system) and is implemented using C++.

### Running the Application:
1. Mac
	- To run my ray tracer with soft shadows and antialiasing, run the executable hw3 at the command line with extra command line arguments. The first three command line arguments are the same as the standard (no-extra credit) arguments (executable, scene file, output jpeg file name). The fourth argument is for soft shadows. To turn soft shadows on, use 'y' or 'Y'. Any other input will not activate soft shadows. The fifth argument is for antialiasing. To turn antialiasing on, use 'y' or 'Y'. Any other input will not activate antialiasing. 

	For example, to run my ray tracer with soft shadows and antialiasing for test2.scene and output the result to test2.jpg, run the following command:

	'''
	./hw3 test2.scene test2.jpg y y
	'''

### Mandatory Features
- Ray tracing triangles
- Ray tracing sphere
- Triangle Phong Shading
- Sphere Phong Shading                   
- Shadows rays                           
- Still images                           
   
### Extra Credit
- soft shadows
- antialiasing


### Notes
The soft shadows and antialiasing effects can most easily be seen using test2.scene (for your testing purposes). Also note that because of the complexity of the SIGGRAPH.scene file, rendering takes a significant amount of time (1 hour 24 minutes on my system). I take 48 light samples per light to achieve the soft shadows effect, so that is the source of the long rendering time. 

In my stills folder, 000.jpg - 004.jpg are the renderings of the scene files without soft shadows and antialiasing. 005.jpg - 009.jpg are the renderings of the scene files with both soft shadows and antialiasing.

