#include "showtextdialog.h"

#include "settings.h"
#include <QFont>
#include <QString>

extern Settings settings;
extern QFont fixedFont;

ShowTextDialog::ShowTextDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textBrowser->show();
    ui.textBrowser->setOpenExternalLinks(true);
}

void ShowTextDialog::append(const QString &s) { ui.textBrowser->append(s); }

void ShowTextDialog::setText(const QString &s) { ui.textBrowser->setText(s); }

void ShowTextDialog::setHtml(const QString &s) { ui.textBrowser->setHtml(s); }

void ShowTextDialog::useFixedFont(bool useFixedFont)
{
    QFont font;
    if (useFixedFont)
        ui.textBrowser->setFont(fixedFont);
}
