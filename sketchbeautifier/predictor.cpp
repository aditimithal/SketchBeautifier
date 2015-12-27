#include "predictor.h"
#include <GL/glew.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "point.h"
#include "line.h"
using namespace std;
# define ABS(x) (((x) < 0) ? -(x) : x)
# define MIN(x, y) (((x) < (y)) ? (x) : (y))
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define PI 3.14159265

predictor::predictor()
{
    points = NULL;
    num_points = 0;
    results = new vector<answer *>;
}

predictor::predictor(GLfloat *p, long num)
{
    points = p;
    num_points = num;
    results = new vector<answer *>;
}

void predictor::nested_work(prev_work * p)
{
    this->prev = p;
}

void copy_prev_work_struct(predictor::prev_work *old, predictor::prev_work *new_)
{
    if(old == NULL) return;
    new_->clamp_centre = old->clamp_centre;
    new_->clamp_circum = old->clamp_circum;
    new_->clamp_line_length = old->clamp_line_length;
    new_->clamp_line_slope = old->clamp_line_slope;
    new_->clamp_point1 = old->clamp_point1;
    new_->clamp_point2 = old->clamp_point2;
    new_->clamp_radius = old->clamp_radius;
    new_->score = old->score;
}

void predictor::setup(GLfloat *p, long num){
    this->points = p;
    this->num_points = num;
}

void predictor::setup(GLfloat *p, long num, vector<GLint> types,  vector< vector <GLfloat> > prev_obj){
    this->points = p;
    this->num_points = num;
    this->prev_types = types;
    this->prev_objects = prev_obj;
}

inline double predictor::euclid_dist(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1){
    GLfloat x = (x0 - x1) * (x0 - x1);
    GLfloat y = (y0 - y1) * (y0 - y1);

    return sqrt(x + y);
}

inline double predictor::euclid_dist(point a, point b){
    GLfloat x0 = a.x; GLfloat y0 = a.y;
    GLfloat x1 = b.x; GLfloat y1 = b.y;
    return euclid_dist(x0, y0, x1, y1);
}

void sample_points_into(int m, GLfloat **points_addr, int num_points){
    GLfloat *points = *points_addr;
    int MINIMUM_POINTS = m;

    // take at least 100 points for sampling.
    GLfloat *sample_points;

    if (num_points < MINIMUM_POINTS)
    {
        sample_points = new GLfloat[num_points * 2];
        for(int i = 0; i < num_points; i++)
        {
            sample_points[2*i] = points[2*i];
            sample_points[2*i + 1] = points[2*i+1];
        }
    }
    else
    {
        sample_points = new GLfloat[MINIMUM_POINTS * 2];

        int incrementer = (int)(num_points/MINIMUM_POINTS);

        int picker = 0;
        for(int i = 0; i < MINIMUM_POINTS - 1; i++)
        {
            sample_points[2*i] = points[picker*2];
            sample_points[2*i + 1] = points[picker*2 + 1];
            picker += incrementer;
        }

        // ensure that end point is in the sample.
        sample_points[2 * (MINIMUM_POINTS-1)] = points[2 * (MINIMUM_POINTS - 1)];
        sample_points[2 * (MINIMUM_POINTS-1) + 1] = points[2 * (MINIMUM_POINTS - 1) + 1];
    }
    *points_addr = sample_points;
}


point *find_intersection_point_circle(point center, GLfloat radius, point *p) {
    GLfloat cx = center.x, cy = center.y;
    point point1 = *p, point2 = center;
    GLfloat dx, dy, A, B, C, det, t;

        dx = point2.x - point1.x;
        dy = point2.y - point1.y;

        A = dx * dx + dy * dy;
        B = 2 * (dx * (point1.x - cx) + dy * (point1.y - cy));
        C = (point1.x - cx) * (point1.x - cx) +
            (point1.y - cy) * (point1.y - cy) -
            radius * radius;

        det = B * B - 4 * A * C;
        if ((A <= 0.0000001) || (det < 0))
        {
            // No real solutions.
            return NULL;
        }
        else if (det == 0)
        {
            // One solution.
            t = -B / (2 * A);
            return new point(point1.x + t * dx, point1.y + t * dy);
        }
        else
        {
            t = (float)((-B + sqrt(det)) / (2 * A));
            point *result1 = new point(point1.x + t * dx, point1.y + t * dy);
            t = (float)((-B - sqrt(det)) / (2 * A));
            point *result2 = new point(point1.x + t * dx, point1.y + t * dy);
            if (predictor::euclid_dist(*p, *result1) < predictor::euclid_dist(*p, *result2))
            {
                return result1;
            }
            return result2;
        }
}


