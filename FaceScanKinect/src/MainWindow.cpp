#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "KinectGrabber.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    kinectGrabber = new KinectGrabber(ui->colorLabel);

    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();
}

MainWindow::~MainWindow()
{
    delete ui;
}
