#define _USE_MATH_DEFINES

#include "glwidget.h"
#include<stdio.h>
#include<qevent.h>
#include<qapplication.h>
#include<math.h>
#include<iostream>
#include<vector>
#include<predictor.h>
#include<QGraphicsSceneMouseEvent>
#include "shader_utils.h"

using namespace std;

GLwidget::GLwidget(QWidget *parent) :
    QGLWidget(parent)
{
}

//some openGL basic variables

GLuint vbo_triangle;
GLuint vbo_triangle_colors;
GLuint program;
GLuint attribute_coord2d;
GLint attribute_v_color;

int widthl=0;
int heightl=0;

int point=4;

int shapeNum=0;//indicate the type of primitive drawn

int flagClear=0;// to clear the buff - drawing area components

int i=0;//iterates over traingles array

GLfloat triangle_vertices[100000];// array to store points taken from user
GLfloat triangle_colors[100000];// array to store colors taken from user

//vector < pair<GLint,vector <GLfloat> > >v;

vector <GLint> v;// array to indicate what primitive is it on the basis of the shapeNum assigned . shapenum 1 is a line, shapenum 2 is a circle, shapenum 5 is an arc
vector < vector<GLfloat> > v1;// array for storing points which are user drawn on the drawing area

vector <GLint> popped_v;
vector < vector<GLfloat> > popped_v1;

float colR=0.0;
float colG=0.0;
float colB=0.0;

int shouldfill=0;// for coloring
int outerVertexCount;// used to running drawcircle and drawArc and it indicates count of vertices received of circle

//values as boolean
int lineprint=0;
int circleprint=0;
int polygonprint=0;

int lastPointx=0;
int lastPointy=0;

int showNoise=1;// if we need to show noise i.e. user drawn lines

int enter=1;// to check if the drawCircle enters only when required
int enterp=0;// to check if the drawPOLYGON enters only when required
int enterA=1;// to check if the drawArc enters only when required
int vertices=0;// count of vertices captured by mouse

int undo=0;// to indicate if user undoed a step 0 means not undoed
int redo=0;

int l=0;// to rack vertices in color array of triangle_vertices

int mouseReleased=0;// to indicte if mouse is released crrently or not
int mouseMoving=0;// to indicte if mouse is moving crrently or not

float startVertexX,startVertexY,endVertexX,endVertexY; // for the predictor class to set points

//vector <pair<QPoint *,QPoint *>> pointsLine ;

void GLwidget::setUndo(int undoval){
    undo=undoval;

}

void GLwidget::setRedo(int redoval){
    redo=redoval;

}

void GLwidget::setNoise(int noiseVal){
    showNoise=noiseVal;

}


void GLwidget::initializeGL(){

    GLenum glew_status = glewInit();
        if (glew_status != GLEW_OK)
        {
          fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));

        }
        GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

        GLuint vs,fs;
        vs = create_shader("triangle.v.glsl",GL_VERTEX_SHADER);
        fs = create_shader("triangle.f.glsl",GL_FRAGMENT_SHADER);


        program = glCreateProgram();
          glAttachShader(program, vs);
          glAttachShader(program, fs);
          glLinkProgram(program);
          glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
          if (!link_ok) {
            fprintf(stderr, "glLinkProgram:");
            print_log(program);
          }

          const char* attribute_name = "coord2d";
              attribute_coord2d = glGetAttribLocation(program, attribute_name);
          attribute_name = "v_color";
          attribute_v_color = glGetAttribLocation(program, attribute_name);
          if (attribute_v_color == -1) {
            fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
            return;
          }
}

