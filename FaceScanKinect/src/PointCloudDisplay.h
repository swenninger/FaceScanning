#ifndef POINTCLOUDDISPLAY_H
#define POINTCLOUDDISPLAY_H

#include <QOpenGLWidget>

class PointCloudDisplay : public QOpenGLWidget
{
public:
    PointCloudDisplay();

    void setData();

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

};

#endif // POINTCLOUDDISPLAY_H
