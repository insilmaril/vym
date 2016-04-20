#include "satellite.h"

void Satellite::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
        emit( focusReleased());
    else
        QWidget::keyPressEvent(e);
}

