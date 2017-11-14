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

    void SetData(Vec3f* p, RGB3f *c, size_t size);
    void SetData(Vec3f *p, RGB3f *c, Vec3f* n, size_t size);
    void ComputeNormals(PointCloud pc);

public slots:
    void ColoredPointsSettingChanged(int state);
    void NormalsComputed();

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
    Vec3f *currentPoints;
    RGB3f *currentColors;
    Vec3f *currentNormals;

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

#endif // POINTCLOUDDISPLAY_H