void GLwidget::paintGL() {

        glClearColor(256.0, 1.0, 1.0, 1.0);

        glUseProgram(program);
        glGenBuffers(1, &vbo_triangle);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

        //glGenBuffers(1, &vbo_triangle_colors);
        //glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_colors);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);

        glEnableVertexAttribArray(attribute_coord2d);
        glEnableVertexAttribArray(attribute_v_color);

        /* Describe our vertices array to OpenGL (it can't guess its format automatically) */
        glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
        glVertexAttribPointer(
          attribute_coord2d, // attribute
          2,                 // number of elements per vertex, here (x,y)
          GL_FLOAT,          // the type of each element
          GL_FALSE,          // take our values as-is
          0,                 // no extra data between each position
          0                  // offset of first element
        );


        paintNow();

       if(flagClear==0){
            glClear(GL_COLOR_BUFFER_BIT);
            flagClear=1;
            //following clears the v and v1 array
            v.clear();
            v1.clear();
        }
        glDisableVertexAttribArray(attribute_coord2d);
        glDeleteProgram(program);
        glDeleteBuffers(1, &vbo_triangle);
}

GLfloat centreX,centreY,circumX,circumY;// TRACK CENTRE AND POINT ON CIRCUMFERENCE OF A CIRCLE
GLfloat *arcArr ;

int number;

void  GLwidget::drawLines()
{

        glBegin(GL_LINE_STRIP);
        glColor3f(1.0,0.0,0.0);
        for(int j=0;j<vertices;j=j+2)
        {
            glVertex2f(triangle_vertices[j],triangle_vertices[j+1]);

        }

        glEnd();
        update();
}

void GLwidget::predict(){

        predictor pred;
        pred.setup(triangle_vertices,vertices/2, v, v1);
        predictor::answer answer;
        if(mouseReleased==1){


            answer= *(pred.analyze());
              if(answer.primitive == predictor::LINE)
              {

                  shapeNum=1;
                  vector< GLfloat  >arr;

                  startVertexX= answer.path[0];
                  startVertexY =  answer.path[1];

                  endVertexX = answer.path[2];
                  endVertexY = answer.path[3];
                  // ------THIS----------------------

//                  cout<<"Snnappp"<< startVertexX<< " "<<startVertexY<<" " <<endVertexX<< " "<<endVertexY<<endl;

                  arr.push_back(startVertexX);
                  arr.push_back(startVertexY);
                  arr.push_back(endVertexX);
                  arr.push_back(endVertexY);
                  v.push_back(1);
                  v1.push_back(arr);

              }
              else if (answer.primitive == predictor::ARC)
              {
                  cout << "Predicted ARC" << endl;
                  shapeNum = 5;
                    arcArr= answer.path;
                  number = answer.segments;
                  vector <GLfloat> ar;
                  for(int k=0;k<number*2;k++)
                  {
                  ar.push_back(arcArr[k]);}
                  v.push_back(5);
                  v1.push_back(ar);

                  i=0;l=0;
              }
              else if (answer.primitive == predictor::CIRCLE)
              {
//                  cout << "Predicted Circle" << endl;

                  shapeNum = 2;
                  GLfloat *arr = answer.path;
                  vector< GLfloat> arr1;
                  arr1.push_back(arr[0]);
                  arr1.push_back(arr[1]);
                  arr1.push_back(arr[2]);
                  arr1.push_back(arr[3]);
//                  cout << "CENTER" << arr1[0] << " " << arr1[1] << " " << arr1[2] << " " << arr1[3] << endl;
                  v.push_back(2);
                  v1.push_back(arr1);


                  centreX=arr[0];
                  centreY=arr[1];
                  circumX=arr[2];
                  circumY=arr[3];
                   i=0;l=0;

              }

              if(showNoise ==0)
                paintAll();

              mouseReleased=0;
        }

}

