#include "url-dialog.h"

UrlDialog::UrlDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.urlLE, SIGNAL(textEdited(const QString &)), this, SLOT(updateText(QString)));
    connect(ui.textLE, SIGNAL(textEdited(const QString &)), this, SLOT(textEdited()));
    updateTextWithUrl = true;
}

void UrlDialog::setUrl(const QString &s)
{
    ui.urlLE->setText(s);
}

QString UrlDialog::url() {return ui.urlLE->text();}

void UrlDialog::setText(const QString &s)
{
    ui.textLE->setText(s);
    updateTextWithUrl = false;
}

QString UrlDialog::text() {return ui.textLE->text();}

void UrlDialog::textEdited()
{
    updateTextWithUrl = false;
}

void UrlDialog::updateText(const QString &s)
{
    // Only update text with url, if it has not been
    // explicitely set so far
    if (updateTextWithUrl)
        ui.textLE->setText(s);
}
