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
    void OnStartRecordRequested(bool);
    void OnStopRecordRequested(bool);
    void OnFrameReady(CapturedFrame);

private:
    Ui::MainWindow *ui;

    KinectGrabber* grabber;


    QLabel* colorDisplay;

    bool capturing;

    int counter;

};

#endif // MAINWINDOW_H
