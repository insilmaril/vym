#include <QMenu>
#include <QMouseEvent>

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
}

void MyTextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers() & Qt::ControlModifier) {
        qDebug() << "MyTextEdit" << __func__ << " anchor=" << anchorAt(e->pos());   // FIXME-2
    } else
        QTextEdit::mousePressEvent(e);
}

void MyTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(actionOpenUrl);

    lastContextMenuPos = e->pos();
    menu->exec(e->globalPos());
    delete menu;
}

void MyTextEdit::openUrlTriggered()
{
    if (mainWindow)
        // mainWindow is created AFTER editors, so in theory might still be nullptr
        mainWindow->openUrl(anchorAt(lastContextMenuPos));
}

