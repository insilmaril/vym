#ifndef FindControlsWidget_H
#define FindControlsWidget_H

#include <QWidget>

class QAction;
class QGroupBox;
class QComboBox;
class QPushButton;

class FindControlsWidget : public QWidget {
    Q_OBJECT

  public:
    enum Status { Undefined, Success, Failed };

    FindControlsWidget(QWidget *parent = nullptr);
    QString getFindText();
    bool getSearchNotes();

  public slots:
    void nextPressed();
    void findTextChanged(const QString &);
    void indexChanged(int);
    void setFocus();
    void setStatus(Status st);

  private:
    Status status;

  signals:
    void nextButtonPressed(QString, bool);

  private:
    QComboBox *findcombo;
    QGroupBox *findbox;
    QPushButton *nextButton;
    QPushButton *filterNotesButton;
};

#endif
