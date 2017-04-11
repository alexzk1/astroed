--[[
This is the most basic object in Luvit. It provides simple prototypal
inheritance and inheritable constructors. All other objects inherit from this.
]]
Object = {}
Object.meta = {__index = Object}

function dbgout(...) print(...) end
------------------------------------------------------------------------------------------------------------------


-- Create a new instance of this object
function Object:create()
    local meta = rawget(self, "meta")
    if not meta then error("Cannot inherit from instance object") end
    return setmetatable({}, meta)
end

--[[
Creates a new instance and calls `obj:initialize(...)` if it exists.

    local Rectangle = Object:extend()
    function Rectangle:initialize(w, h)
      self.w = w
      self.h = h
    end
    function Rectangle:getArea()
      return self.w * self.h
    end
    local rect = Rectangle:new(3, 4)
    p(rect:getArea())
]]
function Object:new(...)
    local obj = self:create()
    if type(obj.initialize) == "function" then
        obj:initialize(...)
    end
    return obj
end

--[[
Creates a new sub-class.

    local Square = Rectangle:extend()
    function Square:initialize(w)
      self.w = w
      self.h = h
    end
]]

function Object:extend()
    local obj = self:create()
    local meta = {}
    -- move the meta methods defined in our ancestors meta into our own
    --to preserve expected behavior in children (like __tostring, __add, etc)
    for k, v in pairs(self.meta) do
        meta[k] = v
    end
    meta.__index = obj
    meta.super=self
    obj.meta = meta
    return obj
end


local function printRecursive(f, tabs)
    if isDebugBuild() then
        local shift = string.rep('\t', tabs)
        if type(f) == "table" then
            for k, v in pairs(f) do
                if type(v)=="table" then
                    print (shift, k,":")
                    printRecursive(v, tabs + 1)
                else
                    print (shift, k, v)
                end
            end
        else
            print (shift, f)
        end
    end
end

function Object:dump(tabs, str)
    dbgout(str or "Dump")
    printRecursive(self, tabs or 1)
end

function Object:dumpAny(tbl, tabs, str)
    dbgout(str or "Dump")
    printRecursive(tbl, tabs or 1)
end

function Object:printField(name, tabs)
    if isDebugBuild() then
        dbgout('Printing Field:', name)
        local f = self[name];
        printRecursive(f, tabs or 1)
    end
end

function Object:instanceof(class, meself)
    local obj = meself or self
    if type(obj) ~= 'table' or obj.meta == nil or not class then
        return false
    end
    if obj.meta.__index == class then
        return true
    end
    local meta = obj.meta
    while meta do
        if meta.super == class then
            return true
        elseif meta.super == nil then
            return false
        end
        meta = meta.super.meta
    end
    return false
end
