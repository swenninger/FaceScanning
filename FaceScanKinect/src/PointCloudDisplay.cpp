﻿#include "PointCloudDisplay.h"

#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions>
#include <QtMath>

float moveSpeed = 0.01f;
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
    drawNormals = false;

    numPoints      = 0;
    currentColors  = nullptr;
    colorBackup    = nullptr;
    currentPoints  = nullptr;
    currentNormals = nullptr;
}

void PointCloudDisplay::SetData(Vec3f *p, RGB3f *c, size_t size)
{
    numPoints = size;
    currentPoints = p;
    currentColors = c;

    drawNormals = false;

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

void PointCloudDisplay::SetData(Vec3f *p, RGB3f *c, Vec3f* n, size_t size)
{
    numPoints = size;
    currentPoints = p;
    currentColors = c;
    currentNormals = n;

    if (buffersInitialized) {
        makeCurrent();
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

        f->glBindBuffer(GL_ARRAY_BUFFER, this->pointBuffer);
        f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentPoints, GL_STATIC_DRAW);

        f->glBindBuffer(GL_ARRAY_BUFFER, this->colorBuffer);
        f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentColors, GL_STATIC_DRAW);

        f->glBindBuffer(GL_ARRAY_BUFFER, this->normalBuffer);
        f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentNormals, GL_STATIC_DRAW);
    }

    drawNormals = true;
    update();
}

void PointCloudDisplay::SetData(PointCloudBuffer *pointcloudBuffer, bool normalsComputed)
{
    if (normalsComputed) {
        SetData(pointcloudBuffer->points, pointcloudBuffer->colors, pointcloudBuffer->normals, pointcloudBuffer->numPoints);
    } else {
        SetData(pointcloudBuffer->points, pointcloudBuffer->colors, pointcloudBuffer->numPoints);
    }
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

//   Point Cloud Shaders

static const char* pointCloudVS = R"SHADER_STRING(
    #version 400
    layout(location = 0) in vec3 vertex_position;
    layout(location = 1) in vec3 vertex_colour;
    out vec3 colour;
    out float should_discard;
    uniform bool drawColoredPoints;
    uniform mat4 projMatrix;
    uniform mat4 mvMatrix;

    void main() {
        if (drawColoredPoints) { colour = vertex_colour; } else { colour = vec3(0.6, 0.6, 0.6); }

        if (vertex_colour == vec3(1.0, 1.0, 1.0)) {
            should_discard = 1.0f;
        } else {
            should_discard = 0.0;
        }
        gl_Position = projMatrix * mvMatrix * vec4(vertex_position, 1.0);
    }

)SHADER_STRING";

static const char* pointCloudFS = R"SHADER_STRING(
    #version 400
    in vec3 colour;
    in float should_discard;
    out vec4 frag_colour;

    void main() {
        if (should_discard > 0.5) {
            discard;
        }
        frag_colour = vec4(colour, 1.0);
    }
)SHADER_STRING";

// Normal Visualization Shader

static const char* normalVisVS = R"SHADER_STRING(
    #version 400
    layout(location = 0) in vec3 vertex_position;
    layout(location = 1) in vec3 vertex_color;
    layout(location = 2) in vec3 vertex_normal;

    out vec3 pos;
    out vec3 normal;
    out vec4 color;

    uniform mat4 projMatrix;
    uniform mat4 mvMatrix;

    void main() {
        color  = vec4(vertex_color, 1.0);
        // color  = vec4(1.0, 0.4, 0.4, 1.0);
        normal = vertex_normal;
        pos    = vertex_position;
        gl_Position = projMatrix * mvMatrix * vec4(vertex_position, 1.0);
    }

)SHADER_STRING";

static const char* normalVisGS = R"SHADER_STRING(
    #version 400
    layout(points) in;
    layout(line_strip, max_vertices = 2) out;

    in vec3[] normal;
    in vec4[] color;
    in vec3[] pos;

    uniform mat4 projMatrix;
    uniform mat4 mvMatrix;

    out vec4 fcolor;

    void main() {
        fcolor = color[0];
        fcolor = vec4(normalize(normal[0])* 0.5f + 0.5f, 1.0f);


        gl_Position = projMatrix * mvMatrix * vec4(pos[0], 1.0);
        EmitVertex();

        gl_Position = projMatrix * mvMatrix * vec4(pos[0] + 0.015 * normal[0], 1.0);
      //  gl_Position = projMatrix * mvMatrix * vec4(pos[0], 1.0);
        EmitVertex();

        EndPrimitive();
    }

)SHADER_STRING";

static const char* normalVisFS = R"SHADER_STRING(
    #version 400
    in vec4 fcolor;
    out vec4 outcolor;

    void main () {
        outcolor = fcolor;
    }
)SHADER_STRING";