predictor::answer *constraint_straight_line_endpoint_snapping(point start, point end, vector<GLint> prev_types, vector< vector <GLfloat> > prev_obj, bool nested=false, predictor::prev_work *prev = NULL){
    bool DEBUG = false;
    GLfloat MAX_DISTANCE_ALLOWED = 0.05;
    vector< vector <GLfloat> > lines_to_snap_to;
    predictor::answer *centre_join_result = NULL;
    point *consider_point = NULL;

    if (DEBUG) cout << "--- ENDPOINT SNAPPING ---" << endl;

    for (uint i = 0; i < prev_obj.size(); ++i) {
        if (prev_types[i] == 1 || prev_types[i] == 2) // TODO: check if this is correct.
        {
            point p1(prev_obj[i][0], prev_obj[i][1]);
            point p2(prev_obj[i][2], prev_obj[i][3]);
            GLfloat min_dist = MIN(100, predictor::euclid_dist(p1, start));
            min_dist = MIN(min_dist, predictor::euclid_dist(p1, end));
            min_dist = MIN(min_dist, predictor::euclid_dist(p2, start));
            min_dist = MIN(min_dist, predictor::euclid_dist(p2, end));

            // Let's snap line end points to circle's circumference as well.
            if (prev_types[i] == 2)
            {
                GLfloat radius = predictor::euclid_dist(p1, p2);
                consider_point = NULL;
                if (sqrt(ABS(
                            (start.x - p1.x) * (start.x - p1.x) +
                            (start.y - p1.y) * (start.y - p1.y) -
                            radius * radius
                            )) < MAX_DISTANCE_ALLOWED * 5)
                {
                    if(prev == NULL || !prev->clamp_point1) consider_point = &start;
                }
                if (sqrt(ABS(
                            (end.x - p1.x) * (end.x - p1.x) +
                            (end.y - p1.y) * (end.y - p1.y) -
                            radius * radius
                            )) < MAX_DISTANCE_ALLOWED * 4)
                {
                    if(prev == NULL || !prev->clamp_point2)  consider_point = &end;
                }
                if (consider_point != NULL)
                {
                    point *circum = find_intersection_point_circle(p1, radius, consider_point);
                    if (circum != NULL)
                    {
                        centre_join_result = new predictor::answer;
                        centre_join_result->primitive = predictor::LINE;
                        centre_join_result->segments = 4;
                        centre_join_result->path = new GLfloat[4];
                        point *other = (consider_point == &start) ? &end : &start;
                        centre_join_result->path[0] = other->x;
                        centre_join_result->path[1] = other->y;
                        centre_join_result->path[2] = circum->x;
                        centre_join_result->path[3] = circum->y;
                        centre_join_result->score = 1 + (1 - predictor::euclid_dist(*circum, *consider_point));

                        centre_join_result->score += (prev == NULL) ? 0: prev->score;
                    }


                    /*
                    // find point on circumference joining consider_point and center.
                    GLfloat slope = (consider_point->y - p1.y)/(consider_point->x - p1.x);
                    GLfloat alpha = p1.y - slope * p1.x;

//                    GLfloat y = (slope*consider_point->x - consider_point->y)/slope;
//                    GLfloat a = 1,
//                            b = -2 * p1.x,
//                            c = (p1.x*p1.x) + (p1.y*p1.y) - (radius*radius) - (2 * p1.y*y) + (y*y);
                    GLfloat a = 1 + slope*slope,
                            b = 2*slope*alpha - 2*p1.x - 2*p1.y*slope,
                            c = alpha*alpha - 2*p1.y*alpha + p1.x*p1.x + p1.y*p1.y - radius*radius;

                    if (DEBUG) cout << "a, b, c : " << a << ", " << b << ", " << c << endl;

                    pair <GLfloat, GLfloat> *roots = solve_quadratic(a, b, c);
                    if (roots != NULL && (roots->first != roots->second || roots->first != 10000))
                    {
                        GLfloat x1 = roots->first,
                                y1 = sqrt((radius*radius) - (x1*x1)),
                                dist1 = predictor::euclid_dist(x1, y1, consider_point->x, consider_point->y);
                        GLfloat x2 = roots->second,
                                y2 = sqrt((radius*radius) - (x2*x2)),
                                dist2 = predictor::euclid_dist(x2, y2, consider_point->x, consider_point->y);
                        point *found;
                        if(dist1 > dist2) found = new point(x2, y2);
                        else found = new point(x1, y1);

                        predictor::answer *result = new predictor::answer;
                        result->primitive = predictor::LINE;
                        result->segments = 4;
                        result->path = new GLfloat[4];
                        point *other = (consider_point == &start) ? &end : &start;
                        result->path[0] = other->x;
                        result->path[1] = other->y;
                        result->path[2] = consider_point->x;
                        result->path[3] = consider_point->y;

                        cout << "centre, circumference (" << p1.x << "," << p1.y <<"),("
                                << p2.x << "," << p2.y << ")" << endl;
                        cout << "new line points (" << result->path[0] << "," << result->path[1]
                                << "), (" << result->path[2] << "," << result->path[3] << ")" << endl;

//                        result->score = 1 - predictor::euclid_dist(new_start, start)/10 - predictor::euclid_dist(new_end, end)/10;
                        return result;
                    }
                    */

                }
            }

            if(DEBUG) cout << "minimum_distance for snapping from each end point is " << min_dist << endl;
            if (min_dist < MAX_DISTANCE_ALLOWED)
            {
                lines_to_snap_to.push_back(prev_obj[i]);
            }
        }
    }

    if(DEBUG) cout << "Considering " << lines_to_snap_to.size() << " lines" << endl;

    if (lines_to_snap_to.empty()) return NULL;

    point new_start(start.x, start.y);
    point new_end(end.x, end.y);

    GLfloat start_diff = 1000;
    GLfloat end_diff = 1000;

    for (uint i = 0; i < lines_to_snap_to.size(); ++i) {
        vector<GLfloat> line_points = lines_to_snap_to[i];

        GLfloat s_diff = predictor::euclid_dist(start.x, start.y, line_points[0], line_points[1]);
        GLfloat e_diff = predictor::euclid_dist(end.x, end.y, line_points[2], line_points[3]);
        if(s_diff < start_diff && s_diff < MAX_DISTANCE_ALLOWED && (prev == NULL || !prev->clamp_point1))
        {
            start_diff = s_diff;
            new_start.x = line_points[0];
            new_start.y = line_points[1];
        }
        if(e_diff < end_diff && e_diff < MAX_DISTANCE_ALLOWED && (prev == NULL || !prev->clamp_point2))
        {
            end_diff = e_diff;
            new_end.x = line_points[2];
            new_end.y = line_points[3];
        }

        s_diff = predictor::euclid_dist(start.x, start.y, line_points[2], line_points[3]);
        e_diff = predictor::euclid_dist(end.x, end.y, line_points[0], line_points[1]);
        if(s_diff < start_diff && s_diff < MAX_DISTANCE_ALLOWED && (prev == NULL || !prev->clamp_point1))
        {
            start_diff = s_diff;
            new_start.x = line_points[2];
            new_start.y = line_points[3];
        }
        if(e_diff < end_diff && e_diff < MAX_DISTANCE_ALLOWED && (prev == NULL || !prev->clamp_point2))
        {
            end_diff = e_diff;
            new_end.x = line_points[0];
            new_end.y = line_points[1];
        }
    }
    if (DEBUG) cout << "Found better end points from line segments " <<
                       new_start.x << " " << new_start.y << " " <<
                       new_end.x << " " << new_end.y << endl;

    predictor::answer *result = new predictor::answer;
    result->primitive = predictor::LINE;
    result->segments = 4;
    result->path = new GLfloat[4];
    result->path[0] = new_start.x;
    result->path[1] = new_start.y;
    result->path[2] = new_end.x;
    result->path[3] = new_end.y;
    result->score = 1 + (1 - predictor::euclid_dist(new_start, start) - predictor::euclid_dist(new_end, end));

    result->score += (prev == NULL) ? 0 : prev->score;

    result->prev = new predictor::prev_work;
    copy_prev_work_struct(prev, result->prev);
    if (new_start != start) result->prev->clamp_point1 = true;
    else if (new_end != end) result->prev->clamp_point2 = true;

    // let's see if attaching to circumference is better.
    if (centre_join_result != NULL && centre_join_result->score > result->score)
    {
        centre_join_result->prev = new predictor::prev_work;
        copy_prev_work_struct(prev, centre_join_result->prev);
        if(consider_point == &start)
            centre_join_result->prev->clamp_point1 = true;
        if(consider_point == &end)
            centre_join_result->prev->clamp_point2 = true;

        if(!nested && prev == NULL) return centre_join_result;
        if((prev->clamp_point1 && consider_point == &start) || (prev->clamp_point2 && consider_point == &end))
        {
            if (DEBUG) cout << "Discarding result" << endl;
        } else{
            return centre_join_result;
        }
    }
    if (nested && ((prev->clamp_point1 && new_start != start) || (prev->clamp_point2 && new_end != end))) return NULL;
    return result;
}


