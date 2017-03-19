#ifndef LUALEXER_H
#define LUALEXER_H

#include <QObject>

#include <Qsci/qscilexerlua.h>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qsciapis.h>
#include <QStringList>

class LuaLexer : public QsciLexerLua
{
public:
    LuaLexer(QObject* parent = nullptr);

    QStringList getAllKeywords() const;
    void installKeywordsToAutocomplete() const;
protected:
    virtual const char *keywords(int set) const override;
};

#endif // LUALEXER_H
