#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QLabel>
#include "glwidget.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_actionExit_2_triggered();

    void on_actionExit_triggered();

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_actionUndo_triggered();

    void fileSave();

    void on_actionRedo_triggered();

    void on_actionYes_triggered();

    void on_actionNo_triggered();

    void on_toolButton_clicked();

    void on_toolButton_2_clicked();

    void on_toolButton_3_clicked();

private:
    Ui::MainWindow *ui;
    GLwidget *Screen;
};

#endif // MAINWINDOW_H
