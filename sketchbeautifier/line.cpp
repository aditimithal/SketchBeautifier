#include "line.h"
#include "point.h"
#include <GL/glew.h>
#include <iostream>
#include <cmath>
using namespace std;

line::line(GLfloat m_, GLfloat c_)
{
    if (std::isinf(m_)) m_ = 10000000;
    m = m_;
    c = c_;
}

line *line::getPerpendicularLineFrom(point p)
{
    GLfloat slope ;

    if(this->m !=0)
         slope = -(1/this->m);
    else slope=INFINITY;
    cout<<"slope"<<slope<<endl;


    GLfloat offset = p.y - slope*p.x;
    return new line(slope, offset);
}

point *line::getPointOfIntersection(line other)
{
    GLfloat x;
    if(this->m - other.m!=0)
        x = (other.c - this->c)/(this->m - other.m);
    else x=INFINITY;
    GLfloat y = this->m * x + this->c;
    cout<<"x:"<<x<<endl;

    return new point(x, y);
}

point *line::getPointOnLineLyingDistanceAway(double distance, point center)
{
    GLfloat x_ = center.x + distance/(sqrt(1 + m*m));
    GLfloat y_ = m*x_ + this->c;
    return new point(x_, y_);
}
