#include "showtextdialog.h"

#include "settings.h"
#include <QFont>
#include <QString>
#include <QStringList>
#include <QTextDocument>

extern Settings settings;
extern QFont fixedFont;

ShowTextDialog::ShowTextDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textBrowser->show();
    ui.textBrowser->setOpenExternalLinks(true);

    originalIsHtml = false;

    connect(ui.findButton, SIGNAL(clicked()), this, SLOT(filterText()));
    connect(ui.findLineEdit, SIGNAL(returnPressed()), this, SLOT(filterText()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
}

void ShowTextDialog::append(const QString &s)
{
    if (!originalText.isEmpty())
        originalText += "\n";
    originalText += s;
    showText();
}

void ShowTextDialog::setText(const QString &s)
{
    originalText = s;
    originalIsHtml = false;
    showText();
}

void ShowTextDialog::setHtml(const QString &s)
{
    originalText = s;
    originalIsHtml = true;
    showText();
}

void ShowTextDialog::filterText() { showText(); }

void ShowTextDialog::clearFilter()
{
    ui.findLineEdit->clear();
    ui.findLineEdit->setFocus();
    showText();
}

void ShowTextDialog::showText()
{
    QString filter = ui.findLineEdit->text();

    if (filter.isEmpty()) {
        if (originalIsHtml)
            ui.textBrowser->setHtml(originalText);
        else
            ui.textBrowser->setText(originalText);
        return;
    }

    // Show only the lines containing the search text. Formatting of HTML
    // is lost while filtering, the plain text of the document is used.
    QString plainText;
    if (originalIsHtml) {
        QTextDocument doc;
        doc.setHtml(originalText);
        plainText = doc.toPlainText();
    }
    else
        plainText = originalText;

    QStringList lines;
    const QStringList allLines = plainText.split("\n");
    for (const QString &line : allLines)
        if (line.contains(filter, Qt::CaseInsensitive))
            lines << line;

    ui.textBrowser->setPlainText(lines.join("\n"));
}

void ShowTextDialog::useFixedFont(bool useFixedFont)
{
    QFont font;
    if (useFixedFont)
        ui.textBrowser->setFont(fixedFont);
}
