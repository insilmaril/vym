#include <QDebug>
#include <QMultiMap>

#include <iostream>

#include "shortcuts.h"

#include "misc.h"

/////////////////////////////////////////////////////////////////
// KeySwitch
/////////////////////////////////////////////////////////////////
KeySwitch::KeySwitch(const QString &identifier,
                     const QString &scope,
                     const QString &tag,
                     QAction* action)
{
    identifierInt = identifier;
    scopeInt = scope;
    tagInt = tag;
    actionInt = action;
}

/////////////////////////////////////////////////////////////////
// Switchboard
/////////////////////////////////////////////////////////////////
Switchboard::Switchboard() {}

QList <KeySwitch> Switchboard::valuesReversed(const QString &scope)
{
    QList<KeySwitch> list;
    if (!scope.isEmpty())
        list = switchesMap.values(scope);
    else
        list = switchesMap.values();
    QList<KeySwitch> rlist;

    QListIterator<KeySwitch> it(list);
    it.toBack();
    while (it.hasPrevious()) {
        rlist << it.previous();
    }
    return rlist;
}

void Switchboard::addScope(QString scopeIdentifier, QString scopeName)
{
    if (!scopesMap.contains(scopeIdentifier))
        scopesMap.insert(scopeIdentifier, scopeName);
}

void Switchboard::addAction(QAction *action,
        const QString &identifier,
        const QString &scope,
        const QString &tag)
{
    addAction(action, identifier, QKeySequence(), scope, tag);
}

void Switchboard::addAction(QAction *action,
        const QString &identifier,
        QKeySequence ks,
        const QString &scope,
        const QString &tag)
{
    action->setShortcut(ks);
    action->setShortcutVisibleInContextMenu(true); // FIXME-3 should obsolete setting in MainWindow::setupContextMenus()

    if (!switchesMap.contains(identifier)) {
        if (!action->shortcut().toString().isEmpty()) {
            // Add shortcut to tooltip
            action->setToolTip(action->toolTip() + 
                    QString(" (%1)").arg(action->shortcut().toString()));
        }
        KeySwitch ksw(identifier, scope, tag, action);
        switchesMap.insert(scope, ksw);
    }
    else
        qDebug()
            << "Warning switchboard::addAction warning: Existing idenifier "
            << identifier;
}


QString Switchboard::getASCII()
{
    QString s;
    QString scope;
    foreach (scope, switchesMap.uniqueKeys()) {
        s += underline(scope, "=");

        QStringList tagsInScope;
        foreach (auto ksw, switchesMap.values(scope)) {
            if (!tagsInScope.contains(ksw.tagInt))
                tagsInScope << ksw.tagInt;
        }

        foreach (auto tag, tagsInScope) {
            s += underline(tag, "-");
            foreach (auto ksw, valuesReversed(scope)) {
                if (ksw.tagInt == tag) {
                    QString desc = ksw.actionInt->text();
                    QString sc = ksw.actionInt->shortcut().toString();
                    QString alt;
                    if (ksw.identifierInt.contains("VimAlt"))
                        alt = QObject::tr("(Vim alternative shortcut)","Shortcut help dialog");
                    if (!sc.isEmpty()) {
#if defined(Q_OS_MACOS)
                        sc.replace("Ctrl","Cmd");
#endif
                    
                        desc = desc.remove('&');
                        desc = desc.remove("...");
                        s += QString(" %1: %2 %3\n").arg(sc, 12).arg(desc).arg(alt);
                    }
                }
            }
            if (tag != tagsInScope.last())
                s += "\n";
        }
        s += "\n";
    }

    // Finally list vim inspired shortcuts
    s += underline("VIM-like shortcuts", "=");
    foreach (auto ksw, valuesReversed()) {
        QString sc = ksw.actionInt->shortcut().toString();
        if (!sc.isEmpty() && ksw.identifierInt.contains("Vim")) {
            QString desc = ksw.actionInt->text();
            desc = desc.remove('&');
            desc = desc.remove("...");
            s += QString(" %1: %2\n").arg(sc, 12).arg(desc);
        }
    }

    return s;
}

void Switchboard::printASCII() { std::cout << qPrintable(getASCII()); }

void Switchboard::printLaTeX()
{
    /*
    QString g;
    foreach (g, actionsMap.uniqueKeys()) {
        std::cout << "Group: " << qPrintable(g) << "\\\\ \\hline" << std::endl;
        QList<QAction *> values = actionsMap.values(g);
        for (int i = 0; i < values.size(); ++i)
            if (!values.at(i)->shortcut().toString().isEmpty()) {
                QString desc = values.at(i)->text();
                QString sc = values.at(i)->shortcut().toString();
                desc = desc.remove('&');
                desc = desc.remove("...");
                std::cout << qPrintable(QString(" %1& %2").arg(sc, 12).arg(desc))
                     << std::endl;
            }
        std::cout << std::endl;
    }
    */
}
