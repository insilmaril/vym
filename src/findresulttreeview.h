#ifndef FindResultTreeView_H
#define FindResultTreeView_H

#include <QTreeView>

class VymModel;

/*! \brief TreeView widget in vym to display and edit a map, based on
 * QTreeView */

class FindResultTreeView : public QTreeView {
    Q_OBJECT

  public:
    FindResultTreeView();
    ~FindResultTreeView();
    void init();
    QModelIndex getSelectedIndex();

  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void closeEvent(QCloseEvent *event);

  signals:
    void searchFinished();

  private slots:
    void startEdit();

  private:
    VymModel *model;
};

#endif
