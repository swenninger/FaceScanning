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
public:
    PointCloudDisplay();

    void setData(Vec3f* p, RGB3f *c, size_t size);

protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;

    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;


private:
    bool buffersInitialized;

    size_t numPoints;
    Vec3f *currentPoints;
    RGB3f *currentColors;

    GLuint vao;
    GLuint colorBuffer;
    GLuint pointBuffer;

    QOpenGLShaderProgram* program;
    int projMatrixLoc;
    int mvMatrixLoc;

    QMatrix4x4 proj;
    QMatrix4x4 modelView;

    QVector3D cameraPosition;
    QVector3D cameraDirection;
    QVector3D cameraRight;
    float horizontalAngle;
    float verticalAngle;

    QPoint lastMousePoint;
    bool   cameraControlRequested;
};

#endif // POINTCLOUDDISPLAY_H
