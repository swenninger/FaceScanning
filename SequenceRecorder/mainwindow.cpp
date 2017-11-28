#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "KinectGrabber.h"

#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDirIterator>
#include <QCheckBox>

// #include  <FaceTracker/Tracker.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    det_parameters(),
    clnf_model("../../data/model/main_clnf_general.txt")
{

    ui->setupUi(this);

    grabber = new KinectGrabber();

    QVBoxLayout* layout = new QVBoxLayout();

    QPushButton* startRecordingButton = new QPushButton("Start recording");
    QPushButton* stopRecordingButton  = new QPushButton("Stop Recording");
    QPushButton* playSequenceButton = new QPushButton("Play recorded Sequence");
    QCheckBox* faceTrackingCheckbox = new QCheckBox("Do Face Tracking");
    faceTrackingCheckbox->setChecked(false);

    colorDisplay = new QLabel();
    colorDisplay->setMinimumHeight(300);

    layout->addWidget(startRecordingButton);
    layout->addWidget(stopRecordingButton);
    layout->addWidget(playSequenceButton);
    layout->addWidget(faceTrackingCheckbox);
    layout->addWidget(colorDisplay);

    QWidget* widget = new QWidget();
    widget->setLayout(layout);
    this->ui->gridLayout->addWidget(widget);

    grabber->ConnectToKinect();
    grabber->StartStream();

    QObject::connect(startRecordingButton, SIGNAL(clicked(bool)), this, SLOT(OnStartRecordRequested(bool)));
    QObject::connect(stopRecordingButton, SIGNAL(clicked(bool)), this, SLOT(OnStopRecordRequested(bool)));
    QObject::connect(playSequenceButton, SIGNAL(clicked(bool)), this, SLOT(OnPlaySequenceRequested(bool)));

    QObject::connect(faceTrackingCheckbox, SIGNAL(toggled(bool)), this, SLOT(OnFacetrackingCheckboxToggled(bool)));

    QObject::connect(grabber, SIGNAL(FrameReady(CapturedFrame)), this, SLOT(OnFrameReady(CapturedFrame)));

    capturing = false;
    doFaceTracking = false;

    counter = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OnStartRecordRequested(bool)
{
    capturing = true;
}

void MainWindow::OnStopRecordRequested(bool)
{
    capturing = false;
}

#include <LandmarkCoreIncludes.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

double fps_tracker = -1.0;
int64 t0 = 0;
// Visualising the results
void visualise_tracking(cv::Mat& captured_image, const LandmarkDetector::CLNF& face_model, const LandmarkDetector::FaceModelParameters& det_parameters, cv::Point3f gazeDirection0, cv::Point3f gazeDirection1, int frame_count, double fx, double fy, double cx, double cy)
{

    // Drawing the facial landmarks on the face and the bounding box around it if tracking is successful and initialised
    double detection_certainty = face_model.detection_certainty;
    bool detection_success = face_model.detection_success;

    double visualisation_boundary = 0.2;

    // Only draw if the reliability is reasonable, the value is slightly ad-hoc
    if (detection_certainty < visualisation_boundary)
    {
        LandmarkDetector::Draw(captured_image, face_model);

        double vis_certainty = detection_certainty;
        if (vis_certainty > 1)
            vis_certainty = 1;
        if (vis_certainty < -1)
            vis_certainty = -1;

        vis_certainty = (vis_certainty + 1) / (visualisation_boundary + 1);

        // A rough heuristic for box around the face width
        int thickness = (int)std::ceil(2.0* ((double)captured_image.cols) / 640.0);

        cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetPose(face_model, fx, fy, cx, cy);

        // Draw it in reddish if uncertain, blueish if certain
        LandmarkDetector::DrawBox(captured_image, pose_estimate_to_draw, cv::Scalar((1 - vis_certainty)*255.0, 0, vis_certainty * 255), thickness, fx, fy, cx, cy);

        if (det_parameters.track_gaze && detection_success && face_model.eye_model)
        {
//            GazeAnalysis::DrawGaze(captured_image, face_model, gazeDirection0, gazeDirection1, fx, fy, cx, cy);
        }
    }

    // Work out the framerate
    if (frame_count % 10 == 0)
    {
        double t1 = cv::getTickCount();
        fps_tracker = 10.0 / (double(t1 - t0) / cv::getTickFrequency());
        t0 = t1;
    }

    // Write out the framerate on the image before displaying it
    char fpsC[255];
    std::sprintf(fpsC, "%d", (int)fps_tracker);
    string fpsSt("FPS:");
    fpsSt += fpsC;
    cv::putText(captured_image, fpsSt, cv::Point(10, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0));

    if (!det_parameters.quiet_mode)
    {
        cv::namedWindow("tracking_result", 1);
        cv::imshow("tracking_result", captured_image);
    }
}


void MainWindow::OnPlaySequenceRequested(bool)
{
    QDir dir("..\\..\\FaceScanKinect\\data\\recordings_small");
    QStringList entries = dir.entryList(QDir::NoFilter, QDir::Name);


    for (int i = 0; i < entries.size(); ++i) {
        QString path = entries.at(i);
        QPixmap pixmap = QPixmap(600, 338);
        bool loaded = pixmap.load(dir.absoluteFilePath(path));

     //   cv::Mat captured_image(pixmap.height(), pixmap.width(), CV_8UC3, pixmap.toImage().bits());
        cv::Mat captured_image = cv::imread(dir.absoluteFilePath(path).toStdString());
        // Reading the images
        cv::Mat_<uchar> grayscale_image;

        if(captured_image.channels() == 3)
        {
            cv::cvtColor(captured_image, grayscale_image, CV_BGR2GRAY);
        }
        else
        {
            grayscale_image = captured_image.clone();
        }

        qInfo() << grayscale_image.cols << grayscale_image.rows;

        bool detection_success = false;
        if (grayscale_image.cols > 0 && grayscale_image.rows > 0) {
            detection_success = LandmarkDetector::DetectLandmarksInVideo(grayscale_image, clnf_model, det_parameters);
            float fx, fy, cx, cy;
            cx = captured_image.cols / 2.0f;
            cy = captured_image.rows / 2.0f;
            fx = 500 * (captured_image.cols / 640.0);
            fy = 500 * (captured_image.rows / 480.0);

            fx = (fx + fy) / 2.0;
            fy = fx;
            visualise_tracking(captured_image, clnf_model, det_parameters, cv::Point3f(), cv::Point3f(), 0, fx, fy, cx, cy);
        }


        // qInfo() << path;
        qInfo() << detection_success;

        colorDisplay->setPixmap(pixmap);

        QApplication::processEvents();

        QTime dieTime= QTime::currentTime().addMSecs(5);
           while (QTime::currentTime() < dieTime)
               QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    }
}

void MainWindow::OnFrameReady(CapturedFrame frame)
{
    // Draw reference image

    int height = colorDisplay->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage(frame.colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    colorDisplay->setPixmap(pixmap.scaledToHeight(height));

    if (doFaceTracking) {
        cv::Mat cv_image = cv::Mat(COLOR_HEIGHT, COLOR_WIDTH, CV_8UC4, frame.colorBuffer);

        cv::resize(cv_image, cv_image, cv::Size(0,0), 0.3, 0.3);

        cv::Mat_<uchar> grayscale_image;

        if (cv_image.channels() == 4) {
            cv::cvtColor(cv_image, grayscale_image, CV_RGBA2GRAY);
        } else if(cv_image.channels() == 3) {
            cv::cvtColor(cv_image, grayscale_image, CV_BGR2GRAY);
        } else {
            grayscale_image = cv_image.clone();
        }


        if (grayscale_image.cols > 0 && grayscale_image.rows > 0) {
            bool detection_success = LandmarkDetector::DetectLandmarksInVideo(grayscale_image, clnf_model, det_parameters);
            float fx, fy, cx, cy;
            cx = cv_image.cols / 2.0f;
            cy = cv_image.rows / 2.0f;
            fx = 500 * (cv_image.cols / 640.0);
            fy = 500 * (cv_image.rows / 480.0);

            fx = (fx + fy) / 2.0;
            fy = fx;
            visualise_tracking(cv_image, clnf_model, det_parameters, cv::Point3f(), cv::Point3f(), counter, fx, fy, cx, cy);
        }

        counter++;
    }


    // Save frames to disk

    if (capturing) {

        bool success = pixmap.save("C:\\Users\\studproject-mirror\\Documents\\Masterarbeit-Stephan\\FaceScanning\\FaceScanKinect\\data\\recordings\\" + QString::number(counter) + ".bmp", "BMP");

        qInfo() << "Capture success: " << success;

        counter++;
    }
}

void MainWindow::OnFacetrackingCheckboxToggled(bool checked)
{
    doFaceTracking = checked;
    counter = 0;
}
