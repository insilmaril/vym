#ifndef ADAPTORModel_H
#define ADAPTORModel_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class VymModel;
class QString;

class AdaptorModel: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.Adaptor")
//    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.Adaptor.test")
//	Q_PROPERTY(QString caption READ caption WRITE setCaption)
//	Q_PROPERTY(QString heading READ getHeading WRITE setHeading)
private:
	VymModel *model;

public:
    AdaptorModel(QObject *obj);
    virtual ~AdaptorModel();
	void setModel (VymModel *vm);

public: // PROPERTIES
	QString m_caption;
	QString caption();
	void setCaption(const QString &newCaption);
public slots: // METHODS
   QDBusVariant query(const QString &);
   QDBusVariant getCurrentModelID();
   QDBusVariant getHeading();
   void setHeading (const QString &s);
   QDBusVariant getInstanceName();
   QDBusVariant execute (const QString &s);

Q_SIGNALS: // SIGNALS
    void crashed();
};

#endif
