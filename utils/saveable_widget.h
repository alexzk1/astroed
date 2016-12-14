#pragma once

#include <QObject>
#include <QWidget>
#include <QSettings>
namespace utility
{
class SaveableWidget
{
private:
    struct LastSett
    {
        QPoint pos;
        QSize  size;
    };
    LastSett lastSett;
protected:
    void readSettings(QWidget* window)
    {
        LastSett tmp;
        tmp.size = window->size();
        tmp.pos  = window->pos();
        lastSett = tmp;

        QSettings settings;

        settings.beginGroup(window->objectName());
        recurseRead(settings, window);

        QVariant value = settings.value("pos");
        if (!value.isNull())
        {
            window->move(settings.value("pos").toPoint());
            window->resize(settings.value("size").toSize());
        }

        settings.endGroup();
    }

    void writeSettings(QWidget* window)
    {
        QSettings settings;

        settings.beginGroup(window->objectName());
        settings.setValue("pos", window->pos());
        settings.setValue("size", window->size());
        recurseWrite(settings, window);
        settings.endGroup();
        settings.sync();
    }

    virtual void recurseRead(QSettings& settings, QObject* object)
    {
        Q_UNUSED(settings)
        Q_UNUSED(object)
    }
    virtual void recurseWrite(QSettings& settings, QObject* object)
    {
        Q_UNUSED(settings)
        Q_UNUSED(object)
    }
    virtual ~SaveableWidget()
    {
    }

public:
    void resetSizePos(QWidget* window)
    {
        LastSett tmp = lastSett;
        window->move(tmp.pos);
        window->resize(tmp.size);
    }
};
}
