#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QFileDialog>
#include<QTextDocumentWriter>
#include <fstream>
#include <iostream>
#include<QBuffer>
#include "glwidget.h"
#include<QPlainTextEdit>
using namespace std;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::fileSave()
    {
        QString fileName=QFileDialog::getSaveFileName(this, tr("Save file"));
       if (fileName.isEmpty())
           return;
       Screen = ui->screen;

       ofstream ofs("a.bmp", ios::binary);

       ofs.write((char *)Screen, sizeof(*Screen));
       ofs.write((char *)Screen, sizeof(*Screen));
       ofs.close();
}



void MainWindow::on_actionExit_2_triggered()
{
    exit(0);
}

void MainWindow::on_actionExit_triggered()//clear all
{
    GLwidget gl;
    gl.setFlag(0);
}

void MainWindow::on_actionLoad_triggered()
{
    QString fileName= QFileDialog::getOpenFileName(this, tr("Load file"));
    if (fileName.isEmpty())
        return;
    Screen = ui->screen;
    Screen->setFlag(0);
    ifstream ifs(fileName.toLatin1().data(), ios::binary);
    ifs.read((char *)(Screen),sizeof(*Screen));
    ifs.close();
}

void MainWindow::on_actionSave_triggered()
{
    fileSave();

}

void MainWindow::on_actionUndo_triggered()
{
    GLwidget gl;
    gl.setUndo(1);
}

void MainWindow::on_actionRedo_triggered()
{
    GLwidget gl;
    gl.setRedo(1);
}

void MainWindow::on_actionYes_triggered()
{
    GLwidget gl;
    gl.setNoise(1);
}

void MainWindow::on_actionNo_triggered()
{
    GLwidget gl;
    gl.setNoise(0);
}

void MainWindow::on_toolButton_clicked()
{
    GLwidget gl;
    gl.setUndo(1);
}

void MainWindow::on_toolButton_2_clicked()
{
    GLwidget gl;
    gl.setRedo(1);
}

void MainWindow::on_toolButton_3_clicked()
{
    GLwidget gl;
    gl.setFlag(0);
}
