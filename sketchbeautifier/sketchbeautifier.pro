#-------------------------------------------------
#
# Project created by QtCreator 2015-11-22T19:31:32
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sketchbeautifier
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    glwidget.cpp \
    line.cpp \
    point.cpp \
    predictor.cpp \
    shader_utils.cpp

HEADERS  += mainwindow.h \
    glwidget.h \
    line.h \
    point.h \
    predictor.h \
    shader_utils.h

FORMS    += mainwindow.ui

DISTFILES += \
    rectangle.f.glsl \
    rectangle.v.glsl \
    triangle.f.glsl \
    triangle.v.glsl


LIBS+= -lGL \
    -lGLEW


QMAKE_CXXFLAGS += -std=c++0x

RESOURCES +=


