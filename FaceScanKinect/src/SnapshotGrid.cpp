#include "SnapshotGrid.h"

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

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

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &SnapshotGrid::onContextMenuRequested);
}

void SnapshotGrid::addSelectableSnapshot(QString metaFileLocation)
{
    SnapshotMetaInformation metaInf;
    bool validMetaFile = LoadMetaFile(metaFileLocation.toStdString(), &metaInf);

    if (!validMetaFile) { return; }

    SelectableSnapshot* snapshot = new SelectableSnapshot(this, metaInf);
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
        if (snapshot->IsSelected()) {
            result.append(snapshot->MetaInfo());
        }
    }

    return result;
}

void SnapshotGrid::onContextMenuRequested(const QPoint pos)
{
    QMenu menu;

    QAction removeAll(tr("Remove all"), this);
    connect(&removeAll, &QAction::triggered, this, &SnapshotGrid::RemoveAll);

    QAction removeAllDeselected(tr("Remove all deselected"), this);
    connect(&removeAllDeselected, &QAction::triggered, this, &SnapshotGrid::RemoveAllDeselected);

    menu.addAction(&removeAll);
    menu.addAction(&removeAllDeselected);

    menu.exec(mapToGlobal(pos));
}

void SnapshotGrid::RemoveAllDeselected(bool)
{
    QVector<SelectableSnapshot*> toClear;

    for (auto snapshotWidget : snapshots) {
        if (!snapshotWidget->IsSelected()) {
            grid->removeWidget(snapshotWidget);
            toClear.append(snapshotWidget);
            delete snapshotWidget;
        }
    }

    for (auto clear : toClear) { snapshots.removeAll(clear); }
}

void SnapshotGrid::RemoveAll(bool)
{
    for (auto snapshotWidget : snapshots) {
        grid->removeWidget(snapshotWidget);
        delete snapshotWidget;
    }

    snapshots.clear();

}

void SnapshotGrid::Remove(SelectableSnapshot *snapshot)
{
    grid->removeWidget(snapshot);
    snapshots.removeAll(snapshot);
}

SelectableSnapshot::SelectableSnapshot(SnapshotGrid *parent, QString metaFileLocation)
    : QLabel(),
      parent_(parent)
{
    LoadMetaFile(metaFileLocation.toStdString(), &meta);

    Initialize();
}

SelectableSnapshot::SelectableSnapshot(SnapshotGrid* parent, SnapshotMetaInformation metaInfo)
    : QLabel(),
      parent_(parent),
      meta(metaInfo)
{
    Initialize();
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

void SelectableSnapshot::onContextMenuRequested(const QPoint pos)
{
    QMenu menu;

    QAction deleteAction("Remove Snapshot", this);
    connect(&deleteAction, &QAction::triggered, this, &SelectableSnapshot::DeleteFromParent);
    menu.addAction(&deleteAction);
    menu.exec(mapToGlobal(pos));
}

void SelectableSnapshot::DeleteFromParent()
{
    parent_->Remove(this);
    this->deleteLater();
}

void SelectableSnapshot::Initialize()
{
    this->setMinimumSize(100, 100);
    this->setMaximumSize(200, 200);

    //
    // TODO: Resize pixmap on widget resize??
    //
    this->setPixmap(QPixmap::fromImage(QImage(QString::fromStdString(meta.colorFile))).scaledToWidth(this->width()));

    this->selected = true;
    this->setStyleSheet(enabledStyle);

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(this, &QLabel::customContextMenuRequested, this, &SelectableSnapshot::onContextMenuRequested);
}