predictor::answer *constraint_straight_line_parallel(point start, point end, vector<GLint> prev_types, vector< vector <GLfloat> > prev_obj, bool nested=false, predictor::prev_work *prev=NULL){
    bool DEBUG = false;
    GLfloat MAX_DEGREE_ROTATION_ALLOWED = (15 * PI)/180;

    if(nested && prev != NULL & (prev->clamp_line_slope || (prev->clamp_point1 && prev->clamp_point2))) return NULL;

    vector<GLfloat> degrees;
    for(uint i = 0; i < prev_obj.size(); i++)
    {
        if (prev_types[i] == 1)
        {
            point p1(prev_obj[i][0], prev_obj[i][1]);
            point p2(prev_obj[i][2], prev_obj[i][3]);
            degrees.push_back(atan((p2.y - p1.y)/(p2.x - p1.x)));
        }
    }
    GLfloat line_degre = atan((start.y - end.y)/(start.x - end.x));
    GLfloat nearest = 180;
    GLfloat degree_to_change_to;
    bool sign = true;
    for (uint i = 0; i < degrees.size(); ++i) {
        GLfloat degree = degrees[i];
        if (ABS(line_degre - degree) < nearest)
        {
            nearest = ABS(line_degre - degree);
            sign = (line_degre - degree > 0) ? true : false;
            degree_to_change_to = degree;
        }
    }
    if (nearest < MAX_DEGREE_ROTATION_ALLOWED)
    {
        if (DEBUG) cout<< "Changing angle by " << nearest * 180 / PI << " degrees" << endl;
        predictor::answer *result = new predictor::answer;
        result->primitive = predictor::LINE;
        result->segments = 4;
        result->path = new GLfloat[4];
        result->prev = new predictor::prev_work;
        copy_prev_work_struct(prev, result->prev);

        if(sign) nearest = -1 * nearest;

        if (prev == NULL || prev->clamp_point1)
        {
            result->path[0] = start.x;
            result->path[1] = start.y;
            point new_end(
                        (end.x - start.x) * cos(nearest) - (end.y - start.y) * sin(nearest) + start.x,
                        (end.x - start.x) * sin(nearest) + (end.y - start.y) * cos(nearest) + start.y
                        );
            result->path[2] = new_end.x;
            result->path[3] = new_end.y;
        } else if (!prev->clamp_point1 && prev->clamp_point2)
        {
            nearest *= -1; // TODO: check this.
            point new_start(
                        (start.x - end.x) * cos(nearest) - (start.y - end.y) * sin(nearest) + end.x,
                        (start.x - end.x) * sin(nearest) + (start.y - end.y) * cos(nearest) + end.y
                        );
            result->path[0] = new_start.x;
            result->path[1] = new_start.y;
            result->path[2] = end.x;
            result->path[3] = end.y;
        }
        // to keep it parallel
        result->prev->clamp_point2 = true;
        result->prev->clamp_point1 = true;

        if (DEBUG)
        {
            cout << "Nearest angle is " << degree_to_change_to * 180/PI << endl;
            cout << "Original angle is " << atan((start.y - end.y)/(start.x - end.x)) * 180/PI << endl;
            cout << "Changed angle is" << atan((result->path[1] - result->path[3])/(result->path[0] - result->path[2])) * 180/PI << endl;
        }

        result->score = 1 + (MAX_DEGREE_ROTATION_ALLOWED - ABS(nearest)) / MAX_DEGREE_ROTATION_ALLOWED;
        result->score += (prev == NULL) ? 0 : prev->score;
        copy_prev_work_struct(prev, result->prev);

        if (prev != NULL){
            if(prev->clamp_line_slope){
                if (DEBUG) cout << "Discarding this [125]" << endl;
                return NULL;
            }
            else if (prev->clamp_point1 && start != point(result->path[0], result->path[1])){
                if (DEBUG) cout << "Discarding this [126]" << endl;
                return NULL;
            }
            else if (prev->clamp_point2 && start != point(result->path[2], result->path[3])){
                if (DEBUG) cout << "Discarding this [127]" << endl;
                return NULL;
            }
        }

        return result;
    }
    return NULL;
}


