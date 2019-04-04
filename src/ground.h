#ifndef GROUND_H
#define GROUND_H
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
        int location = glGetUniformLocation(gpuProgram.getId(), "color");
        glUniform3f(location, 0.0f, 1.0f, 0.0f); // 3 floats
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
	glDrawArrays(GL_LINE_STRIP, 0 /*startIdx*/, windowWidth /*Pair of elements*/);
    }
};

#endif // GROUND_H
