#ifndef CUSTOM_ROLES_H
#define CUSTOM_ROLES_H
#include <QObject>

// Define our new role that can be used in the model.
#define MyMouseCursorRole (Qt::UserRole + 1)
#define MyGetPathRole     (Qt::UserRole + 2)
#define MyScrolledView    (Qt::UserRole + 3)

#endif // CUSTOM_ROLES_H
