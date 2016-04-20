#ifndef SATELLITE_H
#define SATELLITE_H

#include <QKeyEvent>
#include <QWidget>

class Satellite:public QWidget
{
    Q_OBJECT

signals:
    void focusReleased();
    
protected:
    void keyPressEvent(QKeyEvent* );

};
#endif 
