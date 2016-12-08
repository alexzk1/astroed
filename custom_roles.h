#ifndef CUSTOM_ROLES_H
#define CUSTOM_ROLES_H
#include <QObject>

// Define our new role that can be used in the model.
constexpr int MyMouseCursorRole = Qt::UserRole + 1;

// Define a new constant for user-defined roles in this application.
constexpr int MyUserRole = MyMouseCursorRole + 1;

#endif // CUSTOM_ROLES_H