void PointCloudDisplay::initializeGL()
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);

    pointCloudProgram = new QOpenGLShaderProgram();
    pointCloudProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, pointCloudVS);
    pointCloudProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, pointCloudFS);
    pointCloudProgram->bindAttributeLocation("vertex_position", 0);
    pointCloudProgram->bindAttributeLocation("vertex_colour", 1);
    pointCloudProgram->link();

    pointCloudProgram->bind();
    projMatrixLoc   = pointCloudProgram->uniformLocation("projMatrix");
    mvMatrixLoc     = pointCloudProgram->uniformLocation("mvMatrix");
    drawColoredPointsLoc = pointCloudProgram->uniformLocation("drawColoredPoints");


    normalDebugProgram = new QOpenGLShaderProgram();
    normalDebugProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,   normalVisVS);
    normalDebugProgram->addShaderFromSourceCode(QOpenGLShader::Geometry, normalVisGS);
    normalDebugProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, normalVisFS);
    normalDebugProgram->bindAttributeLocation("vertex_position", 0);
    normalDebugProgram->bindAttributeLocation("vertex_colour", 1);
    normalDebugProgram->bindAttributeLocation("vertex_normal", 2);
    normalDebugProgram->link();

    normalDebugProgram->bind();
    normalsProjMatLoc = normalDebugProgram->uniformLocation("projMatrix");
    normalsMVMatLoc   = normalDebugProgram->uniformLocation("mvMatrix");

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

    normalBuffer = 0;
    f->glGenBuffers(1, &normalBuffer );
    f->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, numPoints * 3, currentNormals, GL_STATIC_DRAW);

    f->glGenVertexArrays(1, &pointCloudVAO);
    f->glBindVertexArray(pointCloudVAO);

    f->glGenVertexArrays(1, &normalDebugVAO);
    f->glBindVertexArray(normalDebugVAO);

    buffersInitialized = true;

    InitializeCamera();
}

void PointCloudDisplay::InitializeCamera()
{
    // Projection matrix settings for kinect
    proj.setToIdentity();
    proj.perspective(70.6f, 512 / (GLfloat)424, 0.01f, 1000);

    cameraPosition  = QVector3D(0, 0.25f, 0.4f);
    cameraDirection = QVector3D(0, 0, 1);
    cameraRight     = QVector3D(-1, 0, 0);
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

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup Camera
    modelView.setToIdentity();
    modelView.lookAt(cameraPosition, cameraPosition + cameraDirection, cameraUp);

    // Flip horizontally
    modelView.scale(-1, 1 ,1);

    f->glBindVertexArray(pointCloudVAO);
    f->glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    if (drawNormals) {
        f->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    } else {
        f->glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }


    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);


    bool bound = pointCloudProgram->bind();

    if (bound) {
        pointCloudProgram->setUniformValue(projMatrixLoc, proj);
        pointCloudProgram->setUniformValue(mvMatrixLoc, modelView);
        pointCloudProgram->setUniformValue(drawColoredPointsLoc, drawColoredPoints);
        f->glBindVertexArray(pointCloudVAO);
        // NOTE: maybe change pointsize
        f->glPointSize(2.5f);
        f->glDrawArrays(GL_POINTS, 0, (GLsizei)numPoints);
        pointCloudProgram->release();
    }

    if (drawNormals) {
        qInfo("Drawing Normals");

        f->glBindVertexArray(normalDebugVAO);
        f->glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        f->glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        f->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);
        f->glEnableVertexAttribArray(2);

        bound = normalDebugProgram->bind();

        if (bound) {
            qInfo("Normal Program Bound");
            normalDebugProgram->setUniformValue(normalDebugProgram->uniformLocation("projMatrix"), proj);
            normalDebugProgram->setUniformValue(normalDebugProgram->uniformLocation("mvMatrix"), modelView);

            f->glBindVertexArray(normalDebugVAO);
            f->glDrawArrays(GL_POINTS, 0, (GLsizei)numPoints);
            normalDebugProgram->release();
        }
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
        case Qt::Key_W:         cameraPosition += cameraDirection * moveSpeed; break;
        case Qt::Key_S:         cameraPosition -= cameraDirection * moveSpeed; break;
        case Qt::Key_A:         cameraPosition -= cameraRight * moveSpeed; break;
        case Qt::Key_D:         cameraPosition += cameraRight * moveSpeed; break;
        case Qt::Key_PageDown:  cameraPosition -= cameraUp * moveSpeed; break;
        case Qt::Key_PageUp:    cameraPosition += cameraUp * moveSpeed; break;

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
