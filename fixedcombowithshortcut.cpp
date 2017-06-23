#include "fixedcombowithshortcut.h"
#include <QAbstractItemView>
#include <QEvent>
#include <QKeyEvent>

FixedComboWithShortcut::FixedComboWithShortcut(QWidget *parent):
    QComboBox (parent),
    sequences()
{
    view()->installEventFilter(this);
    installEventFilter(this);
}

void FixedComboWithShortcut::addItemWithShortcut(const QString &text, const QKeySequence &shortcut, QVariant userData)
{
    QString txt = QString("%1 (%2)").arg(text).arg(shortcut.toString());
    addItem(txt, userData);
    sequences[shortcut] = txt;
}

bool FixedComboWithShortcut::eventFilter(QObject *t, QEvent *e)
{
    Q_UNUSED(t);
    QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(e);
    if (keyEvent)
    {
        //this will support 1-key shortcuts (with modifiers, something like ctrl+k+l is NOT supported)
        const QKeySequence ks{keyEvent->key()};
        if (sequences.count(ks))
        {
            if(e->type() == QEvent::KeyPress)
            {
                setCurrentText(sequences.at(ks));
            }
            else
                hidePopup();
            return true;
        }

    }

    return false; //keep handling
}
