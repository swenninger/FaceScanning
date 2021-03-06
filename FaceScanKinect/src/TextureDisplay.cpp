#include "TextureDisplay.h"

#include <fstream>

#include <QDebug>
#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions>
#include <QElapsedTimer>

// TODO: Factor out load functionality and only deal with
//       OpenGL display here!!

// TODO: pass directory
const std::string data_directory = "..\\..\\data\\";

// TODO: which texture size is appropriate
const int TEXTURE_SIZE = 2048 * 2;

TextureDisplay::TextureDisplay(ICoordinateMapper* coordinateMapper, std::string metaFileLocation) :
    coordinateMapper_(coordinateMapper)
{
    LoadMetaFile(metaFileLocation, &meta);

    QElapsedTimer timer;
    timer.start();
    setMinimumSize(TEXTURE_SIZE, TEXTURE_SIZE);

    load_obj(meta.meshFile);

    cameraToColorMapping = new float2[vertices.size()];

    // loadMappingFile();
    fill_cpu_buffers();

    pixels = new uint32_t[TEXTURE_SIZE * TEXTURE_SIZE];

    qInfo() << "Loading buffers from file took " << timer.elapsed() << "ms";
}

TextureDisplay::~TextureDisplay()
{
    delete [] cameraToColorMapping;
    delete [] triangles;
    delete [] coordinates;
    delete [] normals;
    delete [] pixels;
    delete program;
}

bool TextureDisplay::MapCameraToColorSpace(float3 pos, float2& out) {
    CameraSpacePoint p = {pos.x, pos.y, pos.z};
    ColorSpacePoint c;

    HRESULT hr = coordinateMapper_->MapCameraPointToColorSpace(p, &c);

    out.u = c.X / 1920.f;
    out.v = c.Y / 1080.f;

    bool result = (SUCCEEDED(hr)    &&
                   !std::isinf(c.X) &&
                   !std::isinf(c.Y) &&
                   c.X > 0          &&
                   c.Y > 0);

    return result;
}

//
// UV coordinates are in the range [0.0, 1.0]. NDCs range from [-1.0, 1.0].
// This function simply maps one into to the other.
//
// The z-value of the resulting vertex is set to 0.0, we do not deal with any
// perspective or orthogonal projection here.
//
float3 inline TextureDisplay::UVToNormalizedDeviceCoordinate(float2 uv) {
    float3 result = {
       2 * (uv.u - 0.5f),
       2 * (uv.v - 0.5f),
       0.0f
    };

    return result;
}

//
// Maps from 3D camera space to 2D color coordinates.
// This function uses a pre-saved mapping table.
//
// TODO: Do we want this to stay this way and save out the mapping when creating the snapshot?
//       Or do we want to use the KINECT API here instead? (probably slower)
//
bool inline TextureDisplay::map_camera_to_color_space(int indexStartingAtOne, float2* out) {
    float2 result = cameraToColorMapping[indexStartingAtOne - 1];

    // Camera to color Mapping table gives us coordinates in the original image size (1920x1080)
    // We normalize these to get UV values in the range [0.0, 1.0]
    out->u = result.u / 1920.f;
    out->v = result.v / 1080.f;

    return !(result.u == 0 && result.v == 0);
}

