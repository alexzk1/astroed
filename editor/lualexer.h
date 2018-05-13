#ifndef LUALEXER_H
#define LUALEXER_H

#include <QObject>

#include <Qsci/qscilexerlua.h>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qsciapis.h>
#include <QStringList>
#include <memory>
#include <string>
#include <vector>
#include "lexer_types.h"


class LuaLexer : public QsciLexerLua
{
public:
    LuaLexer(QObject* parent = nullptr, const FunctionsListForLexer &exports = FunctionsListForLexer());

protected:
    const char *keywords(int set) const override;
    QColor defaultPaper(int style) const override;
    QColor defaultColor(int style) const override;
    QFont  defaultFont(int style) const override;
    const char* blockStartKeyword(int *style = nullptr)const override;
    const char* blockStart(int *style = nullptr)const override;
    const char* blockEnd(int* style = nullptr)const override;
    int  blockLookback() const override;
private:
    static std::string genExportedKw(const FunctionsListForLexer& exports);
    void installCppExports(const FunctionsListForLexer& exports);
    QStringList getAllKeywords() const;
    const std::string lastJoinedApis;
};

using LuaLexerPtr = std::shared_ptr<LuaLexer>;
#endif // LUALEXER_H
