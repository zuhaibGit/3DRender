This file describes usasge of the script.

The program allows the user to generate two types of 3d objects:
* Cube
* Variable-sided Pyramid
Parameters can be used to control the size, position, colour, and render type (filled or skeleton) of each shape.

# 1 - CLASSES

## 1.1 - CUBE CLASS
	
The definition of the cube constructor is:

	Cube::Cube(float x, float y, float z, float s, uint32_t c, uint32_t(*pix)[SCREEN_WIDTH])

c is the color of the cube. pix is a pointer to the pixels array.

x, y, z represent the world-space coordinates of the bottom-left-near corner of the cube.

s allows you to specify the side lengths of the cube.

The rest of the points in the cube are then computed and stored in 8 Point objects. Then through indirection, an array of 12 triangles is created, where each triangle contains pointers to 3 of the Points in the cube.

To draw a cube, use its drawCube(bool xray) method. If xray is true, then a skeleton of the cube will be drawn showing the triangles that make up the cube. Useful if the colored cube is hard to make sense of due to lighting problems. If xray is false, then the cube will be drawn with the colour c. Whether xray is true or false, the rendering, in both cases, is done by iteratin through the array of trinagles, and calling draw method of each one. If x ray is true, it'll draw the 3 lines for each triangle. If it's false, it'll call the fillTriangle() method for each triangle. The function uses scanline algorithm.
 
## 1.2 - PYRAMID CLASS
	
The definition of the Pyramid constructor is

	Pyramid::Pyramid(float nx, float ny, float nz, float h, float r, int num_sides, uint32_t c, uint32_t(*pix)[SCREEN_WIDTH])

c and pix represent the colour and pointer to pixels.

nx, ny, nz represent the coordinate (in world-space) of the tip of the pyramid. 

h and r represent the height of the pyramid, and the radius of the circle that the base inscribes.

num_sides represents the number of sides that you want the base to have. Using trig, the computer then computes the world-space coordinates of all the vertices of the pyramid, as well as all the triangles that will make up the shape. The vertices and triangles are stored in arrays of Points and Triangles respectively. Note that the program will not accept num_sides > 99.

The Pyramid is rendered the same way as the cube. It too has an xray option to view the skeleton

## 1.3 - TRIANGLE CLASS
	
The constructor for the Triangle class is

	Triangle::Triangle(Point *p1, Point *p2, Point *p3, uint32_t col, uint32_t(*pix)[SCREEN_WIDTH])

c and pix are the colour and pointer to pixels.

p1, p2, and p3 are pointers to three Point objects. NOTE: It's crucial that the points are labelled in such a way that if we did a p1->p2->p3->p1 traversal of the OUTER face of the triangle, that the traversal should be clockwise. This is important because otherwise, the calculation of the normal vectors (used in backface removal and lighting intensity calculations) will not be accurate.

This class is where most of the computation happens. Before rendering, the triangle is checked to see if  
* it's fully within the inside of the clipping planes (i.e. ALL THREE vertices are on the inside)
* it's not a backface.
If these conditions hold, then the triangle is rendered (either as a skeleton, or a filled object).

## 1.4 - POINT CLASS

The constructor for this class is

	Point::Point(float px, float py, float pz)

It'll take the x, y points in canvas coordinates, and the z coordiante corresponds to the world-space coordinate that the point is being projected from. This allows easy conversion between the point on the canvas and the real-world coordinate of that point. Throughout the program, the point, before being instantiated is transformed from world-space coordinates, into canvas coordinates. The constructor takes points that are assumed to be w.r.t to the top-left corner of the canvas, and shifts them so that they are w.r.t to the center of the screen.

Note the two versions of x and y coordinate-returning functions. getX() returns the canvas coordinate w.r.t to the top left of the screen, while getXW() returns the x coordinate w.r.t the center of the screen.

## 2 - ALGORITHMS
	
Brasenham's algorithm was used in line drawing.

Backface culling was used to remove backfaces.

Directional light was used (in the direction of the camera) to illuminate each triangle
of the object. Light intensity was calculated by using the dot product between the normal of the triangle, and the direction of the light. Note: I wasn't quite sure how to change the colour of a face depending on the intensity of light shining on it, so some of the models rendered may have strange lighting. 

A 3D clipping volume was used. The vertical and horizontal clipping planes are defined by the canvas, and the depth plane is defined by a constant DISTANCE_CLIP (set to 35).
Clipping was very rudimentary. Any triangle that was not fully within the clipping volume was not rendered.

Hierarchichal modelling and indirection was used to breakdown each shape into primitive triangles to allow for easier scene modelling.
	
