// main.cpp : This file contains the 'main' function. Program execution begins and ends there.

//Standard library includes
#include <cstdio> //when you want to include a C standard library header, like stdio.h, use cstdio instead

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
using namespace std;

//SDL2 includes
#include <SDL.h>
#include <SDL_ttf.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 720
#define VIEWPORT_DIST 1000
#define DISTANCE_CLIP 35

#define _USE_MATH_DEFINES

//The x and y coordinates of a point, are in canvas-space (i.e. in terms of pixels), while the z coordinate of 
//a point is in terms of world-space. This allows easy conversion between the two spaces.
class Point {
	float x, y, z;
public:
	//x and y coordinates are shifted so that they are with respect to the center of the canvas.
	Point(float px, float py, float pz) {
		x = SCREEN_WIDTH / 2 + px;
		y = SCREEN_HEIGHT / 2 - py;
		z = pz;
	}
	Point() {
		x = 0;
		y = 0;
		z = 0;
	}
	//Return x, y coordinates in canvas-space with respect to center of the canvas
	float getX() { return x; }
	float getY() { return y; }

	//Returns the z coordinate (in world-space) and the x,y coordinates in canvas-space w.r.t the top-left corner
	float getZ() { return z; }
	float getXW() { return (x - SCREEN_WIDTH / 2); }
	float getYW() { return (SCREEN_HEIGHT / 2 - y); }

	//Sets the x and y coordinates. Assumes points are being entered w.r.t to top left of canvas.
	void setX(float px) { x = SCREEN_WIDTH / 2 + px; }
	void setY(float py) { y = SCREEN_HEIGHT / 2 - py; }
	void setZ(float pz) { z = pz; }
};

//Checks that the input coordinates are within the canvas, before plotting them.
void putPixel(int x, int y, uint32_t col, uint32_t(*pixels)[SCREEN_WIDTH]) {
	if ((x < 0) || (x > SCREEN_WIDTH - 1) || (y < 0) || (y > SCREEN_HEIGHT - 1))
		return;
	else
		pixels[y][x] = col;
}

//Draws line between two points specified in canvas-space. Uses Bresenham line algorithm
void drawLine(float px1, float py1, float px2, float py2, uint32_t colour, uint32_t(*pixels)[SCREEN_WIDTH]) {
	int x1 = (int)px1;
	int x2 = (int)px2;
	int y1 = (int)py1;
	int y2 = (int)py2;
	int minY = min(y1, y2);
	int maxY = max(y1, y2);
	int minX = min(x1, x2);
	int maxX = max(x1, x2);
	if (x2 == x1) {
		for (int i = minY; i <= maxY; i++) {
			putPixel(x1, i, colour, pixels);
		}
	}
	else if (y2 == y1) {
		for (int i = minX; i <= maxX; i++) {
			putPixel(i, y1, colour, pixels);
		}
	}
	else {
		float m = ((float)y2 - y1) / (x2 - x1);
		if ((m > 1) || (m < -1)) {
			float b = y1 - m * x1;
			for (int i = minY; i <= maxY; i++) {
				int x = (int)((i - b) / m);
				putPixel(x, i, colour, pixels);
			}
		}
		else {
			float b = y1 - m * x1;
			for (int i = minX; i <= maxX; i++) {
				int y = (int)(m * i + b);
				putPixel(i, y, colour, pixels);
			}
		}
	}
}