predictor::answer *constraint_straight_line_axes(point start, point end, bool nested=false, predictor::prev_work *prev=NULL){

    //~~~ TODO -> down-up-left
    //        up-down-right not working;
    //~~~
    // should work with atan2
    bool DEBUG = false;
    GLfloat MAX_DEGREE_ROTATION_ALLOWED = (15 * PI)/180;

    GLfloat theta = atan((start.y - end.y)/(start.x - end.x));

    if (ABS(theta) < MAX_DEGREE_ROTATION_ALLOWED ||
            ABS(theta - (90 * PI)/180) < MAX_DEGREE_ROTATION_ALLOWED ||
            ABS(theta - (180 * PI)/180) < MAX_DEGREE_ROTATION_ALLOWED ||
            ABS(theta - (270 * PI)/180) < MAX_DEGREE_ROTATION_ALLOWED)
    {
        if (DEBUG) cout<< "Changing angle by " << theta * 180 / PI << " degrees" << endl;
        predictor::answer *result = new predictor::answer;
        result->primitive = predictor::LINE;
        result->segments = 4;
        result->path = new GLfloat[4];
        result->path[0] = start.x;
        result->path[1] = start.y;
        result->path[2] = end.x;
        result->path[3] = end.y;

        result->prev = new predictor::prev_work;
        copy_prev_work_struct(prev, result->prev);
        result->prev->clamp_line_slope = true;

        theta = theta * 180 / PI;
        MAX_DEGREE_ROTATION_ALLOWED = MAX_DEGREE_ROTATION_ALLOWED * 180 / PI;
        if (ABS(theta) < MAX_DEGREE_ROTATION_ALLOWED){
            result->path[3] = result->path[1]; // +x axis
            if(prev != NULL && prev->clamp_point2) return NULL;
            result->prev->clamp_point2 = true;
        }
        if (ABS(theta - 90) < MAX_DEGREE_ROTATION_ALLOWED ){
            result->path[2] = result->path[0]; // +y axis
            if(prev != NULL && prev->clamp_point2) return NULL;
            result->prev->clamp_point2 = true;
        }
        if (ABS(theta - 180) < MAX_DEGREE_ROTATION_ALLOWED ){
            result->path[3] = result->path[1]; // -x axis
            if(prev != NULL && prev->clamp_point1) return NULL;
            result->prev->clamp_point1 = true;
        }
        if (ABS(theta - 270) < MAX_DEGREE_ROTATION_ALLOWED ){
            result->path[2] = result->path[0]; // -y axis
            if(prev != NULL && prev->clamp_point1) return NULL;
            result->prev->clamp_point1 = true;
        }
        result->score = MAX(0, 1 - (ABS(theta)/MAX_DEGREE_ROTATION_ALLOWED));
        result->score = MIN(result->score, 1 + 1 - (ABS(theta - 90)/MAX_DEGREE_ROTATION_ALLOWED));
        result->score = MIN(result->score, 1 + 1 - (ABS(theta - 180)/MAX_DEGREE_ROTATION_ALLOWED));
        result->score = MIN(result->score, 1 + 1 - (ABS(theta - 270)/MAX_DEGREE_ROTATION_ALLOWED));

        if (result->score < -10) result->score = -(result->score/10);
        else if (result->score > -2 && result->score < 0) result->score = ABS(result->score);
        else if (result->score > 0 && result->score < 1) result ->score= 2 - result->score;

        result->score += (prev == NULL) ? 0 : prev->score;

        return result;
    }
    return NULL;
}

