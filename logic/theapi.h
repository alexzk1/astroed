#ifndef THEAPI_H
#define THEAPI_H

#include "utils/inst_once.h"
#include "memimage/image_loader.h"
#include <vector>
#include <QObject>
#include <QString>
#include <QImage>

//okey, I want all aspects of the program be controllable from the LUA scripts.
//to do that easiest way is to put all functions in single place, and make some API from that


class TheAPI : public QObject, public utility::ItCanBeOnlyOne<TheAPI>
{
    Q_OBJECT
public:
    TheAPI();
    virtual ~TheAPI();
    void showPreview(const QString& fileName);
    void showStatusHint(const QString& hint, int delay = 10000);
signals:
    void showPreviewImage(const imaging::image_buffer_ptr& file);
};

#define THEAPI utility::globalInstance<TheAPI>()

#endif // THEAPI_H
