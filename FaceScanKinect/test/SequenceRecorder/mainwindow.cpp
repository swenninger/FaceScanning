#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "KinectGrabber.h"

#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    grabber = new KinectGrabber();

    QVBoxLayout* layout = new QVBoxLayout();

    QPushButton* startRecordingButton = new QPushButton("Start recording");
    QPushButton* stopRecordingButton  = new QPushButton("Stop Recording");

    colorDisplay = new QLabel();
    colorDisplay->setMinimumHeight(300);

    layout->addWidget(startRecordingButton);
    layout->addWidget(stopRecordingButton);
    layout->addWidget(colorDisplay);

    QWidget* widget = new QWidget();
    widget->setLayout(layout);
    this->ui->gridLayout->addWidget(widget);

    grabber->ConnectToKinect();
    grabber->StartStream();

    QObject::connect(startRecordingButton, SIGNAL(clicked(bool)), this, SLOT(OnStartRecordRequested(bool)));
    QObject::connect(stopRecordingButton, SIGNAL(clicked(bool)), this, SLOT(OnStopRecordRequested(bool)));

    QObject::connect(grabber, SIGNAL(FrameReady(CapturedFrame)), this, SLOT(OnFrameReady(CapturedFrame)));

    capturing = false;

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

void MainWindow::OnFrameReady(CapturedFrame frame)
{
    // Draw reference image

    int height = colorDisplay->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage(frame.colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    colorDisplay->setPixmap(pixmap.scaledToHeight(height));

    // Save frames to disk

    if (capturing) {

        bool success = pixmap.save("C:\\Users\\studproject-mirror\\Documents\\Masterarbeit-Stephan\\FaceScanning\\FaceScanKinect\\data\\recordings\\" + QString::number(counter) + ".bmp", "BMP");

        qInfo() << "Capture success: " << success;

        counter++;
    }
}
