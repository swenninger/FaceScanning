#include "PointCloud.h"

#include <iomanip>

#include <QtMath>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <QDir>

#include <Core>
#include <Eigenvalues>

#include "util.h"
#include "MemoryPool.h"
#include "ScanSession.h"

int PointCloudHelpers::theSnapshotCount = 0;

void PointCloudHelpers::CreateAndStartNormalWorker(PointCloudBuffer* src, QObject* listener) {
    QThread* thread = new QThread;

    ComputeNormalWorker* worker = new ComputeNormalWorker(src);
    worker->moveToThread(thread);

    // Setup Signal connection

    // connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    QObject::connect(thread, SIGNAL(started()), worker, SLOT(ComputeNormals()));
    QObject::connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    QObject::connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    QObject::connect(worker, SIGNAL(finished()), listener, SLOT(OnNormalsComputed()));
    QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    // Start Work
    thread->start();
}

void PointCloudHelpers::CreateAndStartFilterWorker(PointCloudBuffer *src, PointCloudBuffer *dst, QObject *listener, size_t numNeighbors, float stddevMultiplier)
{

    QThread* thread = new QThread();
    FilterPointcloudWorker* worker = new FilterPointcloudWorker(src, dst, numNeighbors, stddevMultiplier);
    worker->moveToThread(thread);

    // connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    QObject::connect(thread, SIGNAL(started()), worker, SLOT(FilterPointcloud()));
    QObject::connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    QObject::connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    QObject::connect(worker, SIGNAL(finished()), listener, SLOT(OnPointcloudFiltered()));
    QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}

void PointCloudHelpers::CreateAndStartSaveSnapshotWorker(FrameBuffer *src, QObject* listener)
{
    QString snapshotPath = theScanSession.getCurrentScanSession();
    bool couldCreateSnapshotDirectory = QDir().mkpath(snapshotPath);
    if (!couldCreateSnapshotDirectory) {
        qCritical() << "Could not create snapshot directory at " << snapshotPath;
        return;
    }

    QThread* thread = new QThread();
    SaveSnapshotWorker* worker = new SaveSnapshotWorker(src, snapshotPath);
    worker->moveToThread(thread);

    // connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    QObject::connect(thread, SIGNAL(started()), worker, SLOT(SaveSnapshot()));
    QObject::connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    QObject::connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    QObject::connect(worker, SIGNAL(newMetaFile(QString)), listener, SLOT(OnSnapshotSaved(QString)));
    QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}

void PointCloudHelpers::Filter(PointCloudBuffer *src, PointCloudBuffer *dst, size_t numNeighbors, float stddevMultiplier)
{
    QElapsedTimer timer;
    timer.start();
    // Temporary memory, deleted at the end of the function
    float* distances = new float[src->numPoints];

    Vec3f* points = src->points;
    RGB3f* colors = src->colors;

    Vec3f* dst_points = dst->points;
    RGB3f* dst_colors = dst->colors;

    // TODO:
    // HACK to not filter out landmarks!!

    size_t numPointsInFilteredPointcloud = 0;
    for (int i = 0; i < src->numLandmarks; ++i) {
        size_t pointIndex = src->landmarkIndices[i];
        dst_points[numPointsInFilteredPointcloud] = points[pointIndex];
        dst_colors[numPointsInFilteredPointcloud] = colors[pointIndex];

        dst->landmarkIndices[i] = i;

        numPointsInFilteredPointcloud++;
    }
    dst->numLandmarks = src->numLandmarks;

    // TODO:
    // MAKE THIS CLEARER



    PointCloudHelpers::KDTree tree(3, *src, nanoflann::KDTreeSingleIndexAdaptorParams());
    tree.buildIndex();

    // Compute mean distance to k nearest neighbors for all points

    size_t numResults = numNeighbors;
    std::vector<size_t> indices(numResults);
    std::vector<float>  squaredDistances(numResults);

    size_t numPoints = src->numPoints;
    for (size_t pointIndex = 0; pointIndex < numPoints; pointIndex++) {

        numResults = numNeighbors;

        float distance = 0.0f;

        float* queryPoint = &(points[pointIndex].X);

        tree.knnSearch(queryPoint, numResults, &indices[0], &squaredDistances[0]);

        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {

            // TODO: check if squared distances are also ok!

            distance += sqrt(squaredDistances[neighbor]);
        }

        distance /= numResults;

        distances[pointIndex] = distance;
    }

    // Compute mean and stddev of all average distances

    float mean = 0.0f, stddev = 0.0f;
    for (size_t pointIndex = 0; pointIndex < numPoints; pointIndex++) {
        float n = (float)pointIndex + 1.0f;

        float dist = distances[pointIndex];

        float previousMean = mean;
        mean   += (dist - mean) / n;
        stddev += (dist - mean) * (dist - previousMean);
    }
    stddev = sqrt(stddev / (float)numPoints);


    // TODO:

    // HACK to filter out landmarks, because they have been shuffled in memory!!

    for (int i = 0; i < src->numLandmarks; ++i) {
        distances[src->landmarkIndices[i]] = mean + stddevMultiplier * stddev + 10.0f;
    }

    // MAKE THIS CLEARER

    // TODO:


    // "Remove" points that are further than stddev_multitplier stddevs away from the mean
    for (size_t pointIndex = 0; pointIndex < numPoints; pointIndex++) {
        if (distances[pointIndex] < mean + stddevMultiplier * stddev) {
            dst_points[numPointsInFilteredPointcloud] = points[pointIndex];
            dst_colors[numPointsInFilteredPointcloud] = colors[pointIndex];

            numPointsInFilteredPointcloud++;
        }
    }

    dst->numPoints = numPointsInFilteredPointcloud;

    delete [] distances;
    qInfo() << "Pointcloud filtered in " << timer.elapsed() << "ms";
}