/*
pair<point*, point*> straight_line_least_error(GLfloat *points_arr, int num_points){
    // TODO: unimplemented.
    GLfloat STEPPING_DIST = 0.05;
    vector<point> points;
    GLfloat mean_x = 0, mean_y = 0;
    for(int i = 0; i < num_points - 1; i++){
        point p(points_arr[2*i], points_arr[2*i + 1]);
        points.push_back(p);
        mean_x += p.x; mean_y += p.y;
    }
    mean_x /= points.size(); mean_y /= points.size();
    return pair<point*, point*>(NULL, NULL);
}
*/

void interpolate_points(point p1, point p2, vector<GLfloat> x, vector<GLfloat> y){
    double MINIMUM_DIST = 0.001;
    point mid((p1.x + p2.x)/2, (p1.y + p2.y)/2);
    if (predictor::euclid_dist(p1, p2) > MINIMUM_DIST)
    {
        interpolate_points(p1, mid, x, y);
        interpolate_points(mid, p2, x, y);
    }
    x.push_back(mid.x);
    y.push_back(mid.y);
}

predictor::answer *calculate_straight_line_score(int num_points, GLfloat *points, vector<GLint> prev_types, vector< vector<GLfloat> > prev_obj, int depth, bool nested=false, predictor::prev_work *prev=NULL){
    // TODO: add score to the answer according to the threshold.

    bool DEBUG = false;
    float STRAIGHT_LINE_THRESHOLD = 0.90;

    double straight_line_sum = predictor::euclid_dist(
        points[0], points[1],
        points[2*(num_points-1)], points[2 * (num_points-1) + 1]);

    double added_sum = 0;
    for(int i = 0; i < num_points - 1; i ++)
    {
        added_sum += predictor::euclid_dist(points[2*i], points[2*i+1],
                                 points[2*(i+1)], points[2*(i+1) + 1]);
    }

    if (DEBUG)
    {
        std::cout << "Straight line sum is" << straight_line_sum << std::endl;
        std::cout << "Added line sum is" << added_sum << std::endl;
        std::cout << "Ratio is " << (ABS(straight_line_sum / added_sum)) << std::endl;
    }

    if (ABS(straight_line_sum / added_sum) > STRAIGHT_LINE_THRESHOLD)
    {
        vector<predictor::answer *> choices;

        if (DEBUG) cout<< "Predicted is  a Straight Line" << endl;

        predictor::answer *constraint_parallel = constraint_straight_line_parallel(
                    point(points[0], points[1]),
                    point(points[2*(num_points-1)], points[2*(num_points-1) + 1]),
                    prev_types,
                    prev_obj,
                    nested, prev
                );
        choices.push_back(constraint_parallel);

       predictor::answer *constraint_axes = constraint_straight_line_axes(
                    point(points[0], points[1]),
                    point(points[2*(num_points-1)], points[2*(num_points-1) + 1]),
                    nested, prev
                   );
       choices.push_back(constraint_axes);

//       predictor::answer *ans = NULL;
//       if (constraint_axes != NULL && constraint_parallel != NULL) {
//           ans = constraint_axes->score > constraint_parallel->score ? constraint_axes : constraint_parallel;
//       } else if (constraint_axes == NULL && constraint_parallel != NULL) {
//            ans = constraint_parallel;
//       } else if (constraint_axes != NULL){
//            ans = constraint_axes;
//       }

//       point *start = NULL;
//       point *end = NULL;
//       if (ans == NULL){
//            start = new point(points[0], points[1]);
//            end = new point(points[2*(num_points-1)], points[2*(num_points-1) + 1]);
//       } else {
//            start = new point(ans->path[0], ans->path[1]);
//            end = new point(ans->path[2], ans->path[3]);
//       }

       point *start = new point(points[0], points[1]),
             *end = new point(points[2*(num_points-1)], points[2*(num_points-1)+1]);

       predictor::answer *snapping_answer = constraint_straight_line_endpoint_snapping(
                   *start, *end,
                   prev_types, prev_obj,
                   nested, prev
                   );
       choices.push_back(snapping_answer);

       if (depth >= 1){
           if (choices.size() == 0) return NULL;
           predictor::answer *best = choices[0];
           GLfloat best_score = best == NULL ? 0 : best->score;
           for(uint i = 0; i < choices.size(); i++)
           {
               if (choices[i] == NULL) continue;
               if (choices[i]->score > best_score){
                   best_score = choices[i]->score;
                   best = choices[i];
               }
           }
           if (best != NULL) return best;
           predictor::answer *result = new predictor::answer;
           result->primitive = predictor::LINE;
           result->segments = 4;
           result->path = new GLfloat[4];
           result->path[0] = points[0];
           result->path[1] = points[1];
           result->path[2] = points[2*(num_points-1)];
           result->path[3] = points[2*(num_points-1) + 1];
           result->score = 0;

           return result;
       }
       vector<predictor::answer *> inner_answers;
       for(uint i = 0; i < choices.size(); i++)
       {
           if (choices[i] == NULL) continue;
           GLfloat *new_points = new GLfloat[num_points*2];
           for(int i = 0; i < num_points - 1; i++) { new_points[2*i] = points[2*i]; new_points[2*i + 1] = points[2*i + 1];}
           predictor::answer *current = choices[i];
           new_points[0] = current->path[0];
           new_points[1] = current->path[1];
           new_points[2*num_points - 2] = current->path[2];
           new_points[2*num_points - 1] = current->path[3];
           predictor::answer *inner = calculate_straight_line_score(num_points, new_points, prev_types, prev_obj, depth + 1, true, current->prev);
           if (inner == NULL){
               if (DEBUG) cout << "What" << endl;
           } else inner_answers.push_back(inner);
       }

//       if (snapping_answer != NULL)
//       {
           /*
            if (constraint_axes != NULL)
            {
                return constraint_straight_line_axes(
                            point(snapping_answer->path[0], snapping_answer->path[1]),
                            point(snapping_answer->path[2], snapping_answer->path[3]));
            }
            */
//           return snapping_answer;
//       }
//       if (ans != NULL) return ans;

       if (inner_answers.size() != 0){
           predictor::answer *best = inner_answers[0];
           GLfloat best_score = best == NULL ? 0 : best->score;
           for(int i = 1; i < inner_answers.size(); i++)
           {
               if (inner_answers[i] == NULL) continue;
               if(inner_answers[i]->score > best_score){
                   best = inner_answers[i];
                   best_score = best->score;
               }
           }
           if(best != NULL) return best;
       }

        predictor::answer *result = new predictor::answer;
        result->primitive = predictor::LINE;
        result->segments = 4;
        result->path = new GLfloat[4];
        result->path[0] = points[0];
        result->path[1] = points[1];
        result->path[2] = points[2*(num_points-1)];
        result->path[3] = points[2*(num_points-1) + 1];

        if (DEBUG) cout << "End points (" << result->path[0] << "," << result->path[1]
                        << "), (" << result->path[2] << "," << result->path[3] << ")" << endl;

        return result;
    }
    return NULL;
}

