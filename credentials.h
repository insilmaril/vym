#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include "ui_credentials.h"

class CredentialsDialog : public QDialog {
    Q_OBJECT

  public:
    CredentialsDialog(QWidget *parent = 0);
    void setURL(const QString &u);
    QString getURL();
    void setUser(const QString &u);
    QString getUser();
    void setPassword(const QString &p);
    QString getPassword();
    void setSavePassword(const bool &b);
    bool savePassword();

  private:
    Ui::CredentialsDialog ui;

};

#endif // CREDENTIALS_H
