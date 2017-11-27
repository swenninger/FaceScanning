#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "KinectGrabber.h"

namespace Ui {
class MainWindow;
}

#include <LandmarkCoreIncludes.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void OnStartRecordRequested(bool);
    void OnStopRecordRequested(bool);
    void OnPlaySequenceRequested(bool);
    void OnFrameReady(CapturedFrame);
    void OnFacetrackingCheckboxToggled(bool);

private:
    Ui::MainWindow *ui;

    KinectGrabber* grabber;


    QLabel* colorDisplay;

    bool capturing;
    bool doFaceTracking;

    LandmarkDetector::FaceModelParameters det_parameters;
    LandmarkDetector::CLNF clnf_model;

    int counter;

};

#endif // MAINWINDOW_H
