#ifndef PREDICTOR_H
#define PREDICTOR_H
#include <GL/glew.h>
#include "point.h"
#include <vector>

using namespace std;


class predictor
{

private:
//    float STRAIGHT_LINE_THRESHOLD;
    float ARC_THRESHOLD;

public:
    inline static double euclid_dist(GLfloat, GLfloat, GLfloat, GLfloat);
    inline static double euclid_dist(point a, point b);

    int depth;

    enum primitive_t
    {
        LINE,
        ARC,
        CIRCLE
    };

    struct prev_work;

    typedef struct answer{
        GLfloat *path;
        int segments;
        primitive_t primitive;
        float score;
        prev_work *prev;
    } answer;

    typedef struct prev_work
    {
        GLfloat score;
        bool clamp_centre;
        bool clamp_circum;
        bool clamp_radius;
        bool clamp_line_length;
        bool clamp_line_slope;
        bool clamp_point1;
        bool clamp_point2;
    } prev_work;

    prev_work* prev;

    vector<answer *> *results;
    GLfloat *points;
    long num_points;
    vector <GLint> prev_types;
    vector< vector <GLfloat> > prev_objects;

    int get_results_num() {return results->size();}
    answer *get_result(int i){return (*results)[i];}

    predictor();
    predictor(GLfloat *, long);
    void setup(GLfloat *, long);
    void setup(GLfloat *, long, vector<GLint>, vector< vector <GLfloat> >);
    void nested_work(prev_work *);

    answer *analyze();

};
#endif // PREDICTOR_H
