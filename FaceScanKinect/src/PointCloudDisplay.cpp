#include "PointCloudDisplay.h"

#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions>
#include <QtMath>

float cameraSpeed = 0.1f;

static float Clamp(float min, float val, float max) {
    float Result = val;
    if (val < min) {
        Result = min;
    } else if (val > max) {
        Result = max;
    }
    return Result;
}

PointCloudDisplay::PointCloudDisplay()
{
    this->setFocusPolicy(Qt::StrongFocus);

    buffersInitialized     = false;
    cameraControlRequested = false;
    drawColoredPoints      = true;

    numPoints     = 0;
    currentColors = nullptr;
    currentPoints = nullptr;
}

void PointCloudDisplay::setData(Vec3f *p, RGB3f *c, size_t size)
{
    numPoints = size;
    currentPoints = p;
    currentColors = c;

    if (buffersInitialized) {
        makeCurrent();
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

        f->glBindBuffer(GL_ARRAY_BUFFER, this->pointBuffer);
        f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentPoints, GL_STATIC_DRAW);

        f->glBindBuffer(GL_ARRAY_BUFFER, this->colorBuffer);
        f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentColors, GL_STATIC_DRAW);
    }

    update();
}

void PointCloudDisplay::ColoredPointsSettingChanged(int state)
{
    if (state == Qt::Unchecked) {
        drawColoredPoints = false;
    }  else {
        drawColoredPoints = true;
    }
    update();
}

static const char* vertexShader =
    "#version 400\n"
    "layout(location = 0) in vec3 vertex_position;\n"
    "layout(location = 1) in vec3 vertex_colour;\n"
    "out vec3 colour;\n"
    "uniform bool drawColoredPoints;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "  if (drawColoredPoints) { colour = vertex_colour; } else { colour = vec3(0.6, 0.6, 0.6); } \n"
    "  // colour = vertex_colour;\n"
    "  gl_Position = projMatrix * mvMatrix * vec4(vertex_position, 1.0);\n"
    "}\n";

static const char* fragmentShader =
    "#version 400\n"
    "in vec3 colour;\n"
    "out vec4 frag_colour;\n"
    "void main() {\n"
    "  frag_colour = vec4(colour, 1.0);\n"
    "}\n";

void PointCloudDisplay::initializeGL()
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    program = new QOpenGLShaderProgram();
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
    program->bindAttributeLocation("vertex_position", 0);
    program->bindAttributeLocation("vertex_colour", 1);
    program->link();

    program->bind();
    projMatrixLoc   = program->uniformLocation("projMatrix");
    mvMatrixLoc     = program->uniformLocation("mvMatrix");
    drawColoredPointsLoc = program->uniformLocation("drawColoredPoints");

    // normalMatrixLoc = program->uniformLocation("normalMatrix");
    // lightPosLoc     = program->uniformLocation("lightPos");

    pointBuffer = 0;
    f->glGenBuffers(1, &pointBuffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentPoints, GL_STATIC_DRAW);

    colorBuffer = 0;
    f->glGenBuffers(1, &colorBuffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentColors, GL_STATIC_DRAW);

    f->glGenVertexArrays(1, &vao);
    f->glBindVertexArray(vao);

    buffersInitialized = true;

    InitializeCamera();
}

void PointCloudDisplay::InitializeCamera()
{
    // Projection matrix settings for kinect
    proj.setToIdentity();
    proj.perspective(70.6f, 512 / (GLdouble)424, 0.01f, 1000);

    cameraPosition  = QVector3D(0, 0.25f, 0.4f);
    cameraDirection = QVector3D(0, 0, 1);
    cameraRight     = QVector3D(1, 0, 0);
    cameraUp        = QVector3D(0, 1, 0);

    yaw   = 90.0f;
    pitch = 0.0f;
}

void PointCloudDisplay::paintGL()
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    if (numPoints == 0) { return; }

