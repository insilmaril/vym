#ifndef ADAPTORModel_H
#define ADAPTORModel_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class VymModel;
class QString;

class AdaptorModel: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.vym.model.adaptor")

private:
	VymModel *model;

public:
    AdaptorModel(QObject *obj);
    void setModel (VymModel *vm);

public: // PROPERTIES
    QString m_caption;
    QString caption();
    void setCaption(const QString &newCaption);

public slots: // METHODS
    QDBusVariant getCurrentModelID();
    QDBusVariant getHeading();
    void setHeading (const QString &s);
    QDBusVariant branchCount();
    QDBusVariant execute (const QString &s);
    QDBusVariant listCommands();

Q_SIGNALS: // SIGNALS
    void crashed();
};

#endif