//Uses scanline algorithm to fill triangle
void fillTriangle(Point *v1, Point *v2, Point *v3, uint32_t colour, uint32_t(*pixels)[SCREEN_WIDTH]) {
	//Sort the vertices in increasing order of y-value and put theminto arr.
	Point *arr[3];
	arr[0] = v1;
	arr[1] = v2;
	arr[2] = v3;
	for (int i = 0; i < 3; i++) {
		for (int j = i + 1; j < 3; j++) {
			if (arr[j]->getY() > arr[i]->getY()) {
				Point *tmp = arr[i];
				arr[i] = arr[j];
				arr[j] = tmp;
			}
		}
	}

	//Convert the points to integers to allow incremental drawing of scanlines
	int x1 = (int)arr[0]->getX();
	int x2 = (int)arr[1]->getX();
	int x3 = (int)arr[2]->getX();
	int y1 = (int)arr[0]->getY();
	int y2 = (int)arr[1]->getY();
	int y3 = (int)arr[2]->getY();

	//Calculate line parameters.
	float m12 = 0, m13 = 0, m23 = 0, b12 = 0, b13 = 0, b23 = 0;
	if (x2 != x1) {
		m12 = ((float)y2 - (float)y1) / ((float)x2 - (float)x1);
		b12 = y2 - (m12 * x2);
	}
	if (x3 != x1) {
		m13 = ((float)y3 - (float)y1) / ((float)x3 - (float)x1);
		b13 = y3 - (m13 * x3);
	}
	else {
		m13 = 0;
		b13 = y3;
	}
	if (x3 != x2) {
		m23 = ((float)y3 - (float)y2) / ((float)x3 - (float)x2);
		b23 = y3 - (m23 * x3);
	}

	//x12 will be the x coordinates of the line along the line connecting v1 and v2. Likewise for x23, and x13.
	int x12, x13, x23;

	//Scanline drawing.
	for (int y = y1; y > y3; y--) {
		if ((int)x1 == (int)x3)
			x13 = (int)x3;
		else
			x13 = (int)((y - b13) / m13);
		if ((int)x1 == (int)x2)
			x12 = (int)x1;
		else
			x12 = (int)((y - b12) / m12);
		if ((int)x2 == (int)x3)
			x23 = x2;
		else
			x23 = (int)((y - b23) / m23);
		if (y > y2) {
			drawLine(x12, y, x13, y, colour, pixels);
		}
		else {
			drawLine(x13, y, x23, y, colour, pixels);
		}
	}
}


//Basic building block of all other shapes in this environment. Most of the computation happens at this level.
//Triangle vertices are in clockwise order. i.e. v1->v2->v3->v1 is a clockwise traversal of triangle perimeter.
//This is important for normal vector calculation.
class Triangle {
	Point *v1;
	Point *v2;
	Point *v3;
	float nx;
	float ny;
	float nz;
	uint32_t colour;
	uint32_t(*pixels)[SCREEN_WIDTH];
	public:
		void setTriangle(Point *p1, Point *p2, Point *p3, uint32_t col, uint32_t(*pix)[SCREEN_WIDTH]) {
			v1 = p1;
			v2 = p2;
			v3 = p3;
			//We also want to store the normal for each triangle.
			float v1x = (v2->getXW() * v2->getZ() - v1->getXW() * v1->getZ()) / VIEWPORT_DIST;
			float v1y = (v2->getYW() * v2->getZ() - v1->getYW() * v1->getZ()) / VIEWPORT_DIST;
			float v1z = v2->getZ() - v1->getZ();
			float v2x = (v3->getXW() * v3->getZ() - v1->getXW() * v1->getZ()) / VIEWPORT_DIST;
			float v2y = (v3->getYW() * v3->getZ() - v1->getYW() * v1->getZ()) / VIEWPORT_DIST;
			float v2z = v3->getZ() - v1->getZ();
			//Use cross-product to find the normal to those two vectors.
			nx = v1y * v2z - v1z * v2y;
			ny = v1z * v2x - v1x * v2z;
			nz = v1x * v2y - v1y * v2x;
			colour = col;
			pixels = pix;
		}
		Triangle() {
			v1 = NULL;
			v2 = NULL;
			v3 = NULL;
			colour = 0;
			pixels = NULL;
		}
		//Returns pointers to its vertices.
		Point *getV1() { return v1; }
		Point *getV2() { return v2; }
		Point *getV3() { return v3; }
		//Returns its normal vector components
		float get_nx() { return nx; }
		float get_ny() { return ny; }
		float get_nz() { return nz; }

