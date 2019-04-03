#include "framework.h"
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <math.h>

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(0, 1, 0, 1);	// computed color is the color of the primitive
	}
)";

vec4 asvec4(vec2 v) {
    return vec4(v.x, v.y, 0, 1);
}

vec2 asvec2(vec4 v) {
    return vec2(v.x, v.y);
}

struct Camera
{
    /// Center in word coordiantes
    vec2 wCenter;
    float width;
    float height;
    float * matrix = new float[4 * 4];
public:
    Camera(vec2 wCenter, float width, float height)
    :wCenter(wCenter), width(width), height(height)
    {}
    mat4 getMatrix() {
        float x_push = 2 * -wCenter.x / width;
        float y_push = 2 * -wCenter.y / height;
        return mat4(
                2/width, 0,        0, 0,
                0,       2/height, 0, 0,
                0,       0,        0, 0,
                x_push,  y_push,   0, 1
        );
    }
    mat4 getInversMatrix() {
	float x_push = wCenter.x;
        float y_push = wCenter.y;
        return mat4(
                width/2, 0,        0, 0,
                0,       height/2, 0, 0,
                0,       0,        0, 0,
                x_push,  y_push,   0, 1
        ); 
    }
    virtual ~Camera() {
        delete[] matrix;
    }
};

class Ground {

    vec2 end;
    unsigned int vao;
    unsigned int vbo;
    std::vector<vec2> cPoints = std::vector<vec2>();
    float tension = -0.1;
    vec2 afterEnd;
public:
    Ground(vec2 start, vec2 end)
    :end(end)
    {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	vec2 beforeStart = vec2(start);
	beforeStart.x -= 10;
	afterEnd = vec2(end);
	afterEnd.x += 10;
	cPoints.push_back(beforeStart);
	cPoints.push_back(start);
	cPoints.push_back(end);
	cPoints.push_back(afterEnd);
	
    }
    
    static bool orderByX(vec2 left, vec2 last) { return left.x < last.x; }

    void add(vec2 point) { 
	// Keep the vec2 end at the end.
	cPoints.pop_back();
	cPoints.pop_back();
	cPoints.push_back(point); 
        // The clicked point should be sorted according to the x coordinate
        std::sort(cPoints.begin(), cPoints.end(), orderByX);
	cPoints.push_back(end);
	cPoints.push_back(afterEnd);
    }

    /**
     * @param x - Ranges between 0 and the width of the screen
     */
    vec2 r(float x) {
        // The variables used in the formula
        float x0, x1, x2, x3, y0, y1, y2, y3, dy1, dy2;
        // 1. Preparing variables
        
        int index = -1;
        for (int i = 0; i < cPoints.size(); i++) {
            if ( cPoints[i].x <= x && x <= cPoints[i+1].x  ) {
                index = i;
                break;
            }
        } 
        if (index == -1) {
            std::cout << "Previous point not found";
        }

        // Declare points and calculate indexes in cPoints to them
        vec2 p0, p1, p2, p3;
        int index0, index1, index2, index3;
        
	index0 = index - 1;
	index1 = index;
	index2 = index + 1;
	index3 = index + 2;

	p0 = cPoints[index0]; p1 = cPoints[index1];
	p2 = cPoints[index2]; p3 = cPoints[index3];

        x0 = p0.x; y0 = p0.y;
        x1 = p1.x; y1 = p1.y;
        x2 = p2.x; y2 = p2.y;
        x3 = p3.x; y3 = p3.y;

        // 2. The formula 
        dy1 = (1 - tension) * (
                (y1 - y0) / (x1 - x0)
                +
                (y2 - y1) / (x2 - x1)
                );
        dy2 = (1 - tension) * (
                (y2 - y1) / (x2 - x1)
                +
                (y3 - y2) / (x3 - x2)
                );
        float a0 = y1;
        float a1 = dy1;
        float a2 = 
            ( 3 * (y2 - y1) ) / ( (float) pow(x2 - x1, 2) )
            -
            ( dy2 + 2 * dy1 ) / ( x2 - x1 );
        float a3 = 
            ( 2 * (y1 - y2) ) / ( (float) pow(x2 - x1, 3) )
            +
            ( dy2 + dy1 ) / ( (float) pow(x2 - x1, 2) );
        float y = a3 * std::pow(x - x1, 3) 
             + a2 * std::pow(x - x1, 2) 
             + a1 * (x - x1) 
             + a0;
        return vec2(x, y);
    }
    
    void display() {
	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	unsigned int vbo;		// vertex buffer object
	glGenBuffers(1, &vbo);	// Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Geometry with 24 bytes (6 floats or 3 x 2 coordinates)
	float vertices[windowWidth * 2];
	int doubleStep = 0;
	for (int i = 0; i < windowWidth; i++){
	    vertices[doubleStep] = r(i).x;
	    vertices[doubleStep+1] = r(i).y;
	    doubleStep += 2;
	}
	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(vertices),  // # bytes
		vertices,	      	// address
		GL_DYNAMIC_DRAW);	// we do not change later


	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed
	glDrawArrays(GL_LINE_STRIP, 0 /*startIdx*/, sizeof(vertices) /*# Elements*/);
    }
};



GPUProgram gpuProgram; // vertex and fragment shaders
Camera camera(
    vec2(windowWidth/2, windowHeight/2), // set center so that (0,0) is the bottom left corner
    windowWidth, windowHeight);
Ground * ground;

// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);

    

    // create program for the GPU
    gpuProgram.Create(vertexSource, fragmentSource, "outColor");
    ground = new Ground(vec2(0,windowHeight/2), vec2(windowWidth, windowHeight/2));
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0);     // background color
    glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

    // Set color to (0, 1, 0) = green
    int location = glGetUniformLocation(gpuProgram.getId(), "color");
    glUniform3f(location, 0.0f, 1.0f, 0.0f); // 3 floats
    location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
    glUniformMatrix4fv(location, 1, GL_TRUE, &camera.getMatrix().m[0][0]);	// Load a 4x4 row-major float matrix to the specified location

    ground->display();

    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
	vec4 v4newPoint = asvec4(vec2(cX, cY));
	v4newPoint = v4newPoint * camera.getInversMatrix();
	vec2 v2newPoint = asvec2(v4newPoint);
	if (state == GLUT_DOWN) ground->add(v2newPoint);
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}
