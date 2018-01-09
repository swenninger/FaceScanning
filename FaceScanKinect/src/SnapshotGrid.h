#ifndef SNAPSHOT_GRID_H
#define SNAPSHOT_GRID_H

#include <QWidget>
#include <QLabel>

class QGridLayout;

class SnapshotGrid : public QWidget {

    // TODO: Do we need qobject macro here?
    Q_OBJECT

public:

    SnapshotGrid(QWidget* parent);
    ~SnapshotGrid() { };

    void addSelectableSnapshot();

private:
    QGridLayout* grid;

    int layoutColumn;
    int layoutRow;

    int maxLayoutColumn;
};

class SelectableSnapshot : public QLabel {
    Q_OBJECT

public:
    SelectableSnapshot();
    ~SelectableSnapshot() { }

public slots:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;


private:
    bool selected;
};

#endif // SNAPSHOT_GRID_H
