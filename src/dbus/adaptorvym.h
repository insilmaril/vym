#ifndef ADAPTORVYM_H
#define ADAPTORVYM_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class MainWindow;

class AdaptorVym : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.vym.main.adaptor")

  public:
    AdaptorVym(QObject *obj);

  public slots: // METHODS
    QDBusVariant mapCount();
    void gotoMapID(const uint &id);
    QDBusVariant getInstanceName();
    QDBusVariant getVersion();
    QDBusVariant execute(const QString &s);
    QDBusVariant listCommands();
    QDBusVariant currentMapID();

  Q_SIGNALS: // SIGNALS
    void crashed();
};

#endif
