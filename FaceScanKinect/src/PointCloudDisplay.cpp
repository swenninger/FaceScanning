#include "PointCloudDisplay.h"

#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions>

PointCloudDisplay::PointCloudDisplay()
{
    this->setFocusPolicy(Qt::ClickFocus);
    buffersInitialized = false;
    numPoints = 0;
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

static const char* vertexShader =
    "#version 400\n"
    "layout(location = 0) in vec3 vertex_position;\n"
    "layout(location = 1) in vec3 vertex_colour;\n"
    "out vec3 colour;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "  colour = vertex_colour;\n"
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
    cameraPosition  = QVector3D(0, 0, -1);
    cameraDirection = QVector3D(0, 0,  1);
    cameraRight     = QVector3D(1, 0,  0);

    horizontalAngle = 0.0f;
    verticalAngle   = 0.0f;

}

void PointCloudDisplay::paintGL()
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    if (numPoints == 0) { return; }

    proj.setToIdentity();
    proj.perspective(45, 512 / (GLdouble)424, 0.1, 1000);

    modelView.setToIdentity();

    QVector3D up = QVector3D::crossProduct(cameraRight, cameraDirection);
    modelView.lookAt(cameraPosition, cameraPosition + cameraDirection, up);

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
        f->glBindVertexArray(vao);
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
    float cameraSpeed = 0.1f;

    switch (event->key()) {
    case Qt::Key_W:
        cameraPosition += cameraDirection * cameraSpeed;
        update();
        break;

    case Qt::Key_S:
        cameraPosition -= cameraDirection * cameraSpeed;
        update();
        break;

    case Qt::Key_A:
        cameraPosition -= cameraRight * cameraSpeed;
        update();
        break;

    case Qt::Key_D:
        cameraPosition += cameraRight * cameraSpeed;
        update();
        break;

    case Qt::Key_1:
        // Reset Camera
        InitializeCamera();
        update();
        break;

    default:
        break;
    }
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

        horizontalAngle += (float)diff.x() / 300.0f;
        verticalAngle   -= (float)diff.y() / 300.0f;

        QMatrix4x4 mat;
        mat.setToIdentity();
        mat.rotate(horizontalAngle, 0.0f, 1.0f);
        mat.rotate(verticalAngle,   1.0f, 0.0f);

        QVector4D camdir = mat * QVector4D(0.0f, 0.0f, 1.0f, 1.0f);
        QVector4D camright = mat * QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
        cameraDirection = camdir.toVector3D();
        cameraRight     = camright.toVector3D();

        /*
        float cosineVerticalAngle = cos(verticalAngle);
cameraDirection = QVector3D(cosineVerticalAngle * sin(horizontalAngle),
                                    sin(verticalAngle),
                                    cosineVerticalAngle * cos(horizontalAngle));
        cameraRight = QVector3D(sin(horizontalAngle - M_PI_2),
                                0.0f,
                                cos(horizontalAngle - M_PI_2));
        */
        update();
    }
    QOpenGLWidget::mouseMoveEvent(event);
}
