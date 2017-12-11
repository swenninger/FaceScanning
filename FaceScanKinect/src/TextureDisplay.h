#ifndef TEXTURE_DISPLAY_H
#define TEXTURE_DISPLAY_H

#include <Kinect.h>

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <opencv2/opencv.hpp>

struct float3 {
    float x, y, z;
};

struct float2 {
    float u, v;
};

struct face {
    int v1, v2, v3;
    int vn1, vn2, vn3;
    int vt1, vt2, vt3;
};

class TextureDisplay : public QOpenGLWidget
{
    Q_OBJECT
public:
    TextureDisplay(ICoordinateMapper* coordinateMapper);
    virtual ~TextureDisplay();


protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;


private:
    void load_obj();
    void fill_cpu_buffers();
    void loadMappingFile();
    bool MapCameraToColorSpace(float3 pos, float2& out);
    bool map_camera_to_color_space(int index, float2* out);
    float3 UVToNormalizedDeviceCoordinate(float2 uv);

    ICoordinateMapper* coordinateMapper_;

    float2 getTexCoord(int indexStartingAtOne);
    float3 getVertex(int indexStartingAtOne);

    std::vector<float3> vertices;
    std::vector<float3> normals;
    std::vector<float2> textureCoordinates;
    std::vector<face> faces;

    float2* cameraToColorMapping;
    float3* triangles;
    float2* coordinates;

    uint32_t* pixels;

    GLuint vao;
    GLuint vbo;
    GLuint tbo;
    GLuint textureID;

    int colorImageLocation;
    int viewPortSizeLocation;
    QOpenGLShaderProgram* program;

};

#endif // TEXTURE_DISPLAY_H
