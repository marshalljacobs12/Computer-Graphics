# CSCI-420
Computer Graphics

This repo contains my solutions to the 3 assignments in CSCI 420 Computer Graphics at USC, taught by Professor Jernej Barbic. CSCI 420 covered introductory three-dimensional computer graphics, and assignments were completed using "modern" shader-based OpenGL (core profile). The assignment descriptions were taken from Professor Barbic's course website from the Spring of 2017, and the website is archived here: http://www-bcf.usc.edu/~jbarbic/cs420-s17/

I would like to credit Professor Barbic for an excellent, well-organized course, and Bohan Wang, the TA for the Spring 2017 course, for his constant help in office hours (without which I would have received a substantially lower grade lol).

A brief description of each assignment is listed below. For more detailed assignment descriptions, see the assignment descriptions in each project's folder.

Assignment 1: Height Fields
--> This assignment required me to create a height field based on the data from an image which the user specifies at the command line, and to allow the user to manipulate the height field in three dimensions by rotating, translating, or scaling it. It served as an introduction to shader-based OpenGL (core profile).

Assignment 2: Roller Coaster Simulation
--> In this assignment, I used Catmull-Rom splines along with OpenGL lighting and texture mapping to create a roller coaster simulation. The simulation runs in a first-person view, allowing the user to "ride" the coaster in an immersive environment.

Assignment 3: Ray Tracing
-->  In this assignment, I built a ray tracer. My ray tracer will be able to handle opaque surfaces with lighting and shadows. This assignment has minimal amounts of OpenGL (just GLUT for a windowing system) and is implemented using C++. 

Finally I would like to cite portions of the code that were provided to us but written by Dr. Barbic. In assignments 1 and 2, Dr. Barbic provided the openGLHelper-starterCode library, and for all three assignments, he provided the external library, which I made use of, especially the ImageIO files. To see exactly what starter code was given to students versus what I implemented myself, consult the archived course website and look in the assignment descriptions where you can download the starter code that I began each assignment with. 
