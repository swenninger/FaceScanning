#include "SnapshotGrid.h"

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>


SnapshotGrid::SnapshotGrid(QWidget *parent)
    : QWidget(parent)
{
    layoutRow = 0;
    layoutColumn = 0;
    maxLayoutColumn = 2;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    grid = new QGridLayout(this);

    addSelectableSnapshot();
    addSelectableSnapshot();

    this->setLayout(grid);
}

void SnapshotGrid::addSelectableSnapshot()
{
    grid->addWidget(new SelectableSnapshot(), layoutRow, layoutColumn++);

    if (layoutColumn > maxLayoutColumn) {
        layoutColumn = 0;
        layoutRow++;
    }
}



SelectableSnapshot::SelectableSnapshot()
    : QLabel()
{
    this->setMinimumSize(50,50);
    this->setMaximumSize(100,100);
    this->setText("Hallo");
    this->setStyleSheet("background-color: black;");
    this->selected = false;
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

            if (selected) {
                this->setStyleSheet("background-color: white;");
            } else {
                this->setStyleSheet("background-color: black;");
            }
        }

    } else {
        QLabel::mouseReleaseEvent(event);
    }
}

void SelectableSnapshot::mouseMoveEvent(QMouseEvent *event)
{
    QLabel::mouseMoveEvent(event);
}