		void drawTriangle(bool xray = false) {
			//Clipping. A triangle will only be drawn if all three of its vertices are in the horizontal, vertical, and
			//closeness range. To check horizontal and vertical conditions, we'll use canvas coordinates. To check 
			//forward/backward coniditon, we'll look at the z values.
			float v1x = getV1()->getX(); float v1y = getV1()->getY(); float v1z = getV1()->getZ();
			float v2x = getV2()->getX(); float v2y = getV2()->getY(); float v2z = getV2()->getZ();
			float v3x = getV3()->getX(); float v3y = getV3()->getY(); float v3z = getV3()->getZ();
			if (((v1x < SCREEN_WIDTH) && (v1x >= 0)) && ((v2x < SCREEN_WIDTH) && (v2x >= 0)) && ((v3x < SCREEN_WIDTH) && (v3x >= 0)) &&
				((v1y < SCREEN_HEIGHT) && (v1y >= 0)) && ((v2y < SCREEN_HEIGHT) && (v2y >= 0)) && ((v3y < SCREEN_HEIGHT) && (v3y >= 0)) &&
				((v1z < DISTANCE_CLIP) && (v1z > 0)) && ((v2z < DISTANCE_CLIP) && (v2z > 0)) && ((v3z < DISTANCE_CLIP) && (v3z > 0))) {
				//Determines backface-ness and lighting intensity.
				float dot_p = nx * (v1->getXW() * v1->getZ() / VIEWPORT_DIST) + ny * (v1->getYW() * v1->getZ() / VIEWPORT_DIST) + nz * v1->getZ();
				if (dot_p < 0) {
					if (xray == false) {
						fillTriangle(v1, v2, v3, -dot_p * colour, pixels);
					}
					else {
						drawLine(v1->getX(), v1->getY(), v2->getX(), v2->getY(), colour, pixels);
						drawLine(v1->getX(), v1->getY(), v3->getX(), v3->getY(), colour, pixels);
						drawLine(v2->getX(), v2->getY(), v3->getX(), v3->getY(), colour, pixels);
					}
				}	
			}
		}
};

//The constructor requires the x,y,z coordinates of the pyramid peak, radius of the circle that the base inscribes,
//the hieght, and the number of sides. Can take up to 99 sides. High number of sides allows approximation of a cone.
class Pyramid {
	int sides;
	float height;
	float radius;
	Point vertex[101];
	uint32_t colour;
	uint32_t(*pixels)[SCREEN_WIDTH];
	Triangle triangles[198];
	public:
		Pyramid(float nx, float ny, float nz, float h, float r, int num_sides, uint32_t c, uint32_t(*pix)[SCREEN_WIDTH]) {
			sides = num_sides;
			height = h;
			radius = r;
			float theta = 2 * M_PI / num_sides;
			for (int i = 0; i < num_sides; i++) {
				if (i == 0) {     //Set up first vertex manually.
					vertex[i].setX(nx * VIEWPORT_DIST / (nz - r));
					vertex[i].setY((ny - h) * VIEWPORT_DIST / (nz - r));
					vertex[i].setZ(nz - r);
				}
				else {      //Rest of the vertices are computed using a rotation matrix.
					//Get x-coord and z-coord in world-space
					float wx = vertex[i - 1].getXW() * vertex[i - 1].getZ() / VIEWPORT_DIST;
					float wz = vertex[i - 1].getZ();
					//Translate the points to the x-z origin.
					float x = wx - nx;
					float z = wz - nz;
					//Apply rotation
					float new_z = x * sin(theta) * (-1) + z * cos(theta);
					float new_x = x * cos(theta) + z * sin(theta);
					//Translate back to where it was (in world-space)
					new_z += nz;
					new_x += nx;
					//Translate back into canvas-space
					vertex[i].setZ(new_z);
					vertex[i].setX(new_x * VIEWPORT_DIST / vertex[i].getZ());
					vertex[i].setY((ny - h) * VIEWPORT_DIST / vertex[i].getZ());
				}
			}
			vertex[num_sides].setX(nx * VIEWPORT_DIST / nz);
			vertex[num_sides].setY(ny * VIEWPORT_DIST / nz);
			vertex[num_sides].setZ(nz);
			int t = 0;
			//Triangles on bottom
			for (int i = 2; i < num_sides; i++) {
				triangles[t].setTriangle(&vertex[0], &vertex[i], &vertex[i - 1], c, pix);
				t++;
			}
			//Triangles on surface
			for (int i = 0; i < num_sides - 1; i++) {
				triangles[t].setTriangle(&vertex[i], &vertex[i + 1], &vertex[num_sides], c, pix);
				t++;
			}
			triangles[t].setTriangle(&vertex[num_sides - 1], &vertex[0], &vertex[num_sides], c, pix);
			colour = c;
			pixels = pix;
		}
		void drawPyramid(bool xray = false) {
			for (int i = 0; i < (2 * sides - 2); i++) {
				triangles[i].drawTriangle(xray);
			}
		}
};

