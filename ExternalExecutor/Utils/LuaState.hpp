#pragma once
#include <cstdint>
#include <cstdio>
#include "Memory.hpp"

#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TSTRING 4
#define LUA_TTABLE 5
#define LUA_TFUNCTION 6
#define LUA_TUSERDATA 7
#define LUA_TTHREAD 8

struct TValue {
    union { double n; void* p; int b; } value;
    uint8_t tt_;
    uint8_t flags;
    uint16_t memcat;
};

struct GCObject {
    uint8_t memcat, tt, marked;
    GCObject* next;
};

struct TString {
    uint8_t memcat, tt, marked, flag;
    int16_t atom;
    TString* next;
    unsigned int hash, len;
    char data[1];
};

struct LuaTable {
    uint8_t memcat, tt, marked, nodemask8, lsizenode, safeenv, readonly, tmcache;
    int sizearray, lastfree;
    uintptr_t node, array;
    GCObject* gclist;
    LuaTable* metatable;
};

struct LuaNode {
    TValue key;
    TValue value;
};

struct Proto {
    uint8_t memcat, tt, marked, maxstacksize, is_vararg, numparams, flags, nups;
    uintptr_t abslineinfo, codeentry, debuginsn, userdata, upvalues;
    GCObject* gclist;
    uintptr_t k, code, p, locvars, source, execdata, exectarget, debugname, lineinfo, typeinfo;
    int sizek, sizep, sizetypeinfo, sizelocvars, linedefined, sizelineinfo, sizecode, linegaplog2, sizeupvalues, bytecodeid;
};

struct Closure {
    uint8_t memcat, tt, marked, stacksize, preload, isC, nupvalues;
    GCObject* gclist;
    LuaTable* env;
    union {
        struct { uintptr_t f, cont, debugname; TValue upvals[1]; } c;
        struct { Proto* p; TValue uprefs[1]; } l;
    };
};

struct CallInfo {
    uintptr_t savedpc, top, func, base;
    int nresults;
    unsigned int flags;
};

struct UpVal {
    GCObject* next;
    uintptr_t v;
    TValue value;
};

struct RobloxExtraSpace {
    uint8_t pad1[0x18];
    uintptr_t sharedExtraSpace;
    uint8_t pad2[0x20];
    uintptr_t capabilities;
    uint8_t pad3[0x28];
    uint32_t identity;
    uint8_t pad4[0x14];
    uintptr_t script;
};

struct lua_State {
    uint8_t memcat, tt, marked;
    uint8_t status;
    uint8_t activememcat;
    bool isactive;
    bool singlestep;
    uint8_t pad1[2];
    uintptr_t userdata;
    GCObject* gclist;
    uint16_t nCcalls;
    uint16_t baseCcalls;
    int cachedslot;
    LuaTable* gt;
    uintptr_t top;
    uintptr_t base;
    uintptr_t stack_last;
    CallInfo* ci;
    uintptr_t stack;
    uintptr_t global;
    CallInfo* end_ci;
    CallInfo* base_ci;
    uintptr_t openupval;
    int stacksize;
    int size_ci;
    uintptr_t namecall;
};

struct lua_Debug
{
    const char* name;
    const char* what;
    const char* source;
    const char* short_src;
    int linedefined;
    int currentline;
    unsigned char nupvals;
    unsigned char nparams;
    char isvararg;
    void* userdata;
    char ssbuf[256];
};

struct lua_Callbacks
{
    void* userdata;
    void (*debugprotectederror)(lua_State* L);
    void (*userthread)(lua_State* LP, lua_State* L);
    void (*panic)(lua_State* L, int errcode);
    void (*debugbreak)(lua_State* L, lua_Debug* ar);
    int16_t(*useratom)(lua_State* L, const char* s, size_t l);
    void (*debugstep)(lua_State* L, lua_Debug* ar);
    void (*interrupt)(lua_State* L, int gc);
    void (*debuginterrupt)(lua_State* L, lua_Debug* ar);
    void (*onallocate)(lua_State* L, size_t osize, size_t nsize);
};

struct global_State {
    uintptr_t strt_hash;
    int strt_size;
    uint32_t strt_nuse;
    GCObject* weak;
    GCObject* grayagain;
    GCObject* gray;
    int gcgoal;
    int gcstepmul;
    int gcstepsize;
    uintptr_t frealloc;
    uintptr_t ud;
    uint8_t currentwhite;
    uint8_t gcstate;
    uint8_t pad1[6];
    size_t GCthreshold;
    size_t totalbytes;
    uintptr_t freepages[40];
    uintptr_t allpages;
    uintptr_t mainthread;
    uintptr_t freegcopages[40];
    uintptr_t allgcopages;
    UpVal uvhead;
    uintptr_t sweepgcopage;
    uintptr_t mt[9];
    uintptr_t ttname[9];
    uintptr_t tmname[12];
    TValue pseudotemp;
    TValue registry;
    int registryfree;
    uint64_t ptrenckey[4];
    lua_Callbacks cb;
    uintptr_t errorjmp;
    uint64_t rngstate;
    uintptr_t ecb;
    uint8_t ecbdata[512];
    size_t memcatbytes[16];
    uintptr_t udatagc[256];
    uintptr_t udatamt[256];
    uintptr_t lightuserdataname[256];
    uint8_t gcstats[0xB8];
};

