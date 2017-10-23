#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "KinectGrabber.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void DisplayColorFrame(uchar* colorBuffer);
    void DisplayDepthFrame(uchar* depthBuffer);

private:
    Ui::MainWindow *ui;

    KinectGrabber* kinectGrabber;
};

#endif // MAINWINDOW_H