void  GLwidget::drawArc(GLfloat *arr,int numbers)
{


    float buffer[numbers*2];
   int idx = 0;
    //shape number 5
//        i=0;l=0;
//        glBegin(GL_LINE_STRIP);
//        glColor3f(1.0,0.0,0.0);
//        for(int j=0;j<numbers;j=j+2)
//        {
//            glVertex2f(arr[j],arr[j+1]);
//        }
//        glEnd();

    for(int z=0;z<numbers*2;z++){

     buffer[idx++] = arr[z];
        }

//    cout<<buffer[0]<<endl;


    glGenBuffers(1, &vbo_triangle);
   glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
   glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(attribute_coord2d);

        glVertexAttribPointer(
                attribute_coord2d,      // attribute
                2,                      // number of elements per vertex, here (r,g,b)
                GL_FLOAT,               // the type of each element
                GL_FALSE,               // take our values as-is
                0 ,
                0  // offset of first element
        );
    glDrawArrays(GL_LINE_LOOP, 0, numbers*2);

    glDisableVertexAttribArray(attribute_coord2d);

}

void GLwidget::paintAll(){

    glClear(GL_COLOR_BUFFER_BIT); flagClear=1;
    for (int it = 0 ; it < v.size();it++){
        GLint vx;
        vx=v[it];

//        cout<<vx<<endl;

        if(vx==1)//LINE
        {

            i=0;
            triangle_vertices[i++]=v1[it][0];
            triangle_vertices[i++]=v1[it][1];
            triangle_vertices[i++]=v1[it][2];
            triangle_vertices[i++]=v1[it][3];

            drawLine();
             update();
             circleprint=1;
            i=0;l=0;
             vertices=0;
        }


        if(vx==2 )//CIRCLE
        {
            i=0;
            triangle_vertices[i++]=v1[it][0];
            triangle_vertices[i++]=v1[it][1];
            triangle_vertices[i++]=v1[it][2];
            triangle_vertices[i++]=v1[it][3];
            enter=0;

                drawCircle();
                 update();
                 circleprint=1;
                i=0;l=0;
                 vertices=0;

        }
        if(vx!=2) { circleprint=0;enter=1;}

        if(vx==5){

            /*GLfloat *ar;
            for(int h=0;h<number*2;h++)
                ar[h]=v1[it][h];*/


             // drawArc(ar,number);
        }


        update();
    }


}

int popCounter= 0; //Keeps a count of how many pops are there as of now


// This functions is caled by the paintGL when the activity has to be displayed on the screen
//The mouse events get the points which are plotted by this function which calls other other drawing functions
void GLwidget::paintNow(){

    if(undo==1 && v.size()!=0 ){//if user request to undo something


        popped_v.push_back(v[v.size()-1]);

        popped_v1.push_back(v1[v1.size()-1]);

        v.pop_back();
        v1.pop_back();

        popCounter++;
        undo =0;
        paintAll();


    }

    if(redo==1 && popped_v.size()!=0 ){//if user request to redo something

        v.push_back(popped_v[popCounter-1]);
        v1.push_back(popped_v1[popCounter-1]);

        popped_v.pop_back();
        popped_v1.pop_back();
        popCounter--;
        redo =0;
        paintAll();


    }
    update();
    // we predict any shape only when the mouse is realeased

    if(mouseReleased==1){
        predict();
    }

    // show noise is just a paramemter to show the line which the user is drawing simultaneously
    if(showNoise==1){

        // assigned shapenumbes to different primitives
        //Shapenum 1 is a line, shapenum 2 is acircle, shapenum 5 is an arc
        if(shapeNum==1)//LINE
            {

                i=0;

                //get all the vertices that have been stored by the predictor in array v1
                triangle_vertices[i++]=v1[v1.size()-1][0];
                triangle_vertices[i++]=v1[v1.size()-1][1];
                triangle_vertices[i++]=v1[v1.size()-1][2];
                triangle_vertices[i++]=v1[v1.size()-1][3];

//                cout<<triangle_vertices[i-1]<<endl;

                if(i==4 )// if number of points captured are two then that means you have end and starting points sufficient to draw a line
                   {

                    drawLine();// function to draw actual line with predicted set of points
                    update();// update screen/drawing area
                    lineprint=1;
                    i=0;l=0;
                    vertices=0;
                }
                else
                    lineprint=0; //tells if line has been printed or not, used to set vertice coount(i) equal to 0 to get more opints in nxt iteration
            }


            if(shapeNum!=1)  lineprint=0;


            if(shapeNum==2 && enter==1 )//CIRCLE
            {

                i=0;
                triangle_vertices[i++]=centreX;
                triangle_vertices[i++]=centreY;
                triangle_vertices[i++]=circumX;
                triangle_vertices[i++]=circumY;
                enter=0;

                 if(i==4 )
                   {
                     drawCircle(); // function to draw actual circle with predicted set of points centre and point on circumference
                     update();
                     circleprint=1; //tells if circle has been printed or not, used to set vertice coount(i) equal to 0 to get more points in nxt iteration
                     update();

                     i=0;l=0;
                     vertices=0;
                }
                else
                   { circleprint=0; enter=1;}

            }
            if(shapeNum!=2)  circleprint=0;

            if(shapeNum==5 && enterA==1 )//ARC
            {
                    drawArc(arcArr,number);
                     update();
                }
                else
                   {enterA=1;}



            if(shapeNum!=5)  enterA=1;

          }

            shapeNum=0;// setting shapenum to 0 as initial value so that it does not enter into another loop before prediction is made
}

