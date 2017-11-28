# Height Field
-----------------------------------------------------------------------
## Platform: Mac
-----------------------------------------------------------------------


-----------------------------------------------------------------------
Description: 
This assignment requires that I create a heightfield based off data from an image the user specifies at the command line. Upon generating the heightfield, the program generates a brief animation demonstrating all the required functionality. The heightfield then can be translated, rotated, and scaled with user input. The user can also toggle between the four different renderings of the heightfield using keyboard input.


-----------------------------------------------------------------------
Running the Application:
1. Mac
	- Navigate to the hw1-starterCode directory in terminal then type the 'make' command. This will generate the executable 'hw1'. To run the program enter type ./hw1 photo_path in Terminal where photo_path is the path to the image you wish to use to render the heightfield.


------------------------------------------------------------------------
Functionality:
	- Upon running the program, a 300 frame animation demonstrating all four renderings of the heightfield as well as rotation, translation, and scaling will occur.
	- Pressing '1' will change the heightfield to a point rendering.
	- Pressing '2' will change the heightfield to a wireframe rendering.
	- Pressing '3' will change the heightfield to a rendering using triangles.
	- Pressing '4' will change the heightfield to a rendering of wireframe on top of triangles. (EXTRA CREDIT)
	- Dragging with the left mouse button clicked enables rotation about the x and y axes. 
	- Dragging with the middle mouse button clicked enables rotation about the z axis.
	- Holding 't' while dragging with the left mouse button clicked enables scaling in the x and y directions.
	- Holding 't' while dragging with the middle mouse button clicked enables scaling in the z direction.
	- Pressing 'x' will take a screenshot of the contents of the window in which the application is running. 
	- Pressing 'esc' will quit the application.