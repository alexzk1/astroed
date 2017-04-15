#ifndef LUAQRCPACKAGER_H
#define LUAQRCPACKAGER_H
#include <QString>
#include <QByteArray>
#include <string>
#include "editor/lexer_types.h"

class LuaQrcPackager
{
private:
    const QString& resPrefix; //maybe usefull one day...
    const static QString defaultPrefix;

protected:
    static const QString &getDefaultPrefix();
    static QString     buildLoadPath(const QString& resource, const QString& resPrefix);
    inline bool isFileExists(const QString& fn);
    static QByteArray  loadResource(const QString& resource, const QString& resPrefix);
public:
    LuaQrcPackager(const QString& resPrefix = getDefaultPrefix());
    QString        buildLoadPath(const QString &res) const;
    QByteArray     loadResource(const QString& res) const;

    static QString buildLoadDir(const QString& resPrefix = getDefaultPrefix());
    QStringList getEmbeddedScriptsList() const;
};

#endif // LUAQRCPACKAGER_H