void GLwidget::resizeGL(int w,int h){
    widthl = w;
    heightl = h;
    glViewport(0,0,w,h);
}

//This helps in drawing the points
void GLwidget::drawpoints(){

     if(enter==0){

    glGenBuffers(1, &vbo_triangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(attribute_coord2d);

    glVertexAttribPointer(
            attribute_coord2d,      // attribute
            2,                      // number of elements per vertex, here (r,g,b)
            GL_FLOAT,               // the type of each element
            GL_FALSE,               // take our values as-is
            0 ,
            0  // offset of first element
    );

     glDrawArrays(GL_POINTS,0,2);
     glDisableVertexAttribArray(attribute_coord2d);

     enter=1;

     }

}

//This helps in drawing the line with correct number of points as passed by the predictor

void GLwidget::drawLine(){

   triangle_colors[l++]= colR;
   triangle_colors[l++]= colG;
   triangle_colors[l++]= colB;
   triangle_colors[l++]= colR;
   triangle_colors[l++]= colG;
   triangle_colors[l++]= colB;

   glGenBuffers(1, &vbo_triangle);
   glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
   glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);
   glEnableVertexAttribArray(attribute_coord2d);

   glVertexAttribPointer(
           attribute_coord2d,      // attribute
           2,                      // number of elements per vertex, here (r,g,b)
           GL_FLOAT,               // the type of each element
           GL_FALSE,               // take our values as-is
           0 ,
           0  // offset of first element
   );


    glDrawArrays(GL_LINES,0,2);
    glDisableVertexAttribArray(attribute_coord2d);
}

