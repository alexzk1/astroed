#ifndef LUALEXER_H
#define LUALEXER_H

#include <QObject>

#include <Qsci/qscilexerlua.h>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qsciapis.h>
#include <QStringList>
#include <memory>

class LuaLexer : public QsciLexerLua
{
public:
    LuaLexer(QObject* parent = nullptr);

    QStringList getAllKeywords() const;
    void installKeywordsToAutocomplete() const;
protected:
    virtual const char *keywords(int set) const override;
    virtual QColor defaultPaper(int style) const override;
    virtual QColor defaultColor(int style) const override;
    virtual QFont  defaultFont(int style) const override;
    virtual const char* blockStartKeyword(int *style = 0)const override;
    virtual const char* blockStart(int *style = 0)const override;
    virtual const char* blockEnd(int* style = 0)const override;
    virtual int  blockLookback() const override;
};

using LuaLexerPtr = std::shared_ptr<LuaLexer>;
#endif // LUALEXER_H
