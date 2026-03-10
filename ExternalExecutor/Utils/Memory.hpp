#pragma once
#include <Windows.h>
#include <cstdint>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

static constexpr size_t BATCH_SIZE = 4096;

typedef NTSTATUS(NTAPI* pNtReadVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef NTSTATUS(NTAPI* pNtFreeVirtualMemory)(HANDLE, PVOID*, PSIZE_T, ULONG);
typedef NTSTATUS(NTAPI* pNtQueryVirtualMemory)(HANDLE, PVOID, ULONG, PVOID, SIZE_T, PSIZE_T);

static pNtReadVirtualMemory fnNtRead = nullptr;
static pNtWriteVirtualMemory fnNtWrite = nullptr;
static pNtAllocateVirtualMemory fnNtAlloc = nullptr;
static pNtFreeVirtualMemory fnNtFree = nullptr;
static pNtQueryVirtualMemory fnNtQuery = nullptr;

static DWORD HashString(const char* str) {
    DWORD hash = 0;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

static PVOID GetProcByHash(HMODULE hModule, DWORD hash) {
    if (!hModule) return nullptr;

    auto dosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

    auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return nullptr;

    DWORD exportRva = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportRva) return nullptr;

    auto exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportRva);
    DWORD* nameRvas = (DWORD*)((BYTE*)hModule + exportDir->AddressOfNames);
    WORD* ordinals = (WORD*)((BYTE*)hModule + exportDir->AddressOfNameOrdinals);
    DWORD* funcRvas = (DWORD*)((BYTE*)hModule + exportDir->AddressOfFunctions);

    for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        char* funcName = (char*)((BYTE*)hModule + nameRvas[i]);
        if (HashString(funcName) == hash) {
            WORD ordinal = ordinals[i];
            DWORD funcRva = funcRvas[ordinal];
            return (PVOID)((BYTE*)hModule + funcRva);
        }
    }
    return nullptr;
}

static void InitializeNativeFunctions() {
    if (!fnNtRead) {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll) {
            fnNtRead = (pNtReadVirtualMemory)GetProcByHash(ntdll, HashString("NtReadVirtualMemory"));
            fnNtWrite = (pNtWriteVirtualMemory)GetProcByHash(ntdll, HashString("NtWriteVirtualMemory"));
            fnNtAlloc = (pNtAllocateVirtualMemory)GetProcByHash(ntdll, HashString("NtAllocateVirtualMemory"));
            fnNtFree = (pNtFreeVirtualMemory)GetProcByHash(ntdll, HashString("NtFreeVirtualMemory"));
            fnNtQuery = (pNtQueryVirtualMemory)GetProcByHash(ntdll, HashString("NtQueryVirtualMemory"));
        }
    }
}

namespace Memory {
    void ReadNative(uintptr_t address, void* buffer, size_t size, DWORD pid) {
        InitializeNativeFunctions();
        if (!fnNtRead) return;

        HANDLE Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        size_t offset = 0;
        char* dst = static_cast<char*>(buffer);

        while (offset < size) {
            size_t currentChunk = (BATCH_SIZE < (size - offset)) ? BATCH_SIZE : (size - offset);
            SIZE_T bytes = 0;
            NTSTATUS status = fnNtRead(Handle, (PVOID)(address + offset), dst + offset, currentChunk, &bytes);
            if (!NT_SUCCESS(status) || bytes != currentChunk) break;
            offset += currentChunk;
        }

        CloseHandle(Handle);
    }

    void WriteNative(uintptr_t address, const void* buffer, size_t size, DWORD pid) {
        InitializeNativeFunctions();
        if (!fnNtWrite) return;

        HANDLE Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        size_t offset = 0;
        const char* src = static_cast<const char*>(buffer);

        while (offset < size) {
            size_t currentChunk = (BATCH_SIZE < (size - offset)) ? BATCH_SIZE : (size - offset);
            SIZE_T bytes = 0;
            NTSTATUS status = fnNtWrite(Handle, (PVOID)(address + offset), (PVOID)(src + offset), currentChunk, &bytes);
            if (!NT_SUCCESS(status) || bytes != currentChunk) break;
            offset += currentChunk;
        }

        CloseHandle(Handle);
    }
}

template <class Ty>
inline Ty ReadMemory(uintptr_t address, DWORD pid) {
    Ty value{};
    Memory::ReadNative(address, &value, sizeof(Ty), pid);
    return value;
}

template <class Ty>
inline void WriteMemory(uintptr_t address, Ty value, DWORD pid) {
    Memory::WriteNative(address, &value, sizeof(Ty), pid);
}