//Constructor requires x,y,z coordinates of bottom-left-near vertex of cube, and its size.
class Cube {
	Point v1, v2, v3, v4, v5, v6, v7, v8;
	float size;
	Triangle triangles[12];

	uint32_t colour;
	uint32_t(*pixels)[SCREEN_WIDTH];
	public:
		Cube(float x, float y, float z, float s, uint32_t c, uint32_t(*pix)[SCREEN_WIDTH]) {
			v1.setX(x * VIEWPORT_DIST / z); v1.setY(y * VIEWPORT_DIST / z); v1.setZ(z);
			v2.setX(x * VIEWPORT_DIST / z); v2.setY((y + s) * VIEWPORT_DIST / z); v2.setZ(z);
			v3.setX((x + s) * VIEWPORT_DIST / z); v3.setY((y + s) * VIEWPORT_DIST / z); v3.setZ(z);
			v4.setX((x + s) * VIEWPORT_DIST / z); v4.setY(y * VIEWPORT_DIST / z); v4.setZ(z);
			v5.setX(x * VIEWPORT_DIST / (z + s)); v5.setY(y * VIEWPORT_DIST / (z + s)); v5.setZ(z + s);
			v6.setX(x * VIEWPORT_DIST / (z + s)); v6.setY((y + s) * VIEWPORT_DIST / (z + s)); v6.setZ(z + s);
			v7.setX((x + s) * VIEWPORT_DIST / (z + s)); v7.setY((y + s) * VIEWPORT_DIST / (z + s)); v7.setZ(z + s);
			v8.setX((x + s) * VIEWPORT_DIST / (z + s)); v8.setY(y * VIEWPORT_DIST / (z + s)); v8.setZ(z + s);
			colour = c;
			pixels = pix;
			triangles[0].setTriangle(&v1, &v2, &v3, c, pix);	//Ensure that triangle vertices are in clockwise order
			triangles[1].setTriangle(&v1, &v3, &v4, c, pix);
			triangles[2].setTriangle(&v5, &v7, &v6, c, pix);
			triangles[3].setTriangle(&v5, &v8, &v7, c, pix);
			triangles[4].setTriangle(&v1, &v6, &v2, c, pix);
			triangles[5].setTriangle(&v1, &v5, &v6, c, pix);
			triangles[6].setTriangle(&v4, &v3, &v7, c, pix);
			triangles[7].setTriangle(&v4, &v7, &v8, c, pix);
			triangles[8].setTriangle(&v1, &v8, &v5, c, pix);
			triangles[9].setTriangle(&v1, &v4, &v8, c, pix);
			triangles[10].setTriangle(&v2, &v6, &v7, c, pix);
			triangles[11].setTriangle(&v2, &v7, &v3, c, pix);
		}
		void drawCube(bool xray = false) {
			for (int i = 0; i < 12; i++) {
				triangles[i].drawTriangle(xray);
			}
		}
};



