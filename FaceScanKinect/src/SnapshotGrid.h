#ifndef SNAPSHOT_GRID_H
#define SNAPSHOT_GRID_H

#include <QWidget>
#include <QLabel>

class QGridLayout;

//
// Grid of saved snapshots
//
class SnapshotGrid : public QWidget {

    // TODO: Do we need qobject macro here?
    Q_OBJECT

public:

    SnapshotGrid(QWidget* parent);
    ~SnapshotGrid() { };

    void addSelectableSnapshot(QString metaFileLocation);

private:
    QGridLayout* grid;

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

public slots:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;


private:
    bool selected;
};

#endif // SNAPSHOT_GRID_H
