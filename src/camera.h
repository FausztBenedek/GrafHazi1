#ifndef CAMERA_H
#define CAMERA_H

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

#endif // CAMERA_H