void PointCloudHelpers::ComputeNormals(PointCloudBuffer* src)
{
    KDTree tree(3, *src, nanoflann::KDTreeSingleIndexAdaptorParams());
    tree.buildIndex();

    Vec3f* normals = src->normals;
    Vec3f* points  = src->points;

    // TODO: remove debug output
    // TODO: How many neighbors?
    size_t numResults = 15;
    std::vector<size_t> indices(numResults);
    std::vector<float>  squaredDistances(numResults);

    Eigen::Vector3f zero(0.0f, 0.0f, 0.0f);

    // For each point ...
    for (size_t pointIndex = 0; pointIndex < src->numPoints; ++pointIndex) {
        numResults = 15;

        float* queryPoint = &(points[pointIndex].X);

        // ... get nearest neighbors ...
        numResults = tree.knnSearch(queryPoint, numResults, &indices[0], &squaredDistances[0]);

        // ... calculate Covariance Matrix ...
        Eigen::Vector3f centroid(0.0, 0.0, 0.0);
        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {
            centroid += Eigen::Map<Eigen::Vector3f>(&points[indices[neighbor]].X);
        }
        centroid /= numResults;

        Eigen::Matrix3f covarianceMatrix = Eigen::Matrix3f::Zero();
        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {
            Eigen::Vector3f d = Eigen::Map<Eigen::Vector3f>(&points[indices[neighbor]].X) - centroid;
            covarianceMatrix += d * d.transpose();
        }
        covarianceMatrix /= numResults;

        // ... Compute eigenvalues and eigenvectors of the covariance matrix. This gives us
        //     3 vectors that span a plane through the points ...

        // Eigenvalues are not sorted
        Eigen::EigenSolver<Eigen::Matrix3f> solver(covarianceMatrix, true);

        // ... smallest eigenvalue corresponds to the normal vector of the plane ...
        Eigen::Vector3f eigenValues = solver.eigenvalues().real();
        int min;
        eigenValues.minCoeff(&min);

        // TODO: simplify
        auto eigenvectorsComplex = solver.eigenvectors();
        Eigen::MatrixXf eigenvectorsReal = eigenvectorsComplex.real();
        Eigen::Vector3f normal = eigenvectorsReal.col(min);

        // ... 3D Sensor can only retrieve elements, that point towards it ...
        Eigen::Vector3f pointToView = zero - Eigen::Map<Eigen::Vector3f>(queryPoint);
        float dotProduct = normal.transpose() * pointToView;

        // ... flip normal if it does not point towards sensor ...
        if (dotProduct < 0) { normal = -normal; }

        normals[pointIndex] = Vec3f(normal.data()); // [0], normal[1], normal[2]);
    }

    qInfo("Normals Computed");
}

/**
 * @brief GenerateRandomHemiSphere Randomly generates points on a hemisphere (leaving out half the z-values) with the same method as GenerateRandomSphere()
 * @param numPoints
 * @param center
 * @param radius
 * @return
 */
void PointCloudHelpers::GenerateRandomHemiSphere(PointCloudBuffer* dst,int numPoints, Vec3f center, float radius) {

    dst->numPoints = numPoints;

    Vec3f* points = dst->points;
    RGB3f* colors = dst->colors;

    for (int i = 0; i < numPoints; ++i) {

        float theta = 2 * M_PI * RandomFloat01();
        float phi   = acos(1 - 2 * RandomFloat01());

        double x = sin(phi) * cos(theta);
        double y = sin(phi) * sin(theta);
        double z = cos(phi);

        // discard vertices if they lie on the wrong hemisphere
        if (z > -0.09) {
            if (i > 0) {
                --i;
                continue;
            }
        }

        points[i] = Vec3f(center.X + radius * x,
                          center.Y + radius * y,
                          center.Z + radius * z);
        colors[i] = RGB3f(0.4f, 0.8f, 0.2f);
    }
}


