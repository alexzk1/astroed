#ifndef LUAEDITOR_H
#define LUAEDITOR_H

#include <memory>
#include "lualexer.h"

using LuaLexerPtr = std::shared_ptr<QsciLexerLua>;

class LuaEditor : public QsciScintilla
{
    Q_OBJECT
public:
    LuaEditor(QWidget *parent = nullptr, const LuaLexerPtr& lexer = getSharedLexer());
    static LuaLexerPtr createLuaLexer();
    static LuaLexerPtr getSharedLexer(); //single instance, used to share between all tabs or so

    void newFile();
    bool openFile(const QString &file);
    bool save();
    bool saveAs();
    bool saveFile(const QString &name);
    QString currentFileP() { return currentFile; }
    bool loadWordsFromFile(const QString &fileName);
    void addCppApiFunc(const QStringList& func);

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    LuaLexerPtr lexer;
    bool isUntitled;
    QString currentFile;

    void setCurrentFile(const QString &file);
    QString getStrippedName(const QString &fullPath);
    QString getCurrentFile();

};

#endif // LUAEDITOR_H
