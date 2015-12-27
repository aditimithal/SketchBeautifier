#ifndef POINT_H
#define POINT_H
#include <GL/glew.h>
#include "line.h"

class line;

class point
{
public:
    GLfloat x;
    GLfloat y;
    point(GLfloat, GLfloat);
    line *getTangent(point);

    bool operator==(const point& other){
        return x == other.x && y == other.y;
    }

    bool operator!=(const point& other){
        return !(*this == other);
    }
};

#endif // POINT_H
