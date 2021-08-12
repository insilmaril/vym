#ifndef SLIDEEDITOR_H
#define SLIDEEDITOR_H

#include <QItemSelection>
#include <QWidget>

class SlideModel;
class QTreeView;
class SlideControlWidget;
class VymModel;

class SlideEditor : public QWidget {
    Q_OBJECT

  public:
    SlideEditor(VymModel *);

  public slots:
    void previousSlide();
    void nextSlide();
    void addSlide();
    void editSlide();
    void deleteSlide();
    void moveSlideUp();
    void moveSlideDown();
    void updateSelection(QItemSelection, QItemSelection);

  private:
    VymModel *vymModel;
    SlideModel *slideModel;
    QTreeView *view;
    SlideControlWidget *slideControl;
};

#endif
