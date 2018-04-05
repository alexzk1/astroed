#ifndef FIXEDCOMBOWITHSHORTCUT_H
#define FIXEDCOMBOWITHSHORTCUT_H
#include <QComboBox>
#include <QKeySequence>
#include <map>
//this class allows to assign individual shortcuts per contained item
class FixedComboWithShortcut : public QComboBox
{
    Q_OBJECT
public:
    FixedComboWithShortcut(QWidget *parent = nullptr);
    void addItemWithShortcut(const QString& text, const QKeySequence& shortcut, QVariant userData = QVariant());
private:
    std::map<QKeySequence, QString> sequences;
    virtual bool eventFilter(QObject *t, QEvent *e) override;
};

#endif // FIXEDCOMBOWITHSHORTCUT_H