#if 0
    qInfo()
            << "Pos: " << cameraPosition
            << "Dir: " << cameraDirection
            << "Up : " << cameraUp
            << "R  : " << cameraRight
    ;
#endif

    // Setup Camera
    modelView.setToIdentity();
    modelView.lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp);

    // Flip horizontally
    modelView.scale(-1, 1 ,1);

    f->glBindVertexArray(vao);
    f->glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    f->glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool bound = program->bind();

    if (bound) {
        program->setUniformValue(projMatrixLoc, proj);
        program->setUniformValue(mvMatrixLoc, modelView);
        program->setUniformValue(drawColoredPointsLoc, drawColoredPoints);
        f->glBindVertexArray(vao);
        // NOTE: maybe change pointsize
        // f->glPointSize(2.0f);
        f->glDrawArrays(GL_POINTS, 0, (GLsizei)numPoints);
        program->release();
    }
}

void PointCloudDisplay::resizeGL(int w, int h)
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glViewport(0, 0, w, h);
}

void PointCloudDisplay::keyPressEvent(QKeyEvent *event)
{
    boolean updateWidget = true;
    boolean angleChanged = false;

    switch (event->key()) {
        case Qt::Key_W:         cameraPosition += cameraDirection * cameraSpeed; break;
        case Qt::Key_S:         cameraPosition -= cameraDirection * cameraSpeed; break;
        case Qt::Key_A:         cameraPosition -= cameraRight * cameraSpeed; break;
        case Qt::Key_D:         cameraPosition += cameraRight * cameraSpeed; break;
        case Qt::Key_PageDown:  cameraPosition -= cameraUp * cameraSpeed; break;
        case Qt::Key_PageUp:    cameraPosition += cameraUp * cameraSpeed; break;

        case Qt::Key_1:         InitializeCamera(); break;

        case Qt::Key_Left:    yaw -= 30.0f * cameraSpeed; angleChanged = true; break;
        case Qt::Key_Right:   yaw += 30.0f * cameraSpeed; angleChanged = true; break;
        case Qt::Key_Up:    pitch += 30.0f * cameraSpeed; angleChanged = true; break;
        case Qt::Key_Down:  pitch -= 30.0f * cameraSpeed; angleChanged = true; break;

        default:
            // No camera update needed
            updateWidget = false;
            break;
    }

    if (angleChanged) { updateCameraFromAngles(); }
    if (updateWidget) { update(); }

    QOpenGLWidget::keyPressEvent(event);
}

void PointCloudDisplay::wheelEvent(QWheelEvent *event)
{
    cameraPosition += cameraDirection * ((float)event->delta() / 300.0f);
    update();

    QOpenGLWidget::wheelEvent(event);
}

void PointCloudDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        lastMousePoint = event->pos();
        cameraControlRequested = true;
    }
    QOpenGLWidget::mousePressEvent(event);
}

void PointCloudDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        cameraControlRequested = false;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void PointCloudDisplay::mouseMoveEvent(QMouseEvent *event)
{    
    if (cameraControlRequested) {
        QPoint diff = event->pos() - lastMousePoint;
        lastMousePoint = event->pos();

        yaw   += (float)diff.x() * cameraSpeed;
        pitch -= (float)diff.y() * cameraSpeed; // y has to flipped
        pitch = Clamp(-89.f, pitch, 89.f);      // Prevent screen flip

        updateCameraFromAngles();
        update();
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void PointCloudDisplay::updateCameraFromAngles() {
    float yawRadians   = qDegreesToRadians(yaw);
    float pitchRadians = qDegreesToRadians(pitch);

    // Angles to 3D Vector
    cameraDirection = QVector3D(
                qCos(yawRadians) * qCos(pitchRadians),
                qSin(pitchRadians),
                qSin(yawRadians) * qCos(pitchRadians)).normalized();

    cameraRight = QVector3D::crossProduct(cameraDirection, QVector3D(0,1,0)).normalized();
    cameraUp    = QVector3D::crossProduct(cameraRight, cameraDirection).normalized();
}
