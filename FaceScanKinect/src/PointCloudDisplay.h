#ifndef POINTCLOUDDISPLAY_H
#define POINTCLOUDDISPLAY_H

#include <QtOpenGL>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

#include "util.h"


class PointCloudDisplay : public QOpenGLWidget
{
    Q_OBJECT

public:
    PointCloudDisplay();

    void SetData(PointCloud pc);
    void SetData(Vec3f* p, RGB3f *c, size_t size);
    void SetData(Vec3f *p, RGB3f *c, Vec3f* n, size_t size);
    void ComputeNormals(PointCloud pc);
    void FilterPointcloud(PointCloud pc, size_t numNeighbors = 50, float stddevMultiplier = 1.0f);
    void RefilterPointcloud(size_t numNeighbors = 50, float stddevMultiplier = 1.0f);

public slots:
    void ColoredPointsSettingChanged(int state);
    void NormalsComputed();
    void PointcloudFiltered();

protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;

    // Camera Controls
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateCameraFromAngles();

    // Data to display
    size_t numPoints;
    Vec3f* currentPoints;
    RGB3f* currentColors;
    RGB3f* colorBackup;
    Vec3f* currentNormals;


    bool drawColoredPoints;
    bool drawNormals;

    // OpenGL Buffers and Shaders
    bool buffersInitialized;
    GLuint pointCloudVAO;
    GLuint normalDebugVAO;
    GLuint colorBuffer;
    GLuint pointBuffer;
    GLuint normalBuffer;

    // Drawing the pointcloud
    QOpenGLShaderProgram* pointCloudProgram;
    int projMatrixLoc;
    int mvMatrixLoc;
    int drawColoredPointsLoc;

    // Drawing normals for debugging
    QOpenGLShaderProgram* normalDebugProgram;
    int normalsProjMatLoc;
    int normalsMVMatLoc;

    // Camera Setup
    void InitializeCamera();
    QMatrix4x4 proj;
    QMatrix4x4 modelView;

    QVector3D cameraPosition;
    QVector3D cameraDirection;
    QVector3D cameraRight;
    QVector3D cameraUp;

    float yaw;
    float pitch;

    QPoint lastMousePoint;
    bool   cameraControlRequested;
};

/**
 * @brief The ComputeNormalWorker class computes normal and emits signal when it is finished.
 *
 * It is supposed to be run by a thread, that listens to the finished signal
 */
class ComputeNormalWorker : public QObject
{
    Q_OBJECT

public:
    ComputeNormalWorker(PointCloud pc, Vec3f* out_normals) {
        this->pc = pc;
        this->out_normals = out_normals;
    }

    ~ComputeNormalWorker() {}

public slots:
    void ComputeNormals();

signals:
    void finished();

protected:

private:
    PointCloud pc;
    Vec3f* out_normals;
};

/**
 * @brief The FilterPointcloudWorker class filters the pointcloud and emits singal when it is finished.
 *
 * It is supposed to be run by a thread, that listens to the finished signal
 */
class FilterPointcloudWorker : public QObject
{
    Q_OBJECT

public:
    FilterPointcloudWorker(PointCloud pc_, size_t numNeighbors_, float stddevMultiplier_) {
        this->pc = pc_;
        this->numNeighbors = numNeighbors_;
        this->stddevMultiplier = stddevMultiplier_;
    }

    ~FilterPointcloudWorker() {}

public slots:
    void FilterPointcloud();

signals:
    void finished();

protected:

private:
    PointCloud pc;

    size_t numNeighbors;
    float  stddevMultiplier;
};


#endif // POINTCLOUDDISPLAY_H
