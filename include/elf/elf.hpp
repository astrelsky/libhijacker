#pragma once

#include "dbg/dbg.hpp"
#include "hijacker/hijacker.hpp"
extern "C" {
	#include <elf.h>
	#include <sys/elf64.h>
	#include <ps5/payload_main.h>
}


#include "util.hpp"

class ManagedResolver;

constexpr auto i = sizeof(Elf64_Dyn);

class Elf : Elf64_Ehdr {

	public:
		struct MappedMemory {
			uintptr_t mem;
			size_t len;
		};

	private:
	dbg::Tracer tracer;
	const Elf64_Phdr *__restrict phdrs;
	const char *__restrict strtab;
	size_t strtabLength;
	const Elf64_Sym *__restrict symtab;
	size_t symtabLength;
	const Elf64_Rela *__restrict relatbl;
	size_t relaLength;
	const Elf64_Rela *__restrict plt;
	size_t pltLength;
	Hijacker *__restrict hijacker;
	size_t textOffset;
	size_t textIndex;
	uintptr_t imagebase;
	uint8_t *data;
	UniquePtr<ManagedResolver> resolver;
	Array<MappedMemory> mappedMemory;
	int jitFd;

	bool processProgramHeaders() noexcept;
	bool parseDynamicTable() noexcept;
	bool processRelocations() noexcept;
	bool processPltRelocations() noexcept;
	bool load() noexcept;
	bool start(uintptr_t args) noexcept;
	bool fillSymbolTables(const Array<String> &names, int handleCount, int *preLoadedHandles) noexcept;
	bool processLibs(List<const Elf64_Dyn *> &neededLibs) noexcept;
	uintptr_t toFileOffset(uintptr_t addr) const noexcept;
	uintptr_t toVirtualAddress(uintptr_t addr) const noexcept;
	uintptr_t setupKernelRW() noexcept;
	uintptr_t getSymbolAddress(const Elf64_Rela *__restrict rel) const noexcept;

	template<typename T>
	T faddr(uintptr_t addr) const noexcept {
		// this was just sheer laziness
		return reinterpret_cast<T>(data + toFileOffset(addr));
	}

	public:
		Elf(Hijacker *hijacker, uint8_t *data) noexcept;
		Elf(const Elf&) = delete;
		Elf &operator=(const Elf&) = delete;
		Elf(Elf&&) noexcept = default;
		Elf &operator=(Elf&&) noexcept = default;
		~Elf() noexcept; // external linkage to prevent undefined behavior

		bool launch() noexcept;
		explicit operator bool() const noexcept {
			return static_cast<bool>(tracer);
		}
};
