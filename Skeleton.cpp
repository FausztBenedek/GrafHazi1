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
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";
GPUProgram gpuProgram; // vertex and fragment shaders

vec4 asvec4(vec2 v) {
    return vec4(v.x, v.y, 0, 1);
}

vec2 asvec2(vec4 v) {
    return vec2(v.x, v.y);
}

#include "src/camera.h"
#include "src/spline.h"
#include "src/circle.h"

Spline * ground;

class Bike {
    vec2 pos;
    unsigned int vao;
    unsigned int vbo;
    float rad = 10;
    vec2 translate = vec2(0,0);
public:
    Bike(vec2 startPos) 
    :pos(startPos)
    {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
    }
    void update_variables() {
        pos.x += 1;
    }
    /**
     * Pushes the circle, as if it would be on the top of the spline
     */
    void push_pos() {
        float x = pos.x;

        // Gravitational counter force
        float f1;
        vec2 diff = ground->r(x + 1) - ground->r(x);
        vec2 normal;
        // Swap coordinates and -1 (turn 90grad)
        normal.x = diff.y * (-1); normal.y = diff.x;
    
        // Turn the vector around if y is negative
        if (normal.y < 0) normal = normal * (-1);
        normal = normalize(normal);
        normal = normal * rad;

        float y = ground->r(x).y;
        pos = vec2(x, y);
        x += normal.x;
        y += normal.y;
        translate = vec2(normal);
    }
    void display() {
        vec2 pos = this->pos;
        pos = pos + translate;
        int location = glGetUniformLocation(gpuProgram.getId(), "color");
        glUniform3f(location, 1.0f, 0.5f, 0.0f); // 3 floats
	glBindVertexArray(vao);		// make it active
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // Draw circle
        float vertices[360 * 2];
	int doubleStep = 0;
        for (int i = 0; i < 360; i++) {
            vec2 edge;
            float theta_rad = i * 2 * M_PI / (float) 360;
            edge.x = pos.x + rad * cos(theta_rad);
            edge.y = pos.y + rad * sin(theta_rad);
            vertices[  doubleStep  ] = edge.x;
            vertices[doubleStep + 1] = edge.y;
            doubleStep += 2;
        }
	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(vertices),  // # bytes
		vertices,	      	// address
		GL_DYNAMIC_DRAW);	


	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed
	glDrawArrays(GL_LINE_STRIP, 0 /*startIdx*/, 360 /*# Elements*/);
    }
    mat4 getRotationMatrix(float alpha) {
        return mat4(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 1
                );
    }
};

Camera camera(
    vec2(windowWidth/2, windowHeight/2), // set center so that (0,0) is the bottom left corner
    windowWidth, windowHeight);
Bike * bike;
Circle * circle;
CircleDrawer * circleDraw;
CircleController * circleControl;


// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);

    // create program for the GPU
    gpuProgram.Create(vertexSource, fragmentSource, "outColor");
    ground = new Spline(vec2(0,windowHeight/2), vec2(windowWidth, windowHeight/2), -0.1);
    circle = new Circle(vec2(10, 400), 10);
    circleDraw = new CircleDrawer(circle);
    circleControl = new CircleController(circle, ground);
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
    circleDraw->draw();

    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'd') { 
        glutPostRedisplay();         // if d, invalidate display, i.e. redraw
        circleControl->tick();
    }
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
