#include "line.h"
#include "point.h"
#include <GL/glew.h>
#include <iostream>
#include <cmath>

point::point(GLfloat x_, GLfloat y_){x = x_; y = y_;}

line *point::getTangent(point neighbour)
{
    GLfloat slope = 0;
    if(neighbour.x != this->x)
    {
        slope = (neighbour.y - this->y)/(neighbour.x - this->x);
    }
    else
    {
        slope = (neighbour.y - this->y)/(neighbour.x - this->x + 1);
    }
    GLfloat offset = this->y = slope*this->x;
    return new line(slope, offset);
}
