#include "luaeditor.h"
#include <QString>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#ifdef _DEBUG
#include <QDebug>
#endif

LuaEditor::LuaEditor(QWidget *parent, const LuaLexerPtr &lexer):
    QsciScintilla(parent),
    lexer(lexer),
    isUntitled(true),
    currentFile()
{
    setLexer(lexer.get());

    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionCaseSensitivity(true);
    setAnnotationDisplay(AnnotationDisplay::AnnotationStandard);
    setAutoCompletionFillupsEnabled(true);
    setAutoCompletionReplaceWord(true);
    setAutoCompletionThreshold(2);
    setAutoCompletionUseSingle(AutoCompletionUseSingle::AcusExplicit);

    setUtf8(true);
    setMarginLineNumbers(1, true);
    setMarginWidth(1, 35);
    setTabIndents(true);
    setIndentationGuides(true);
    setIndentationsUseTabs(true);
    setAutoIndent(true);

    setBackspaceUnindents(true);
    setTabWidth(4);
    setFolding(QsciScintilla::BoxedTreeFoldStyle);
    setBraceMatching(QsciScintilla::StrictBraceMatch);

    setWrapMode(QsciScintilla::WrapWord);


    connect(this, &LuaEditor::textChanged, this, [this]()
    {
        setWindowModified(isModified());
    }, Qt::QueuedConnection);
}

LuaLexerPtr LuaEditor::createLuaLexer()
{
    LuaLexer* p;
    LuaLexerPtr lexer(p = new LuaLexer());

    lexer->setFoldCompact(false);
    lexer->setAPIs(new QsciAPIs(lexer.get()));
    p->installKeywordsToAutocomplete();

   // lexer->setAutoIndentStyle(1);
//        lexer->setColor(QColor(128, 128, 255), 5);
//        lexer->setColor(QColor(0, 220, 0), 1);
//        lexer->setColor(QColor(0, 220, 0), 2);
//        lexer->setColor(QColor(Qt::red), 6);
//        lexer->setColor(QColor(Qt::red), 8);
//        lexer->setColor(QColor(255, 128, 0), 13);
//        lexer->setColor(QColor(51, 102, 255), 15);
//        lexer->setColor(QColor(72, 61, 139), 10);
    //    lexer->setFont(QFont("Courier New", 11, QFont::Bold));

    return lexer;
}

LuaLexerPtr LuaEditor::getSharedLexer()
{
    static LuaLexerPtr sharedLexer(nullptr);
    if (!sharedLexer)
        sharedLexer = createLuaLexer();
    return sharedLexer;
}

void LuaEditor::newFile()
{
    static int sequence = 1;
    isUntitled = true;
    currentFile = tr("newScript%1.lua").arg(sequence++);
    setWindowTitle(tr("%1[*]").arg(currentFile));
    setModified(false);
}

bool LuaEditor::openFile(const QString &file)
{
    QFile f(file);
    if(!f.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Unable to open file!",
                             tr("Could not read file %1.\nError string: %2")
                             .arg(currentFile, f.errorString()), QMessageBox::Ok,
                             QMessageBox::NoButton);
        return false;
    }
    QTextStream stream(&f);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    this->setText(stream.readAll());
    QApplication::restoreOverrideCursor();
    setCurrentFile(file);
    return true;
}

void LuaEditor::setCurrentFile(const QString &file)
{
    currentFile = QFileInfo(file).canonicalFilePath();
    isUntitled = false;
    setModified(false);
    setWindowModified(false);
}

QString LuaEditor::getStrippedName(const QString &fullPath)
{
    return QFileInfo(fullPath).fileName();
}

QString LuaEditor::getCurrentFile()
{
    return getStrippedName(currentFile);
}

bool LuaEditor::saveAs()
{
    QString file = QFileDialog::getSaveFileName(this, "Save As...", "", "Lua Scripts (*.lua)");

    if(file.isEmpty())
        return false;

    return saveFile(file);
}

bool LuaEditor::save()
{
    if(this->isUntitled)
        return saveAs();
    else
        return saveFile(currentFile);
}

bool LuaEditor::saveFile(const QString &name)
{
    QFile file(name);
    if(!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Unable to save file!",
                             tr("Could not save file %1.\nError string: %2")
                             .arg(name, file.errorString()), QMessageBox::Ok,
                             QMessageBox::NoButton);
        return false;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QTextStream stream(&file);
    stream << this->text();
    setCurrentFile(name);
    QApplication::restoreOverrideCursor();
    return true;
}

void LuaEditor::addCppApiFunc(const QStringList &func)
{
    //fixme: not sure how this works really, if I can add 1-by-1 or all-at-once
    if (lexer)
    {
        auto apis = qobject_cast<QsciAPIs*>(lexer->apis());
        if (apis)
        {
            for (const auto& f : func)
                apis->add(f);
            apis->prepare();
        }
    }
}

void LuaEditor::closeEvent(QCloseEvent *event)
{
    save();
    event->accept();
}

void LuaEditor::keyPressEvent(QKeyEvent *event)
{
    bool got = false;
    switch (event->key()) {
        case Qt::Key_Space:
            if (event->modifiers() & Qt::ControlModifier)
            {
                callTip();
                got = true;
            }
            break;
        case Qt::Key_Return:
            if (event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::AltModifier)
            {
                autoCompleteFromAll();
                got = true;
            }
            break;
    }
    if (!got)
        QsciScintilla::keyPressEvent(event);
}
