#include "lualexer.h"
#include <QString>
#include <sstream>

LuaLexer::LuaLexer(QObject *parent, const std::vector<CppExport> &exports):
    QsciLexerLua(parent),
    lastJoinedApis(genExportedKw(exports))
{
    installCppExports(exports);

    setFoldCompact(false);
    //setAutoIndentStyle(QsciScintilla::AiOpening | QsciScintilla::AiClosing);
    //setAutoIndentStyle(QsciScintilla::AiClosing);
    //setAutoIndentStyle(QsciScintilla::AiMaintain);
    setAutoIndentStyle(0); //block structured
}

QStringList LuaLexer::getAllKeywords() const
{
    QStringList res;
    for (int i = 1; ; ++i)
    {
        auto sptr = keywords(i);
        if (!sptr)
            break;
        QString str(sptr);
        res.append(str.split(" ", QString::SkipEmptyParts));
    }
    return res;
}

void LuaLexer::installCppExports(const std::vector<CppExport> &exports)
{
    auto apis = qobject_cast<QsciAPIs*>(this->apis());
    if (apis)
    {
        setAPIs(nullptr);
        delete apis;
    }
    //once prepared, nothing can be added it seems
    setAPIs(apis = new QsciAPIs(this));

    if (apis)
    {
        auto tmp = getAllKeywords();
        for (const auto& f : tmp)
            apis->add(f);

        for (const auto& f : exports)
        {
            auto s = f.buildApiText();
            apis->add(s);
        }
        apis->prepare();
    }
}

const char *LuaLexer::keywords(int set) const
{
    if (set == 1)
        // Keywords.
        return
                "and break do else elseif end for function if "
                "in local not or repeat return then until "
                "while";

    if (set == 2)
        // Basic functions.
        return
                "_ALERT _ERRORMESSAGE _INPUT _PROMPT _OUTPUT _STDERR "
                "_STDIN _STDOUT call dostring getn "
                "globals newtype rawget rawset require sort "

                "G getfenv getmetatable ipairs loadlib next pairs "
                "pcall rawegal rawget rawset require setfenv "
                "setmetatable xpcall string table math coroutine io "
                "os print tostring tonumber";

    if (set == 3)
        // String, table and maths functions.
        return
                "abs acos asin atan atan2 ceil cos deg exp floor "
                "format frexp gsub ldexp log log10 max min mod rad "
                "random randomseed sin sqrt strbyte strchar strfind "
                "strlen strlower strrep strsub strupper tan "

                "string.byte string.char string.dump string.find "
                "string.len string.lower string.rep string.sub "
                "string.upper string.format string.gfind string.gsub "
                "table.concat table.foreach table.foreachi table.getn "
                "table.sort table.insert table.remove table.setn "
                "math.abs math.acos math.asin math.atan math.atan2 "
                "math.ceil math.cos math.deg math.exp math.floor "
                "math.frexp math.ldexp math.log math.log10 math.max "
                "math.min math.mod math.pi math.rad math.random "
                "math.randomseed math.sin math.sqrt math.tan";

    if (set == 4)
        // Coroutine, I/O and system facilities.
        return
                "openfile closefile readfrom writeto appendto remove "
                "rename flush seek tmpfile tmpname read write clock "
                "date difftime execute exit getenv setlocale time "

                "coroutine.create coroutine.resume coroutine.status "
                "coroutine.wrap coroutine.yield io.close io.flush "
                "io.input io.lines io.open io.output io.read "
                "io.tmpfile io.type io.write io.stdin io.stdout "
                "io.stderr os.clock os.date os.difftime os.execute "
                "os.exit os.getenv os.remove os.rename os.setlocale "
                "os.time os.tmpname";
    if (set == 5)
        return "self true false nil";

    if (set == 6) //that MUST BE set from constructor
        return lastJoinedApis.c_str();

    return nullptr;
}

QFont LuaLexer::defaultFont(int style) const
{
    auto f = QsciLexerLua::defaultFont(style);
    if (f.pointSize() < 12)
        f.setPointSize(12);
    if (style == KeywordSet5 || style == KeywordSet6)
        f.setItalic(true);

    if (style == Keyword)
        f.setBold(true);

    return f;
}

int LuaLexer::blockLookback() const
{
    return 200;
}

std::string LuaLexer::genExportedKw(const std::vector<CppExport> &exports)
{
    std::vector<std::string> lastApis;
    for (const auto& f : exports)
        lastApis.push_back(f.funcName.toStdString());
    std::stringstream res;
    std::copy(lastApis.begin(), lastApis.end(), std::ostream_iterator<std::string>(res, " "));
    return res.str();
}

const char *LuaLexer::blockStartKeyword(int *style) const
{
    //this is should be list which starts 1-line block like
    //while () ;
    //in C++, it is 1 line block, absent in lua such
    if (style)
        *style = Keyword;
    return nullptr;
}

const char *LuaLexer::blockStart(int *style) const
{
    if (style)
        *style = Keyword;
    return "do then else function";
}

const char *LuaLexer::blockEnd(int *style) const
{
    if (style)
        *style = Keyword;
    return "end";
}

QColor LuaLexer::defaultPaper(int style) const
{
    return QsciLexer::defaultPaper(style);
}

// Returns the foreground colour of the text for a style.
QColor LuaLexer::defaultColor(int style) const
{

    switch (style)
    {
        case Default:
            return QColor(0x00,0x00,0x00);

        case Comment:
        case LineComment:
            return QColor(0x00,0x7f,0x00);

        case Number:
            return QColor(0x00,0x7f,0x7f);

        case KeywordSet5:
            return QColor(0x4b,0x00,0x82);

        case KeywordSet6: //this is reserved set for C++ exported API
            return QColor(0x30,0xD5,0xC8);

        case Keyword:
        case BasicFunctions:
        case StringTableMathsFunctions:
        case CoroutinesIOSystemFacilities:
            return QColor(0x00,0x00,0x7f);

        case String:
        case Character:
        case LiteralString:
            return QColor(0x7f,0x00,0x7f);

        case Preprocessor:
        case Label:
            return QColor(0x7f,0x7f,0x00);
        case Identifier:
        case Operator:
            break;

    }

    return QsciLexer::defaultColor(style);
}

