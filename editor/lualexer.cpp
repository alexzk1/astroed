#include "lualexer.h"
#include <QString>

LuaLexer::LuaLexer(QObject *parent):
    QsciLexerLua(parent)

{
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

void LuaLexer::installKeywordsToAutocomplete() const
{
    auto apis = qobject_cast<QsciAPIs*>(this->apis());
    if (apis)
    {
        auto tmp = getAllKeywords();
        for (const auto& f : tmp)
            apis->add(f);
        apis->prepare();
    }
}

const char *LuaLexer::keywords(int set) const
{
    if (set == 1)
            // Keywords.
            return
                "and break do else elseif end false for function if "
                "in local nil not or repeat return then true until "
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
                "os debug";

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
                "os.time os.tmpname print";

        return nullptr;
}
