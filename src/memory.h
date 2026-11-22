// Small helpers for patching the game's code and data in memory (32-bit).
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace Memory {
	// Writes `value` at `address`, temporarily making the page writable.
	template<typename T>
	inline void Patch(uintptr_t address, T value) {
		DWORD oldProtect, unused;
		VirtualProtect((void*)address, sizeof(value), PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy((void*)address, &value, sizeof(value));
		VirtualProtect((void*)address, sizeof(value), oldProtect, &unused);
	}

	// Fills `count` bytes at `address` with `value` (e.g. 0x90 = NOP).
	inline void Fill(uintptr_t address, uint8_t value, size_t count) {
		DWORD oldProtect, unused;
		VirtualProtect((void*)address, count, PAGE_EXECUTE_READWRITE, &oldProtect);
		memset((void*)address, value, count);
		VirtualProtect((void*)address, count, oldProtect, &unused);
	}

	enum Instruction : uint8_t {
		CALL = 0xE8,
		JMP  = 0xE9,
	};

	// Writes a 5-byte relative CALL/JMP at `src` that goes to `dest`.
	template<typename T>
	inline void PatchRelative(Instruction type, uintptr_t src, T dest) {
		Patch<uint8_t>(src, type);
		Patch<int32_t>(src + 1, (int32_t)((uintptr_t)dest - (src + 5)));
	}

	// Entry point of the main executable as an RVA (identifies the exe build).
	inline uintptr_t GetEntryPoint() {
		auto base = (uintptr_t)GetModuleHandleA(nullptr);
		auto dos = (IMAGE_DOS_HEADER*)base;
		auto nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
		return nt->OptionalHeader.AddressOfEntryPoint;
	}
}
