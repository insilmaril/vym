#ifndef OPTIONS_H
#define OPTIONS_H

#include <QStringList>

/*! \brief A single option which is listed in Options */
class Option {
  public:
    /*! Types of options */
    enum Type {
        Switch, //!< No paramater
        String  //!< Parameter is a string
    };

    Option();
    Option(const QString &, const Type &, const QString &, const QString &);
    void set(const QString &, const Type &, const QString &, const QString &);
    QString getName();
    QString getShort();
    QString getLong();
    Type getType();
    void setArg(const QString &);
    QString getArg();
    void setActive();
    bool isActive();

  private:
    QString name;
    Type type;
    QString sName;
    QString lName;
    QString sarg;
    bool active;
};

/*! \brief Simple class to deal with command line options */

class Options {
  public:
    Options();
    int parse();
    void add(Option);
    void add(const QString &, const Option::Type &, const QString &,
             const QString &);
    void setHelpText(const QString &);
    QString getHelpText();
    QString getProgramName();
    QStringList getFileList();
    bool isOn(const QString &);
    QString getArg(const QString &);

  private:
    QString progname;
    QString helptext;
    QStringList filelist;
    QList<Option> optlist;
};

#endif
