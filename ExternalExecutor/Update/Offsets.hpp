#pragma once
#include <Windows.h>
#include "Utils/LuaState.hpp"

namespace Offsets {
	const uintptr_t EnableLoadModule = 0x7b61808;

	inline constexpr uintptr_t FakeDataModelPointer = 0x81d3ea8;
	inline constexpr uintptr_t FakeDataModelToDataModel = 0x1C0;

	inline constexpr uintptr_t Children = 0x70;
	inline constexpr uintptr_t ChildrenEnd = 0x8;
	inline constexpr uintptr_t Name = 0xB0;
	inline constexpr uintptr_t Value = 0xD0;

	inline constexpr uintptr_t ClassDescriptor = 0x18;
	inline constexpr uintptr_t ClassDescriptorToClassName = 0x8;

	inline constexpr uintptr_t LocalScriptByteCode = 0x1A8;
	inline constexpr uintptr_t ModuleScriptByteCode = 0x150;
    namespace BindableEvent {
        inline constexpr uintptr_t Signals = 0xc8;
        inline constexpr uintptr_t Function = 0x8;
        inline constexpr uintptr_t FunctionImpl = 0x30;
        inline constexpr uintptr_t ThreadRef = 0x60;
        inline constexpr uintptr_t LiveThread = 0x20;
        inline constexpr uintptr_t LuaState = 0x8;
    }
}

inline static uintptr_t identity_to_caps(uintptr_t i) {
    uintptr_t ret = 0;
    switch (i) {
    case 4: ret = 0x2000000000000003LL; break;
    case 3: ret = 0x200000000000000BLL; break;
    case 5: ret = 0x2000000000000001LL; break;
    case 6: ret = 0x600000000000000BLL; break;
    case 8: ret = 0xFFFFFFFFFFFFFFFF; break;
    case 9: ret = 12LL; break;
    case 10: ret = 0x6000000000000003LL; break;
    case 11: ret = 0x2000000000000000LL; break;
    default: ret = 0LL; break;
    }
    return ret | 0x3FFFFFF00LL;
};
