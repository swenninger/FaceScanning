#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <QObject>
#include "nanoflann.hpp"
#include "Types.h"

struct PointCloudBuffer;
struct FrameBuffer;

namespace PointCloudHelpers {

extern int theSnapshotCount;

//
// Short Name for nanoflann-KDTree-Implementation
//
typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloudBuffer>,
        PointCloudBuffer,
        3> KDTree;

//
// Filter Pointcloud into destination PointCloudBuffer. If a point is more than sttdevMultiplier standard deviations
// away from its numNeighbors neighbors, then it is excluded in the filtered PointCloud.
//
void Filter(PointCloudBuffer* src, PointCloudBuffer* dst, size_t numNeighbors = 10, float stddevMultiplier = 1.0f);

//
// Compute Normals and store the result into the passed buffer.
//
void ComputeNormals(PointCloudBuffer* src);

//
// Save incoming frame to disk
//
QString SaveSnapshot(FrameBuffer* frame, QString snapshotPath);

//
// Load frame from disk
//
void LoadSnapshot(const std::string snapshotMetaFileName, PointCloudBuffer* buf);

//
// Creates a Thread and runs the normal computation asynchronously
//
// The listener object needs to define a SLOT named OnNormalsComputed to be notified
// when the thread completes.
//
void CreateAndStartNormalWorker(PointCloudBuffer* src, QObject* listener);

//
// Creates a Thread and runs the filtering asynchronously.
//
// The listener object needs to define a SLOT named OnPointcloudFiltered
//
void CreateAndStartFilterWorker(PointCloudBuffer* src, PointCloudBuffer* dst, QObject* listener,
                                size_t numNeighbors = 10, float stddevMultiplier = 1.0f);

//
// Creates a Thread and runs snapshot saving asynchronously.
//
// The listener object needs to define a SLOT named OnSnapshotSaved(QString)
//
void CreateAndStartSaveSnapshotWorker(FrameBuffer* src, QObject* listener);

//
// Generates random points on a hemisphere and stores the result into the passed buffer
//
void GenerateRandomHemiSphere(PointCloudBuffer* dst,int numPoints, Vec3f center = Vec3f(0.0f, 0.0f, 1.0f), float radius = 0.1f);


//
// Wrapper class for running normal computation in a worker thread
//
class ComputeNormalWorker : public QObject
{
    Q_OBJECT

public:
    ComputeNormalWorker(PointCloudBuffer* src) : pc(src) {}
    ~ComputeNormalWorker() {}

public slots:
    void ComputeNormals() { PointCloudHelpers::ComputeNormals(pc); emit finished(); }

signals:
    void finished();

private:
    PointCloudBuffer* pc;
};

//
// Wrapper Class for running PointCloudFiltering in a worker thread
//
class FilterPointcloudWorker : public QObject
{
    Q_OBJECT

public:
    FilterPointcloudWorker(PointCloudBuffer* src, PointCloudBuffer* dst, size_t numNeighbors, float stddevMultiplier)
        : src_(src), dst_(dst), numNeighbors_(numNeighbors), stddevMultiplier_(stddevMultiplier) {}
    ~FilterPointcloudWorker() {}

public slots:
    void FilterPointcloud() { PointCloudHelpers::Filter(src_, dst_, numNeighbors_, stddevMultiplier_); emit finished(); }

signals:
    void finished();

private:
    PointCloudBuffer* src_;
    PointCloudBuffer* dst_;

    size_t numNeighbors_;
    float  stddevMultiplier_;
};

//
// Wrapper Class for running SnapShot-Saving in a worker thread
//
class SaveSnapshotWorker : public QObject
{
    Q_OBJECT

public:
    SaveSnapshotWorker(FrameBuffer* src, QString snapshotPath) :
        src_(src), snapshotPath_(snapshotPath) {}
    ~SaveSnapshotWorker() {}

public slots:
    void SaveSnapshot() {
        QString metaFile = PointCloudHelpers::SaveSnapshot(src_, snapshotPath_);
        emit newMetaFile(metaFile);
        emit finished();
    }

signals:
    void finished();
    void newMetaFile(QString metaFileLocation);

private:
    FrameBuffer* src_;
    QString snapshotPath_;
};

}

#endif //POINTCLOUD_H
