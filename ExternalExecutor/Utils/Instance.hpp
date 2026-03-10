#pragma once
#include <Windows.h>
#include <functional>

#include "Memory.hpp"
#include "../Update/Offsets.hpp"

class Instance {
private:
	uintptr_t address;
	DWORD ProcessID;
public:
	Instance(uintptr_t addr, DWORD PID) :
		address(addr),
		ProcessID(PID)
	{
	}

    uintptr_t GetAddress() {
        return address;
    }

    std::string Name() {
        std::uintptr_t nameaddr = ReadMemory<uintptr_t>(address + Offsets::Name, ProcessID);
        const auto size = ReadMemory<size_t>(nameaddr + 0x10, ProcessID);
        if (size >= 16)
            nameaddr = ReadMemory<uintptr_t>(nameaddr, ProcessID);
        std::string str;
        BYTE code = 0;
        for (std::int32_t i = 0; code = ReadMemory<uint8_t>(nameaddr + i, ProcessID); i++) {
            str.push_back(code);
        }
        return str;
    }

	Instance FindFirstChild(std::string name) {
        std::uintptr_t childrenPtr = ReadMemory<uintptr_t>(address + Offsets::Children, ProcessID);
        if (childrenPtr == 0)
            return Instance(0, ProcessID);
        std::uintptr_t childrenStart = ReadMemory<uintptr_t>(childrenPtr, ProcessID);
        std::uintptr_t childrenEnd = ReadMemory<uintptr_t>(childrenPtr + Offsets::ChildrenEnd, ProcessID);
        for (std::uintptr_t childAddress = childrenStart; childAddress < childrenEnd; childAddress += 0x10) {
            std::uintptr_t childPtr = ReadMemory<uintptr_t>(childAddress, ProcessID);
            if (childPtr != 0) {
                std::uintptr_t nameaddr = ReadMemory<uintptr_t>(childPtr + Offsets::Name, ProcessID);
                const auto size = ReadMemory<size_t>(nameaddr + 0x10, ProcessID);
                if (size >= 16)
                    nameaddr = ReadMemory<uintptr_t>(nameaddr, ProcessID);
                if (size != name.length())
                    continue;
                std::string str;
                BYTE code = 0;
                for (std::int32_t i = 0; code = ReadMemory<uint8_t>(nameaddr + i, ProcessID); i++) {
                    str.push_back(code);
                    if (str != name.substr(0, str.length()))
                        break;
                }
                if (str == name)
                    return Instance(childPtr, ProcessID);
            }
        }
        return Instance(0, ProcessID);
	}

    Instance WaitForChild(std::string name) {
        Instance child = FindFirstChild(name);
        while (child.GetAddress() == 0) {
            Sleep(5);
            child = FindFirstChild(name);
        }
        return child;
    }

    std::string ClassName() {
        std::uintptr_t classaddr = ReadMemory<uintptr_t>(address + Offsets::ClassDescriptor, ProcessID);
        std::uintptr_t nameaddr = ReadMemory<uintptr_t>(classaddr + Offsets::ClassDescriptorToClassName, ProcessID);
        const auto size = ReadMemory<size_t>(nameaddr + 0x10, ProcessID);
        if (size >= 16)
            nameaddr = ReadMemory<uintptr_t>(nameaddr, ProcessID);
        std::string str;
        BYTE code = 0;
        for (std::int32_t i = 0; code = ReadMemory<uint8_t>(nameaddr + i, ProcessID); i++) {
            str.push_back(code);
        }
        return str;
    }

    std::function<void()> SetScriptBytecode(const std::vector<char>& bytes, size_t size) {
        uintptr_t offset = (ClassName() == "LocalScript")
            ? Offsets::LocalScriptByteCode
            : Offsets::ModuleScriptByteCode;

        uintptr_t embedded = ReadMemory<uintptr_t>(address + offset, ProcessID);

        uintptr_t original_bytecode_ptr = ReadMemory<uintptr_t>(embedded + 0x10, ProcessID);
        uint64_t original_size = ReadMemory<uint64_t>(embedded + 0x20, ProcessID);

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
        void* newMem = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!newMem) {
            CloseHandle(hProcess);
            return []() {};
        }

        Memory::WriteNative((uintptr_t)newMem, bytes.data(), bytes.size(), ProcessID);
        WriteMemory<uintptr_t>(embedded + 0x10, reinterpret_cast<uintptr_t>(newMem), ProcessID);
        WriteMemory<uint64_t>(embedded + 0x20, static_cast<uint64_t>(size), ProcessID);

        CloseHandle(hProcess);

        return [=]() {
            WriteMemory<uintptr_t>(embedded + 0x10, original_bytecode_ptr, ProcessID);
            WriteMemory<uint64_t>(embedded + 0x20, original_size, ProcessID);
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
            VirtualFreeEx(hProcess, newMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            };
    }

    void a() {

    }
};

inline Instance FetchDatamodel(uintptr_t BaseModule, DWORD ProcessID) {
	uintptr_t Fake = ReadMemory<uintptr_t>(BaseModule + Offsets::FakeDataModelPointer, ProcessID);
	uintptr_t Real = ReadMemory<uintptr_t>(Fake + Offsets::FakeDataModelToDataModel, ProcessID);
	return Instance(Real, ProcessID);
}