int main(int argc, char* args[]) {
	bool running = true;

	//VERY IMPORTANT: Ensure SDL2 is initialized
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not initialize sdl2: %s\n", SDL_GetError());
		return 1;
	}

	//if using text in your program, ensure SDL2_ttf library is initialized
	if (TTF_Init() < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not initialize SDL2_ttf: %s\n", TTF_GetError());
	}

	//This creates the actual window in which graphics are displayed
	SDL_Window* window = SDL_CreateWindow(
		"hello_sdl2",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN
	);

	//Check for errors: window will be NULL if there was a problem
	if (window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	//A window by itself can't do many useful things.  We need a renderer so that we can paint in this window.
	//Think of a renderer like a paint brush.
	//First, let's instantiate a renderer in the window using the system's preferred graphics API.
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); //the "0" lets the SDL2 choose what's best.

	if (renderer == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create renderer: %s\n", SDL_GetError());
		return 1;
	}

	//OK, now we have the window and the rendering context.
	//Let's create our virtual "canvas" that we'll use to paint on.
	//This will live entirely on the CPU, and is stored in an SDL_Surface.
	//SDL_Surfaces are always CPU-only objects.
	//Note that we are requesting an RGBA8 surface.
	SDL_Surface* canvas = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
	if (canvas == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create surface: %s\n", SDL_GetError());
		return 1;
	}

	//Unfortunately, we can't output CPU surfaces directly to the window with the renderer.  We have to copy
	//it over first.  Create a texture on the GPU that will receive our rendered images.  Consider it to be the GPU
	//side of our canvas.
	//The pixel format should be in agreement with the surface given.
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	//SDL_TEXTUREACCESS_STREAMING allows the texture to be streamed from the CPU.

	if (texture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "could not create texture: %s\n", SDL_GetError());
		return 1;
	}

	// Just in case you need text:
	// load iosevka-regular.ttf at a large size into font
	TTF_Font* font = TTF_OpenFont("iosevka-regular.ttf", 64);
	if (!font) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont: %s\n", TTF_GetError());
		return 1;
	}

	//OK, now we're all set to start rendering.  Let's paint a white background.
	//FillRect essentially fills a rectangle of pixels with a solid colour for us.
	SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 0xFF, 0xFF, 0xFF));

	//Now, lets draw some pixel-sized points.

	//uint32_t* pixels = (uint32_t*) canvas->pixels; //Recall that an RGBA8 pixel is 32 bits wide
	uint32_t(*pixels)[SCREEN_WIDTH] = (uint32_t(*)[SCREEN_WIDTH]) canvas->pixels;

	SDL_Event ev;

	Cube cube1(-4, -3, 10, 2.5, 0xFF0000FF, pixels);
	Cube cube2(5, 2, 20, 2.5, 0x00FFF0FF, pixels);
	Cube cube3(1, 2, 10, 1, 0x012345FF, pixels);
	Pyramid pyr2(3, -1, 15, 2, 1, 49, 0x00000FFF, pixels);
	Pyramid pyr1(1, 4, 25, 3, 2, 7, 0xFFFF11FF, pixels);
	Pyramid pyr3(5, -1, 10, 2, 1, 20, 0x00000FFF, pixels);

	while (running) {
		cube2.drawCube();
		cube1.drawCube(true);
		cube3.drawCube();
		pyr1.drawPyramid(true);
		pyr2.drawPyramid();
		pyr3.drawPyramid();


		//ouput drawing on window
		SDL_UpdateTexture(texture, NULL, canvas->pixels, canvas->pitch);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		int x;
		scanf_s("%d", &x);

	}

	//Wait for input on the command line
	puts("Press any key in the terminal to exit");
	int const input = getchar();

	SDL_DestroyTexture(texture);
	SDL_FreeSurface(canvas);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// handles program exit, so no "return 0" statement is necessary.
	SDL_Quit();

	return 0;

}