predictor::answer *calculate_circle_score_approach1(int num_points, GLfloat *coords, bool interpolate, bool median=false){
    // TODO add score/weight.

    // takes the mean/median of all the points to find the centre.
    // finds the radius using the mean of distance of centre from all the points.

    point center(0, 0);
    vector<point> points;
    vector<GLfloat> x_points;
    vector<GLfloat> y_points;

    for (int i = 0; i < num_points; ++i)
    {
        points.push_back(point(coords[2*i], coords[2*i+1]));
    }
    for (uint i = 0; i < points.size(); ++i)
    {
        if (interpolate)
        {
            interpolate_points(points[i],
                               i == points.size() - 1 ? points[0]: points[i + 1],
                               x_points, y_points);
        }
        x_points.push_back(points[i].x);
        y_points.push_back(points[i].y);
    }
    if (x_points.size() != y_points.size()) cout << "SHOULD NEVER HAPPEN" << endl;

    if (median)
    {
        sort(x_points.begin(), x_points.begin() + x_points.size());
        sort(y_points.begin(), y_points.begin() + y_points.size());
        center.x = x_points[x_points.size() / 2];
        center.y = y_points[y_points.size() / 2];
    }
    else
    {
        for (uint i = 0; i < x_points.size(); ++i)
        {
            center.x += x_points[i];
            center.y += y_points[i];
        }
        center.x /= x_points.size();
        center.y /= y_points.size();
    }

    float radius = 0;
    for(uint i = 0; i < x_points.size(); ++i)
    {
        radius += predictor::euclid_dist(
                    center.x, center.y,
                    x_points[i], y_points[i]);
    }
    radius /= x_points.size();
    point circum(center.x + radius, center.y);

    predictor::answer* result = new predictor::answer;
    result->primitive = predictor::CIRCLE;
    result->segments = 4;
    result->path = new GLfloat[4];
    result->path[0] = center.x;
    result->path[1] = center.y;
    result->path[2] = circum.x;
    result->path[3] = circum.y;
    return result;
}

