#ifndef GLWIDGET_H
#define GLWIDGET_H
#include <GL/glew.h>
#include <QGLWidget>
#include <QMouseEvent>
#include<QString>
#include<QGraphicsSceneMouseEvent>

class GLwidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLwidget(QWidget *parent = 0);

   void initializeGL();
   void paintGL();
   void resizeGL(int w,int h);

   void setUndo(int undo);
   void setRedo(int redo);
   void setNum(int setNu);
   void setColor(QString setCol);
   void setFlag(int fl);
   void setNoise(int noiseVal);

   void mousePressEvent(QMouseEvent* );
    void drawLines();// to display user drawn lines

   void mouseMoveEvent(QMouseEvent* );
    void setPoints(int num);
   void mouseReleaseEvent(QMouseEvent* );

   void predict();

   void drawLine();
   void drawArc(GLfloat *arr,int number);
   void drawpoints();
   void drawCircle();
   void paintNow();
   void paintAll();


signals:

public slots:

};

#endif // GLWIDGET_H
