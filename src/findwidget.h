#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include <QWidget>

class QAction;
class QGroupBox;
class QComboBox;
class QPushButton;

class FindWidget : public QWidget {
    Q_OBJECT

  public:
    enum Status { Undefined, Success, Failed };

    FindWidget(QWidget *parent = nullptr);
    QString getFindText();

  public slots:
    void cancelPressed();
    void nextPressed();
    void findTextChanged(const QString &);
    void setFocus();
    void setStatus(Status st);

  private:
    Status status;

  signals:
    void hideFindWidget();
    void nextButtonPressed(QString, bool);

  private:
    QComboBox *findcombo;
    QGroupBox *findbox;
    QPushButton *nextButton;
    QPushButton *filterNotesButton;
};

#endif
