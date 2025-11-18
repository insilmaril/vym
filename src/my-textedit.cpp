#include <QMenu>
#include <QMouseEvent>
#include <QTextBlock>

#include "mainwindow.h"

#include "my-textedit.h"

extern Main *mainWindow;

MyTextEdit::MyTextEdit(QWidget *parent)
{
    //qDebug() << "Constr MyTextEdit";
    QAction *a = new QAction(QPixmap(":/flag-url.svg"), tr("Open URL", "TextEdit menu"),
                    this);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(openUrlTriggered()));
    actionOpenUrl = a;
    setRichTextMode(false);
}

bool  MyTextEdit::richTextMode()
{
    return richTextModeInt;
}

void MyTextEdit::setRichTextMode(bool b)
{
    richTextModeInt = b;
    actionOpenUrl->setEnabled(richTextModeInt);
    if (richTextModeInt) {
    } else {
    }
}

void MyTextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier) {
        if (mainWindow) {
            QString url = anchorAt(e->pos());
            mainWindow->openUrl(url);

            // Note used currently: text
            // QTextCursor c = cursorForPosition(e->pos());
            // QString text = c.block().text();
        }

    } else
        QTextEdit::mousePressEvent(e);
}

void MyTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();

    if (richTextModeInt) {
        menu->addSeparator();
        menu->addAction(actionOpenUrl); // FIXME-4 also add image actions
    }

    lastContextMenuPos = e->pos();
    menu->exec(e->globalPos());
    delete menu;
}

void MyTextEdit::openUrlTriggered()
{
    if (mainWindow)
        // mainWindow is created AFTER editors, so in theory mainWindow might still be nullptr
        mainWindow->openUrl(anchorAt(lastContextMenuPos));
}

