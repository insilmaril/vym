#ifndef CONFLUENCEUSER_H
#define CONFLUENCEUSER_H

#include <QMetaType>
#include <QString>


class ConfluenceUser {
  public:
    ConfluenceUser() = default;
    ~ConfluenceUser() = default;
    ConfluenceUser (const ConfluenceUser &) = default;
    ConfluenceUser &operator=(const ConfluenceUser &) = default;
    void copy(const ConfluenceUser &);
    void setTitle(const QString &s);
    void setUrl(const QString &s);
    void setUserName(const QString &s);
    void setDisplayName(const QString &s);
    void setUserKey(const QString &s);

    QString getTitle();
    QString getUrl();
    QString getUserName();
    QString getDisplayName();
    QString getUserKey();

  private:
    QString title;
    QString url;
    QString userName;
    QString userKey;
    QString displayName;
};

Q_DECLARE_METATYPE(ConfluenceUser);

//! [custom type streaming operator]
//QDebug operator<<(QDebug dbg, const Message &message);

#endif
