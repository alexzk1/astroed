// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "luaqrcpackager.h"
#include "utils/fatal_err.h"
#include <QDebug>
#include <QFile>
#include <QDirIterator>
#include <QFileInfo>

const QString LuaQrcPackager::defaultPrefix = ":/luarc";

LuaQrcPackager::LuaQrcPackager(const QString& resPrefix):
    resPrefix(resPrefix)
{

}

QString LuaQrcPackager::buildLoadPath(const QString &resource, const QString &resPrefix)
{
    return QString("%1/%2").arg(buildLoadDir(resPrefix)).arg(resource);
}

QString LuaQrcPackager::buildLoadPath(const QString &res) const
{
    return buildLoadPath(res, resPrefix);
}

QByteArray LuaQrcPackager::loadResource(const QString &res) const
{
    return loadResource(res, resPrefix);
}

QString LuaQrcPackager::buildLoadDir(const QString &resPrefix)
{
    return resPrefix;
}

QStringList LuaQrcPackager::getEmbeddedScriptsList() const
{
    QStringList res;
    QDirIterator it(getDefaultPrefix(), QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto fp = it.next();
        auto fi = it.fileInfo();
        if (fi.suffix().toLower() == "lua")
            res.push_back(fp);
    }
    return res;
}

const QString& LuaQrcPackager::getDefaultPrefix()
{
    return defaultPrefix;
}

bool LuaQrcPackager::isFileExists(const QString &fn)
{
    QFile mainf(buildLoadPath(fn, resPrefix));
    return mainf.exists();
}

QByteArray LuaQrcPackager::loadResource(const QString &resource, const QString &resPrefix)
{
    QByteArray r = "";
    QFile mainf(buildLoadPath(resource, resPrefix));
    try
    {
        if (mainf.open(QIODevice::ReadOnly))
            r = mainf.readAll();
    }
    catch(std::exception& e)
    {
        FATAL_RISE(e.what());
    }
#ifdef _DEBUG
    //qDebug() << "loadResource: "<< package << resource << ", Size: "<<r.length();
#endif
    return r;
}

