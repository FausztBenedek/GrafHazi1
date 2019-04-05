#ifndef CIRCLE_H
#define CIRCLE_H

#include <vector>
#include <math.h>

class Circle {

    float rad;

    mat4 rotationMatrix() {
        return mat4(
                cos(alpha), -sin(alpha), 0, 0,
                sin(alpha), cos(alpha),  0, 0,
                0,          0,           0, 0,
                0,          0,           0, 1 
                );
    }
    mat4 centerSetterMatrix() {
        return mat4(
                1,0,0,0,
                0,1,0,0,
                0,0,0,0,
                center.x, center.y, 0, 1
                );
    }
    mat4 pushFromCenterMatrix() {
        return mat4(
                1,0,0,0,
                0,1,0,0,
                0,0,0,0,
                pushFromCenter.x, pushFromCenter.y, 0, 1
                );
    }

public:

    Circle(vec2 center, float rad) 
    :rad(rad)
    {
        this->center.x = center.x;
        this->center.y = center.y;
    }

    vec4 center;
    vec2 pushFromCenter = vec2(0,0);
    float alpha = 0;
    float getRad() { return rad; }

    const std::vector<vec4> getDrawingPoints() {

        std::vector<vec4> ret;
        for (int i = 0; i < 361; i++) {
            // edge will be the new point to add
            vec4 edge(0, 0, 0, 1);
            // convert i to radian
            float theta_rad = i * 2 * M_PI / (float) 360;

            edge.x = rad * cos(theta_rad);
            edge.y = rad * sin(theta_rad);
            ret.push_back(edge);
            
            if (i % 45 == 0) {
                vec4 oppositeEdge = edge * (-1);
                oppositeEdge.w = 1;
                ret.push_back(oppositeEdge);
                ret.push_back(edge);
            }
        }
        for (int i = 0; i < ret.size(); i++) {
            ret[i] = ret[i] * this->rotationMatrix();
            ret[i] = ret[i] * this->centerSetterMatrix();
            ret[i] = ret[i] * this->pushFromCenterMatrix();
        }
        return ret;
    }
};

class CircleController {

    Circle * circle;
    Spline * ground;
    bool rightGoing;

public:
    CircleController(Circle * circle, Spline * ground, bool rightGoing = true)
    :circle(circle), ground(ground), rightGoing(rightGoing)
    {}

    void tick() {
        // Update circle data
        {
            // The slope (derivative) (dx = vel)
            float dy = ground->r(circle->center.x + 1).y - ground->r(circle->center.x).y;

            // Update velocity
            static float vel = 0; // difference in x coordinate
            {

                // Effect of the gravitational
                float f_grav_x; 
                // The formula was calculated on a piece of paper
                f_grav_x = -1 * (dy * 10) / (dy*dy + 1);

                // Effect of the air resistance
                float f_airResistance_x;
                // Experimented constant * velocity
                f_airResistance_x = -0.005 * vel;

                float f_ride = rightGoing ? 1.5 : -1.5;

                vel += f_grav_x;
                vel += f_airResistance_x;
                vel += f_ride;
            }

            // Update andle
            float dAlpha; 
            {
                dAlpha = 0.02 * sqrt(dy*dy + vel*vel);
                if (vel < 0) dAlpha *= -1;
            }


            circle->center.x += vel;;
            circle->alpha += dAlpha;
            if (circle->alpha > 2 * M_PI) circle->alpha = 0;

            // Determine direction and manage turn arounds at the side
            {
                if (rightGoing) {
                    // If the circle is beyond the right side
                    // than turn around
                    if (circle->center.x + circle->getRad() > windowWidth) {
                        rightGoing = false;
                        vel = 0;
                        circle->center.x = windowWidth - circle->getRad();
                    }
                } else {
                    // If the circle is beyond the left side
                    // than turn around
                    if (circle->center.x - circle->getRad() < 0) {
                        rightGoing = true;
                        vel = 0;
                        circle->center.x = circle->getRad();
                    }
                }
            } 
        }
        // Put circle to position (make it appear as if it would be on the line)
        {
            // Adjust y coordinate
            float x = circle->center.x;
            circle->center.y = ground->r(x).y;

            // Push circle perpendicular to the ground spline
            // This creates an illusion that the circle is ON the spline
            vec2 diff = ground->r(x + 1) - ground->r(x);
            vec2 normal;
            // Swap coordinates and -1 (turn 90grad)
            normal.x = diff.y * (-1); normal.y = diff.x;
        
            // Turn the vector around if y is negative
            // normal has to be on the upper half plane
            if (normal.y < 0) normal = normal * (-1);
            normal = normalize(normal);
            normal = normal * circle->getRad();

            circle->pushFromCenter = normal;
        }
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