inline float DotProduct(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float length(float3 a) {
    return sqrtf(DotProduct(a, a));
}

inline float3 normalizedOrZero(float3 a) {
    float len = length(a);

    if (fabs(len) < 1e-6) {
        return {0.f, 0.f, 0.f};
    }

    return { a.x / len, a.y / len, a.z / len };
}


//
// Create the buffers that will be uploaded to OpenGL
//
void TextureDisplay::fill_cpu_buffers() {
    size_t numFaces = faces.size();

    double minCos, maxCos;
    minCos = 10000.f;
    maxCos = -10000.f;

    triangles   = new float3[numFaces * 3];
    coordinates = new float2[numFaces * 3];
    normals     = new float3[numFaces * 3];
    mesh_vertices = new float3[numFaces * 3];

    qInfo() << "Filling cpu buffers: " << numFaces << " faces";

    int numBadMappings = 0;

    size_t count = 0;

    cv::Mat im = cv::imread(meta.colorFile);


    for (size_t i = 0; i < numFaces; ++i) {
        auto& face = faces[i];

        float3 v1 = getVertex(face.v1);
        float3 v2 = getVertex(face.v2);
        float3 v3 = getVertex(face.v3);

        float2 t1 = getTexCoord(face.vt1);
        float2 t2 = getTexCoord(face.vt2);
        float2 t3 = getTexCoord(face.vt3);

        bool success = true;
        float2 uv1, uv2, uv3;

        // This is the version that uses KINECT API

        success = success && MapCameraToColorSpace(v1, uv1);
        success = success && MapCameraToColorSpace(v2, uv2);
        success = success && MapCameraToColorSpace(v3, uv3);

        // Uses a pre-saved table for the mapping

        // success = success && map_camera_to_color_space(face.v1, &uv1);
        // success = success && map_camera_to_color_space(face.v2, &uv2);
        // success = success && map_camera_to_color_space(face.v3, &uv3);

        float3 n1  = getNormal(face.v1);
        float3 n2  = getNormal(face.v2);
        float3 n3  = getNormal(face.v3);

        //
        // IDEA:
        //    Do not set Texel if the normals is facing backwards to any of the views!
        //
        //    We should be able to get the views in 3D space relative to the mesh, by
        //    applying the Rotation and Translation we get from the ICP algorithm to
        //    the camera point at the origin
        //

#if 0
        // View is at {0,0,0}, so pointToView is -v
        float3 vertexToView1 = {-v1.x, -v1.y, -v1.z};
        float3 vertexToView2 = {-v2.x, -v2.y, -v2.z};
        float3 vertexToView3 = {-v3.x, -v3.y, -v3.z};

        float cosViewToNormal1 = DotProduct(n1, normalizedOrZero(vertexToView1));
        float cosViewToNormal2 = DotProduct(n2, normalizedOrZero(vertexToView2));
        float cosViewToNormal3 = DotProduct(n3, normalizedOrZero(vertexToView3));

        success = success && (cosViewToNormal1 >= 0.f);
        success = success && (cosViewToNormal2 >= 0.f);
        success = success && (cosViewToNormal3 >= 0.f);
#endif
        if (success) {
            triangles[count * 3 + 0] = UVToNormalizedDeviceCoordinate(t1);
            triangles[count * 3 + 1] = UVToNormalizedDeviceCoordinate(t2);
            triangles[count * 3 + 2] = UVToNormalizedDeviceCoordinate(t3);

            normals[count * 3 + 0] = n1;
            normals[count * 3 + 1] = n2;
            normals[count * 3 + 2] = n3;

            mesh_vertices[count * 3 + 0] = v1;
            mesh_vertices[count * 3 + 1] = v2;
            mesh_vertices[count * 3 + 2] = v3;

            coordinates[count * 3 + 0] = uv1;
            coordinates[count * 3 + 1] = uv2;
            coordinates[count * 3 + 2] = uv3;

            im.at<cv::Vec3b>(uv1.v * 1080, uv1.u * 1920) = cv::Vec3b(100,200,30);
            im.at<cv::Vec3b>(uv2.v * 1080, uv2.u * 1920) = cv::Vec3b(100,200,30);
            im.at<cv::Vec3b>(uv3.v * 1080, uv3.u * 1920) = cv::Vec3b(100,200,30);

            ++count;
        } else {
            numBadMappings++;
        }
    }

    cv::imwrite("mapped.png", im);

    qInfo() << numBadMappings << " bad mappings from coordinate mapper";
    this->numPoints = count;
}

// TODO: factor out (and pass memory?)

// Reads the mesh into the std::vector class members
void TextureDisplay::load_obj(std::string objFile) {
    std::ifstream file(objFile);

    size_t numVertices = 0;
    size_t numFaces = 0;
    size_t numNormals = 0;
    size_t numTexcoords = 0;

    float x, y, z;
    face f;

    std::string line;

    while(std::getline(file, line)) {

        size_t size = line.size();

        if (size > 3) {
            if (line[0] == 'v' && line[1] == 'n')
            {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                mesh_normals.push_back({x, y, z});
                numNormals++;
            }
            else if (line[0] == 'v' && line[1] == 't')
            {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                textureCoordinates.push_back({x, y});
                numTexcoords++;
            }
            else if (line[0] == 'v' && line[1] == ' ')
            {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                vertices.push_back({x, y, z});
                numVertices++;
            }
            else if (line[0] == 'f' && line[1] == ' ')
            {
                sscanf(line.c_str(), "%*s %d/%d/%d %d/%d/%d %d/%d/%d",
                       &f.v1, &f.vt1, &f.vn1,
                       &f.v2, &f.vt2, &f.vn2,
                       &f.v3, &f.vt3, &f.vn3
                       );
                faces.push_back(f);
                numFaces++;
            }
        }
    }

    file.close();
}

void TextureDisplay::loadMappingFile() {
    // TODO: pass filename (and storage ?)
    std::ifstream mappingFile(data_directory + "mesh\\mapping.txt");

    float x,y;
    int count = 0;
    while(mappingFile >> x >> y) {
        cameraToColorMapping[count++] = {x,y};
    }

    mappingFile.close();

    qInfo() << "Mapping.txt loaded " << count << " mappings";
}

//
// Helper functions for mapping the 1-based indices from the obj file
// to 0-based arrays in memory
//
inline float2 TextureDisplay::getTexCoord(int indexStartingAtOne) {
    return textureCoordinates[indexStartingAtOne - 1];
}

inline float3 TextureDisplay::getVertex(int indexStartingAtOne) {
    return vertices[indexStartingAtOne - 1];
}

inline float3 TextureDisplay::getNormal(int indexStartingAtOne) {
    return mesh_normals[indexStartingAtOne - 1];
}

//
// Pass through vertex shader
//
const char* vertexShaderFile = "..\\src\\shaders\\textureCreation.vs";

//
// The fragment shader looks up interpolated texture coordinates
// in the rgb image from the kinect.
//
const char* fragmentShaderFile = "..\\src\\shaders\\textureCreation.fs";

void TextureDisplay::initializeGL()
{
    QElapsedTimer timer;
    timer.start();

    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glClearColor(0.2f, 0.7f, 0.2f, 1.0f);

    f->glGenVertexArrays(1, &vao);
    f->glBindVertexArray(vao);

    f->glGenBuffers(1, &vbo);
    f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    f->glBufferData(GL_ARRAY_BUFFER, this->numPoints * 3 * sizeof(float3), triangles, GL_STATIC_DRAW);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    f->glEnableVertexAttribArray(0);

    f->glGenBuffers(1, &tbo);
    f->glBindBuffer(GL_ARRAY_BUFFER, tbo);
    f->glBufferData(GL_ARRAY_BUFFER, this->numPoints * 3 * sizeof(float2), coordinates, GL_STATIC_DRAW);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    f->glEnableVertexAttribArray(1);

    GLuint vertices;
    f->glGenBuffers(1, &vertices);
    f->glBindBuffer(GL_ARRAY_BUFFER, vertices);
    f->glBufferData(GL_ARRAY_BUFFER, this->numPoints * 3 * sizeof(float3), mesh_vertices, GL_STATIC_DRAW);
    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    f->glEnableVertexAttribArray(2);

    f->glGenBuffers(1, &nbo);
    f->glBindBuffer(GL_ARRAY_BUFFER, nbo);
    f->glBufferData(GL_ARRAY_BUFFER, this->numPoints * 3 * sizeof(float3), normals, GL_STATIC_DRAW);
    f->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
    f->glEnableVertexAttribArray(3);


    program = new QOpenGLShaderProgram();
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderFile);
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderFile);
    program->bindAttributeLocation("vert_pos", 0);
    program->bindAttributeLocation("tex_coord", 1);
    program->link();
    program->bind();
    colorImageLocation = program->uniformLocation("colorImage");
    viewPortSizeLocation = program->uniformLocation("viewport_size");
    // activate texture unit
    f->glActiveTexture(GL_TEXTURE0);

    // create texture object
    f->glGenTextures(1, &textureID);
    f->glBindTexture(GL_TEXTURE_2D, textureID);

    // set texture parameters
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    cv::Mat cv_tex = cv::imread(meta.colorFile);
    cv::cvtColor(cv_tex, cv_tex, CV_BGR2BGRA);

    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, cv_tex.cols, cv_tex.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, cv_tex.ptr());
    f->glGenerateMipmap(GL_TEXTURE_2D);

    qInfo() << "Initializing OpenGL took " << timer.elapsed() << "ms";
}

