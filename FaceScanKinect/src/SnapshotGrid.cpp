#include "SnapshotGrid.h"

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>

#include "util.h"


//
// TODO: Better visualization of enabled/disabled state
//
QString enabledStyle  = "border: 4px solid #bbbbbb; background: #bbbbbb";
QString disabledStyle = "border: 4px solid #111111; background: #111111";

SnapshotGrid::SnapshotGrid(QWidget *parent)
    : QWidget(parent)
{
    layoutRow = 0;
    layoutColumn = 0;
    maxLayoutColumn = 2;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    grid = new QGridLayout(this);
    this->setLayout(grid);
}

void SnapshotGrid::addSelectableSnapshot(QString metaFileLocation)
{
    SelectableSnapshot* snapshot = new SelectableSnapshot(metaFileLocation);
    snapshots.append(snapshot);

    grid->addWidget(snapshot, layoutRow, layoutColumn++);

    if (layoutColumn > maxLayoutColumn) {
        layoutColumn = 0;
        layoutRow++;
    }
}

QVector<SnapshotMetaInformation*> SnapshotGrid::selectedSnapshots()
{
    QVector<SnapshotMetaInformation*> result;

    for (auto snapshot : snapshots) {
        if (snapshot->isSelected()) {
            result.append(&snapshot->metaInfo());
        }
    }

    return result;
}



SelectableSnapshot::SelectableSnapshot(QString metaFileLocation)
    : QLabel()
{
    LoadMetaFile(metaFileLocation.toStdString(), &meta);

    this->setMinimumSize(100, 100);
    this->setMaximumSize(200, 200);

    //
    // TODO: Resize pixmap on widget resize??
    //
    this->setPixmap(QPixmap::fromImage(QImage(QString::fromStdString(meta.colorFile))).scaledToWidth(this->width()));

    this->selected = true;
    this->setStyleSheet(enabledStyle);
}

void SelectableSnapshot::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
}

void SelectableSnapshot::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {

        if (rect().contains(event->localPos().toPoint())) {
            selected = !selected;
            this->setStyleSheet(selected ? enabledStyle : disabledStyle);
        }

    } else {
        QLabel::mouseReleaseEvent(event);
    }
}

void SelectableSnapshot::mouseMoveEvent(QMouseEvent *event)
{
    QLabel::mouseMoveEvent(event);
}

