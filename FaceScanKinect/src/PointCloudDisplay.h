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
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

private:
    size_t numPoints;
    Vec3f *currentPoints;
    RGB3f *currentColors;

    int xRot;
    int yRot;
    int zRot;
    QPoint lastPos;
    //QOpenGLVertexArrayObject vao;
    GLuint vao;
    GLuint colorBuffer;
    GLuint pointBuffer;
    //QOpenGLBuffer colorBuffer;
    //QOpenGLBuffer pointBuffer;
    QOpenGLShaderProgram* program;
    int projMatrixLoc;
    int mvMatrixLoc;
    int normalMatrixLoc;
    int lightPosLoc;
    QMatrix4x4 proj;
    QMatrix4x4 camera;
    QMatrix4x4 world;

    QMatrix4x4 modelView;
};

#endif // POINTCLOUDDISPLAY_H