//This helps in drawing the circle with correct number of points as passed by the predictor
void GLwidget::drawCircle(){

    if(enter==0){

        int vertexCount = 21;
        float buffer[vertexCount*2]; // (x,y) for each vertex

        float center_x= triangle_vertices[0];
        float  center_y=triangle_vertices[1];
       int idx = 0;

        float radius=sqrt(pow(triangle_vertices[3]-triangle_vertices[1],2)+pow(triangle_vertices[2]-triangle_vertices[0],2));
        //outer vertices of the circle
        outerVertexCount = vertexCount-1;
        l=0;
        for (int i = 0; i < outerVertexCount+1; i++){
        float percent = (i / (float) (outerVertexCount-1));
       float rad = percent * 2*(22/7);
        float outer_x = (center_x + radius * cos(rad));// x coord of point on circumference
        float outer_y = center_y + radius * sin(rad);

        triangle_colors[l++]= colR;
        triangle_colors[l++]= colG;
        triangle_colors[l++]= colB;

         buffer[idx++] = outer_x;
         buffer[idx++] = outer_y;
        }

       glGenBuffers(1, &vbo_triangle);
       glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle);
       glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

       glEnableVertexAttribArray(attribute_coord2d);

       glVertexAttribPointer(
            attribute_coord2d,      // attribute
            2,                      // number of elements per vertex, here (r,g,b)
            GL_FLOAT,               // the type of each element
            GL_FALSE,               // take our values as-is
            0 ,
            0  // offset of first element
    );


//        if(shouldfill==10){
//            glGenBuffers(1, &vbo_triangle_colors);
//            glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_colors);
//            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);
//            glEnableVertexAttribArray(attribute_v_color);

//            glVertexAttribPointer(
//                    attribute_v_color,      // attribute
//                    3,                      // number of elements per vertex, here (r,g,b)
//                    GL_FLOAT,               // the type of each element
//                    GL_FALSE,               // take our values as-is
//                    0 ,
//                    0  // offset of first element
//            );


//        }

//        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);

//        if(shouldfill==1)
//        {
//            glGenBuffers(1, &vbo_triangle_colors);
//            glBindBuffer(GL_ARRAY_BUFFER, vbo_triangle_colors);
//            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);
//            glEnableVertexAttribArray(attribute_v_color);

//            glVertexAttribPointer(
//                    attribute_v_color,      // attribute
//                    3,                      // number of elements per vertex, here (r,g,b)
//                    GL_FLOAT,               // the type of each element
//                    GL_FALSE,               // take our values as-is
//                    0 ,
//                    0  // offset of first element
//            );


//            glDrawArrays(GL_TRIANGLE_FAN, 0,outerVertexCount);


//         }

        glDrawArrays(GL_LINE_LOOP, 0, outerVertexCount);

        glDisableVertexAttribArray(attribute_coord2d);

    enter=1;

    }


}

int countPoints=0;

void GLwidget::mousePressEvent(QMouseEvent* e)
{

    startVertexX=(float)(((float)(e->x())-(float)(widthl/2))/(float)(widthl/2));
    startVertexY=-1*(float)(((float)(e->y())-(float)(heightl/2))/(float)(heightl/2));
    mouseReleased=0;

   // qDebug("Debug: '%s'", "Mouse Pressed");
    triangle_vertices[i++]= (float)(((float)(e->x())-(float)(widthl/2))/(float)(widthl/2));
    triangle_vertices[i++]=-1*(float)(((float)(e->y())-(float)(heightl/2))/(float)(heightl/2));

   //  paintNow();

}

void GLwidget::mouseMoveEvent(QMouseEvent* e)
{
    //qDebug("Debug: '%s'", "Mouse Move");

    triangle_vertices[i++]= (float)(((float)(e->x())-(float)(widthl/2))/(float)(widthl/2));
    triangle_vertices[i++]=-1*(float)(((float)(e->y())-(float)(heightl/2))/(float)(heightl/2));
    mouseMoving=1;
    vertices=i;
    drawLines();
}

void GLwidget::mouseReleaseEvent(QMouseEvent* e)
{
    mouseReleased=1;
    //qDebug("Debug: '%s'", "Mouse Released");

    endVertexX=(float)(((float)(e->x())-(float)(widthl/2))/(float)(widthl/2));
    endVertexY=-1*(float)(((float)(e->y())-(float)(heightl/2))/(float)(heightl/2));

    triangle_vertices[i++]= (float)(((float)(e->x())-(float)(widthl/2))/(float)(widthl/2));
    triangle_vertices[i++]=-1*(float)(((float)(e->y())-(float)(heightl/2))/(float)(heightl/2));

    if(lineprint==1 )
        {i=0;l=0;}
    if(circleprint==1 || polygonprint==1)
        {i=0;l=0;}

    //saving number of points total
    vertices=i;
    mouseMoving=0;

    paintNow();

}

void GLwidget::setFlag(int fl)//to set if we wanna clear the screen
{
    flagClear=fl;
}



