#ifndef LINE_H
#define LINE_H
#include <GL/glew.h>
#include "point.h"

class point;

class line
{
public:
    GLfloat m;
    GLfloat c;
    line(GLfloat, GLfloat);
    line *getPerpendicularLineFrom(point);
    point *getPointOfIntersection(line);
    point *getPointOnLineLyingDistanceAway(double, point);
};

#endif // LINE_H