predictor::answer *calculate_circle_score_approach2(int num_points, GLfloat *coords){
    // TODO add score/weight.

    // creates cluster of all intersection points of normals from each point,
    // and tries to estimate the centre from that.
    // finds the radius using the mean of distance of centre from all the points.

    bool DEBUG = false;

    vector<line> point_normals;
    vector<point> points;

    for (int i = 0; i < num_points - 1; ++i) {
        point p1(coords[2*i], coords[2*i+1]);
        point p2(coords[2*(i+1)], coords[2*(i+1) + 1]);
        point_normals.push_back(
                    *p1.getTangent(p2)->getPerpendicularLineFrom(p1)
        );
        points.push_back(p1);
        if (i == num_points - 2) points.push_back(p2);
    }

    vector<point> center_points;
    for (uint i = 0; i < point_normals.size(); ++i) {
        for (uint j = i+1; j < point_normals.size(); ++j) {
            if (i == j) continue;
            line normal1 = point_normals[i];
            line normal2 = point_normals[j];
            center_points.push_back(
                        *normal1.getPointOfIntersection(normal2)
                        );
        }
    }
    if (DEBUG)
    {
        cout << "Size" << point_normals.size() << endl;
        cout << "SIZE" << center_points.size() << endl;
    }

    point center(0, 0);

    for(uint i = 0; i < center_points.size(); i++){
        point p = center_points[i];
        if (isinf(p.x) || isinf(p.y) || isnan(p.x) || isnan(p.y)) continue;
        // SHOULD I DO THIS?
        if (ABS(p.x) > 1 || ABS(p.y) > 1) continue;
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= center_points.size();
    center.y /= center_points.size();

    float radius = 0;
    for(uint i = 0; i < points.size(); ++i)
    {
        radius += predictor::euclid_dist(center, points[i]);
    }
    radius /= points.size();
    point circum(center.x + radius, center.y);

    predictor::answer* result = new predictor::answer;
    result->primitive = predictor::CIRCLE;
    result->segments = 4;
    result->path = new GLfloat[4];
    result->path[0] = center.x;
    result->path[1] = center.y;
    result->path[2] = circum.x;
    result->path[3] = circum.y;
    return result;
}

void change_answer_for_concentric_circles(predictor::answer *prev_answer, vector<GLint> prev_types, vector< vector<GLfloat> > prev_obj)
{
    GLfloat THRESHOLD_DIST = 0.1;
    bool DEBUG = false;
    point *selected_center = NULL;
    GLfloat min_dist = 1000;
    point given_center = point(prev_answer->path[0], prev_answer->path[1]);
    for(uint i = 0; i < prev_obj.size(); i++)
    {
        if (prev_types[i] == 2)
        {
            point *p = new point(prev_obj[i][0], prev_obj[i][1]);
            if (selected_center == NULL){
                selected_center = p;
                min_dist = predictor::euclid_dist(*p, given_center);
            }
            else{
                GLfloat dist = predictor::euclid_dist(*p, given_center);
                if (dist < min_dist){
                    if (selected_center != NULL) { free(selected_center); }
                    selected_center = p;
                    min_dist = dist;
                }
            }
        }
    }

    if (selected_center != NULL){
        if (DEBUG) cout << "Least distance between centres: Old circle(" << selected_center->x << "," << selected_center->y << ")" <<
                           " new circle(" << prev_answer->path[0] << "," << prev_answer->path[1] << "). Distance: " << min_dist << endl;
        if (min_dist < THRESHOLD_DIST){
            prev_answer->path[0] = selected_center->x;
            prev_answer->path[1] = selected_center->y;
        };
    }
}

predictor::answer *calculate_circle_score(int num_points, GLfloat *points, int method, vector<GLint> prev_types, vector< vector<GLfloat> > prev_obj)
{
    bool DEBUG = false;
    GLfloat CIRCLE_ENDPOINT_SLOPE_THRESHOLD = 1;
    GLfloat CIRCLE_PATH_RATIO_THRESHOLD = 2;

    point start(points[0], points[1]);
    point end(points[2 * (num_points - 1)], points[2 * (num_points - 1) + 1]);

    point start_neighbour(points[2], points[3]);
    point end_neighbour(points[2*(num_points-2)], points[2*(num_points-2) + 1]);

    line start_tangent = *(start.getTangent(start_neighbour));
    line end_tangent = *(end.getTangent(end_neighbour));

    GLfloat distance = predictor::euclid_dist(start, end);

    // TODO: instead of comparing slopes, compare angle between lines by taking atan;
    if (ABS(start_tangent.m - end_tangent.m) < CIRCLE_ENDPOINT_SLOPE_THRESHOLD && distance < 0.5)
    {
        GLfloat complete_path_distance = 0;
        for(int i = 0; i < num_points - 1; i++)
        {
            complete_path_distance += predictor::euclid_dist(points[2*i], points[2*i+1],
                                 points[2*(i+1)], points[2*(i+1) + 1]);
        }
        GLfloat path_ratio = (GLfloat)ABS(complete_path_distance/distance);

        if (path_ratio > CIRCLE_PATH_RATIO_THRESHOLD || path_ratio==0)
        {
            predictor::answer *result;
            if (method == 1)
                result = calculate_circle_score_approach1(num_points, points, false);
            if (method == 2)
                result = calculate_circle_score_approach1(num_points, points, false, true);
            if (method == 3)
                result = calculate_circle_score_approach2(num_points, points);
            if (method == 1 || method == 2 || method == 3)
            {
                change_answer_for_concentric_circles(result, prev_types, prev_obj);
                return result;
            }
            else
            {
                cout << "You should call only methods 1, 2, 3";
                return NULL;
            }
        }
    }
    return NULL;
}

predictor::answer *predictor::analyze(){
    ARC_THRESHOLD = 0;
    int ARC_SEGMENTS = 500;


    int MINIMUM_POINTS = 100;
    sample_points_into(MINIMUM_POINTS, &this->points, this->num_points);
    GLfloat *sample_points = this->points; // remove at the end when everything is in methods.
    int num = MIN(num_points, MINIMUM_POINTS);

    // first check if its a straight line;
    answer *straight_line_result = calculate_straight_line_score(num, this->points, this->prev_types, this->prev_objects, depth);
//    if (straight_line_result != NULL) results->push_back(straight_line_result);
    if (straight_line_result != NULL) return straight_line_result;

    // Let's check whether it's a circle now.
    answer *circle_result = calculate_circle_score(num, this->points, 1, this->prev_types, this->prev_objects);
//    if (circle_result != NULL) results->push_back(circle_result);
    if (circle_result != NULL) return circle_result;

    circle_result = calculate_circle_score(num, this->points, 2, this->prev_types, this->prev_objects);
    if (circle_result != NULL) results->push_back(circle_result);

    circle_result = calculate_circle_score(num, this->points, 3, this->prev_types, this->prev_objects);
    if (circle_result != NULL) results->push_back(circle_result);

    cout << "----___---" << endl;
    for (uint i = 0; i < results->size(); ++i) {
        answer *ans = (*results)[i];
        cout << ans->primitive << endl;
        cout << ans->path[0] << " "
             << ans->path[1] << " "
             << ans->path[2] << " "
             << ans->path[3] << endl;

    }

    // Let's check for arcs now.
    point p1(points[0], points[1]);
    point p2(points[2 * (num_points - 1)], points[2 * (num_points - 1) + 1]);

    point p1_neighbour(points[2], points[3]);
    point p2_neighbour(points[2*(num_points-2)], points[2*(num_points-2) + 1]);

    line p1_normal = *(p1.getTangent(p1_neighbour)->getPerpendicularLineFrom(p1));
    line p2_normal = *(p2.getTangent(p2_neighbour)->getPerpendicularLineFrom(p2));

    point center = *(p1_normal.getPointOfIntersection(p2_normal));
    double radius = euclid_dist(p1, center);

    // There maybe wasn't any need to compute this.
    // angle difference in degrees between the 2 normals. They'll meet at the center.
    // double angle_difference = (atan(p1_normal.m) * 180/PI) - (atan(p2_normal.m) * 180/PI);

    double starting_angle = -1*atan(p1_normal.m) * 180/PI;
    double ending_angle = atan(p2_normal.m) * 180/PI;

    cout<<"------------------------------"<<endl;
    cout<<starting_angle<<endl;
    cout<<ending_angle<<endl;

    cout<<"------------------------------"<<endl;


    GLfloat *result_path = new GLfloat[ARC_SEGMENTS * 2];
    double angle_decrement_amount = (starting_angle - ending_angle)/ARC_SEGMENTS;
    double angle = starting_angle;
    for(int i = 0; i < ARC_SEGMENTS; i++)
    {
        GLfloat m = tan((angle * PI/180));
        GLfloat c = center.y - m*center.x;
        line segment_to_center(m, c);
        point *circle_point = segment_to_center.getPointOnLineLyingDistanceAway(radius, center);
        result_path[2 * i] = circle_point->x;
        result_path[2 * i + 1] = circle_point->y;
        angle -= angle_decrement_amount;
    }

    answer* result = new answer;
    result->primitive = predictor::ARC;
    result->segments = ARC_SEGMENTS;
    result->path = result_path;

    return result;
}