void TextureDisplay::paintGL()
{
    QElapsedTimer timer;
    timer.start();

    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glClear(GL_COLOR_BUFFER_BIT);
    f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bool bound = program->bind();
    if (bound) {
        f->glBindVertexArray(vao);
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, textureID);
        program->setUniformValue(colorImageLocation, 0);
        program->setUniformValue(viewPortSizeLocation, TEXTURE_SIZE);
        //f->glDrawElements(mode, n_indices_, GL_UNSIGNED_INT, NULL);
        f->glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(faces.size() * 3));
        program->release();
    }

    // Read Pixels stores the screen buffer into the passed memory
    f->glReadPixels(0, 0, TEXTURE_SIZE, TEXTURE_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    cv::Mat texture = cv::Mat(TEXTURE_SIZE, TEXTURE_SIZE, CV_8UC4, pixels);

    // OpenGL is bottom up, OpenCV is topdown.
    cv::flip(texture, texture, 0);

    // Write result texture to disk.
    cv::imwrite("test_texture.png", texture);

    qInfo() << "PaintGL + imwrite took " << timer.elapsed() << "ms";
}

void TextureDisplay::resizeGL(int w, int h)
{
    QOpenGLFunctions_4_0_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_0_Core>();
    f->glViewport(0, 0, w, h);
}
