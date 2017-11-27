#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <QObject>
#include "MemoryPool.h"
#include "nanoflann.hpp"

namespace PointCloudHelpers {

// TODO: CLEANUP

//
// Short Name for nanoflann-KDTree-Implementation
//
typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloudBuffer>,
        PointCloudBuffer,
        3> KDTree;

void Filter(PointCloudBuffer* src, PointCloudBuffer* dst, size_t numNeighbors = 10, float stddevMultiplier = 1.0f);
void ComputeNormals(PointCloudBuffer* src);
void SaveSnapshot(FrameBuffer* frame);
void LoadSnapshot(const std::string pointcloudFilename, PointCloudBuffer* buf);


void CreateAndStartNormalWorker(PointCloudBuffer* src, QObject* listener);
void CreateAndStartFilterWorker(PointCloudBuffer* src, PointCloudBuffer* dst, QObject* listener,
                                size_t numNeighbors = 10, float stddevMultiplier = 1.0f);

void GenerateRandomHemiSphere(PointCloudBuffer* dst,int numPoints, Vec3f center = Vec3f(0.0f, 0.0f, 1.0f), float radius = 0.1f);


/**
 * @brief Wrapper class for running normal computation in a worker thread
 */
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

/**
 * @brief Wrapper Class for running PointCloudFiltering in a worker thread
 */
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

}

#endif //POINTCLOUD_H
