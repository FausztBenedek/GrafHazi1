#ifndef CIRCLE_H
#define CIRCLE_H

#include <vector>
#include <math.h>

class Circle {

    float rad;
    float alpha = 0;

    mat4 rotationMatrix() {
        return mat4(
                cos(alpha), -sin(alpha), 0, 0,
                sin(alpha), cos(alpha),  0, 0,
                0,          0,           0, 0,
                0,          0,           0, 1 
                );
    }
    mat4 translateMatrix() {
        return mat4(
                1,0,0,0,
                0,1,0,0,
                0,0,0,0,
                center.x, center.y, 0, 1
                );
    }

public:

    Circle(vec2 center, float rad) 
    :rad(rad)
    {}

    vec4 center;

    const std::vector<vec4> getDrawingPoints() {

        std::vector<vec4> ret;
        for (int i = 0; i < 360; i++) {
            // edge will be the new point to add
            vec4 edge(0, 0, 0, 1);
            // convert i to radian
            float theta_rad = i * 2 * M_PI / (float) 360;

            edge.x = rad * cos(theta_rad);
            edge.y = rad * sin(theta_rad);
            ret.push_back(edge);
            if (i == 90) {
                // In this case the loop is at the top
                // and the opposite edge is at the bottom
                vec4 oppositeEdge(-rad, 0, 0, 1);
                ret.push_back(oppositeEdge);
                ret.push_back(edge); // Go back
            }
            else if (i == 180) {
                // In this case the loop is at the right,
                // so the opposite edge is at the left
                vec4 oppositeEdge(0, rad, 0, 1);
                ret.push_back(oppositeEdge);
                ret.push_back(edge); // Go back
            }
        }
        for (int i = 0; i < ret.size(); i++) {
            ret[i] = ret[i] * 
                this->rotationMatrix() *
                this->translateMatrix();
        }
        return ret;
    }
};

class CircleDrawer {

    Circle* circle;
    unsigned int vao;
    unsigned int vbo;

public:

    CircleDrawer(Circle* circle)
    :circle(circle)
    {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
    }

    void draw() {
        // set color
        int location = glGetUniformLocation(gpuProgram.getId(), "color");
        glUniform3f(location, 1.0f, 0.5f, 0.0f); 
        // vao, vbo
	glBindVertexArray(vao);		
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // Get drawing vertices from circle and copy it to the gpu
        std::vector<vec4> vVertices = circle->getDrawingPoints();
        // This array will be copied to the gpu
        float fVertices[vVertices.size() * 2];

        // copy values to fVertices
        int doubleStep = 0;
        for (int i = 0; i < vVertices.size(); i++) {
            fVertices[  doubleStep  ] = vVertices[i].x;
            fVertices[doubleStep + 1] = vVertices[i].y;
            doubleStep += 2;
        }

	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(fVertices),  // # bytes
		fVertices,	      	// address
		GL_DYNAMIC_DRAW);	

	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed
	glDrawArrays(GL_LINE_STRIP, 0 /*startIdx*/, vVertices.size() /*# Elements*/);
    }
};

#endif // CIRCLE_H