//
// Hacky function for not filtering out landmarks
//
void AddLandmarks(PointCloudBuffer* src, PointCloudBuffer *dst) {
    for (int i = 0; i < src->numLandmarks; ++i) {
        size_t landmark = src->landmarkIndices[i];
        dst->colors[dst->numPoints] = src->colors[landmark];
        dst->points[dst->numPoints] = src->points[landmark];
        dst->normals[dst->numPoints] = src->normals[landmark];

        dst->numPoints++;
    }
}

static void CopyLandmarkInformation(PointCloudBuffer* src, PointCloudBuffer *dst) {
    memcpy(dst->landmarkIndices, src->landmarkIndices, LANDMARK_BUFFER_SIZE);
    dst->numLandmarks = src->numLandmarks;
}

QString PointCloudHelpers::SaveSnapshot(FrameBuffer *frame, QString snapshotPath)
{
    std::stringstream stringBuilder;
    stringBuilder << snapshotPath.toStdString() << std::setfill('0') << std::setw(3) << theSnapshotCount++ << "_";
    std::string snapshotDirectoryWithCountPrefix = stringBuilder.str();
    PointCloudBuffer tmp;

    std::string metaFile = snapshotDirectoryWithCountPrefix + "snapshot.meta";

    SnapshotMetaInformation metaInfo;
    metaInfo.pointCloudFile = snapshotDirectoryWithCountPrefix + "pointcloud.pc";
    metaInfo.colorFile      = snapshotDirectoryWithCountPrefix + "color.bmp";
    metaInfo.depthFile      = snapshotDirectoryWithCountPrefix + "depth.bmp";
    metaInfo.landmarkFile   = snapshotDirectoryWithCountPrefix + "landmark_indices.txt";
    metaInfo.meshFile       = snapshotDirectoryWithCountPrefix + "mesh.obj";

    // Preprocessing
    Filter(frame->pointCloudBuffer, &tmp);
    ComputeNormals(&tmp);

    // Write files
    WriteMetaFile(metaFile, metaInfo);
    SavePointCloud(metaInfo.pointCloudFile, tmp.points, tmp.colors, tmp.normals, tmp.numPoints);
    SaveColorImage(metaInfo.colorFile, frame->colorBuffer);
    SaveDepthImage(metaInfo.depthFile, frame->depthBuffer8);
    SaveLandmarks(metaInfo.landmarkFile, tmp.landmarkIndices, tmp.numLandmarks);

    // Write back the filtered pointcloud to the inspection frame
    CopyPointCloudBuffer(&tmp, frame->pointCloudBuffer);

    return QString::fromStdString(metaFile);
}

// TODO: change to framebuffer and load images
void PointCloudHelpers::LoadSnapshot(const std::string snapshotMetaFileName, PointCloudBuffer* buf) {

    SnapshotMetaInformation metaInfo;
    LoadMetaFile(snapshotMetaFileName, &metaInfo);

    LoadLandmarks(metaInfo.landmarkFile, buf->landmarkIndices, &buf->numLandmarks);

    std::ifstream pointcloudFile(metaInfo.pointCloudFile);

    if (!pointcloudFile.is_open()) {
        qCritical() << "Could not open pointcloud file for reading";
        return;
    }

    int count = 0;

#if 0
    std::string line;

    while (std::getline(pointcloudFile, line)) {
        ++count;
    }
    pointcloudFile.close();
    pointcloudFile.open(pointcloudFilename);
#endif

    Vec3f* pointData =  buf->points;
    RGB3f* colorData =  buf->colors;
    Vec3f* normalData = buf->normals;

    float x, y, z;
    int   r, g, b;
    float nx, ny, nz;

    while (pointcloudFile >> x  >> y  >> z
                          >> r  >> g  >> b
                          >> nx >> ny >> nz) {
        Vec3f* p = pointData  + count;
        RGB3f* c = colorData  + count;
        Vec3f* n = normalData + count;

        p->X = x;
        p->Y = y;
        p->Z = z;

        c->R = (float)(r / 255.0f);
        c->G = (float)(g / 255.0f);
        c->B = (float)(b / 255.0f);

        n->X = nx;
        n->Y = ny;
        n->Z = nz;

        count++;
    }

    // TODO: remove this debug visualization
    for (int i = 0; i < buf->numLandmarks; ++i) {  colorData[buf->landmarkIndices[i]] = {0.1f, 0.1f, 1.0f}; }


    buf->numPoints = count;
    pointcloudFile.close();
}
