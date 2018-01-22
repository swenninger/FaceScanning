#ifndef SNAPSHOT_GRID_H
#define SNAPSHOT_GRID_H

#include <QWidget>
#include <QLabel>
#include <QVector>

#include "util.h"

class QGridLayout;
class SelectableSnapshot;

//
// Grid of saved snapshots
//
class SnapshotGrid : public QWidget {

    // TODO: Do we need qobject macro here?
    Q_OBJECT

public:

    SnapshotGrid(QWidget* parent);
    ~SnapshotGrid() { }

    void addSelectableSnapshot(QString metaFileLocation);
    void Remove(SelectableSnapshot* snapshot);

    QVector<SnapshotMetaInformation*> selectedSnapshots();

public slots:
    virtual void onContextMenuRequested(const QPoint pos);

    void RemoveAllDeselected(bool);
    void RemoveAll(bool);

private:
    QGridLayout* grid;
    QVector<SelectableSnapshot*> snapshots;


    int layoutColumn;
    int layoutRow;

    int maxLayoutColumn;
};

//
// Individual snapshots than can be toggled on and off
//
class SelectableSnapshot : public QLabel {
    Q_OBJECT

public:
    SelectableSnapshot(SnapshotGrid* parent, QString metaFileLocation);
    SelectableSnapshot(SnapshotGrid* parent, SnapshotMetaInformation metaInfo);
    ~SelectableSnapshot() { }

    bool IsSelected() { return selected; }
    SnapshotMetaInformation* MetaInfo() { return &meta; }

public slots:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

    virtual void onContextMenuRequested(const QPoint pos);

private:
    SelectableSnapshot();
    void DeleteFromParent();
    void Initialize();

    SnapshotGrid* parent_;
    SnapshotMetaInformation meta;

    bool selected;
};

#endif // SNAPSHOT_GRID_H
