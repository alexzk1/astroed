#ifndef LEXER_TYPES_H
#define LEXER_TYPES_H
#include <memory>
#include <string>
#include <vector>
#include <QString>

struct CppExport
{
   const QString& funcName;
   const QString& paramsNames;
   const QString& hint;

   QString buildApiText() const
   {
       return QString("%1(%2) %3").arg(funcName).arg(paramsNames).arg(hint);
   }
};

using FunctionsListForLexer = std::vector<CppExport>;

#endif // LEXER_TYPES_H
