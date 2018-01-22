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

    QVector<SnapshotMetaInformation*> selectedSnapshots();

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
    explicit SelectableSnapshot(QString metaFileLocation);
    ~SelectableSnapshot() { }

    bool isSelected() { return selected; }
    SnapshotMetaInformation metaInfo() { return meta; }

public slots:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;


private:
    bool selected;
    SnapshotMetaInformation meta;
};

#endif // SNAPSHOT_GRID_H