struct Lua {
    uintptr_t addr;

    Lua(uintptr_t a) : addr(a) {}

    lua_State* operator->() {
        static lua_State s;
        Memory::ReadNative(addr, &s, sizeof(lua_State), Lua::pid);
        return &s;
    }

    template<typename T>
    T* as() {
        static T obj;
        Memory::ReadNative(addr, &obj, sizeof(T), Lua::pid);
        return &obj;
    }

    template<typename T>
    T& ref() {
        static T obj;
        Memory::ReadNative(addr, &obj, sizeof(T), Lua::pid);
        return obj;
    }

    static DWORD pid;
};

DWORD Lua::pid = 0;

inline LuaTable* GetRegistry(uintptr_t luastateAddr, DWORD pid) {
    lua_State L;
    Memory::ReadNative(luastateAddr, &L, sizeof(lua_State), pid);
    
    global_State gs;
    Memory::ReadNative((uintptr_t)L.global, &gs, sizeof(global_State), pid);
    
    return (LuaTable*)gs.registry.value.p;
}

inline LuaTable* GetGlobalTable(uintptr_t luastateAddr, DWORD pid) {
    lua_State L;
    Memory::ReadNative(luastateAddr, &L, sizeof(lua_State), pid);
    return L.gt;
}

inline std::string ReadLuaString(uintptr_t strAddr, DWORD pid) {
    TString str;
    Memory::ReadNative(strAddr, &str, sizeof(TString), pid);
    if (str.len == 0 || str.len > 4096) return "";
    
    std::string result(str.len, '\0');
    Memory::ReadNative(strAddr + offsetof(TString, data), &result[0], str.len, pid);
    return result;
}

inline uintptr_t FindTableKey(LuaTable* tbl, const char* key, DWORD pid) {
    if (!tbl || !tbl->node) return 0;
    
    int lsizenode = tbl->lsizenode;
    int nodecount = 1 << lsizenode;
    
    TString keyStr;
    uint32_t hash = 0;
    const char* k = key;
    while (*k) {
        hash = ((hash << 5) + hash) + *k++;
    }
    
    LuaNode node;
    for (int i = 0; i < nodecount; i++) {
        Memory::ReadNative(tbl->node + i * sizeof(LuaNode), &node, sizeof(LuaNode), pid);
        
        if (node.key.value.p == nullptr && node.key.tt_ == LUA_TNIL) continue;
        
        if (node.key.tt_ == LUA_TSTRING) {
            uintptr_t keyStrAddr = (uintptr_t)node.key.value.p;
            TString nodeKey;
            Memory::ReadNative(keyStrAddr, &nodeKey, sizeof(TString), pid);
            
            if (nodeKey.hash == hash) {
                std::string nodeKeyStr(nodeKey.len, '\0');
                Memory::ReadNative(keyStrAddr + offsetof(TString, data), &nodeKeyStr[0], nodeKey.len, pid);
                if (nodeKeyStr == key) {
                    return tbl->node + i * sizeof(LuaNode);
                }
            }
        }
    }
    return 0;
}

inline LuaTable* GetTableAt(uintptr_t tblAddr, DWORD pid) {
    LuaTable* tbl = (LuaTable*)tblAddr;
    Memory::ReadNative(tblAddr, tbl, sizeof(LuaTable), pid);
    return tbl;
}

inline TValue* ReadTableValue(uintptr_t tblAddr, const char* key, DWORD pid) {
    static TValue result;
    memset(&result, 0, sizeof(TValue));
    
    LuaTable tbl;
    Memory::ReadNative(tblAddr, &tbl, sizeof(LuaTable), pid);
    
    uintptr_t nodeAddr = FindTableKey(&tbl, key, pid);
    if (!nodeAddr) return nullptr;
    
    Memory::ReadNative(nodeAddr + sizeof(TValue), &result, sizeof(TValue), pid);
    return &result;
}

inline LuaTable* ReadTableValueAsTable(uintptr_t tblAddr, const char* key, DWORD pid) {
    TValue* val = ReadTableValue(tblAddr, key, pid);
    if (!val || val->tt_ != LUA_TTABLE) return nullptr;
    return (LuaTable*)val->value.p;
}
