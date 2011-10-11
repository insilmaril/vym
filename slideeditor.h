#ifndef SLIDEEDITOR_H 
#define SLIDEEDITOR_H

#include <QItemSelection>
#include <QWidget>

class SlideModel;
//class TreeItem;
class QTreeView;
//class QPushButton;
class SlideControlWidget;
class VymModel;

class SlideEditor: public QWidget
{
    Q_OBJECT

public:
    SlideEditor (VymModel *);
    QModelIndex getSelectedIndex();
    void addItem (const QString &s);

public slots:	
//    void popup();
    void previousSlide();
    void nextSlide();
    void takeSnapshot();
    void updateSelection(QItemSelection ,QItemSelection);

private:
    VymModel *model;
    SlideModel *slideModel;
    QTreeView *view;
    SlideControlWidget *slideControl;
};

#endif

