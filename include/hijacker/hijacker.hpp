#pragma once

#include "dbg.hpp"
#include "hijacker/allocator.hpp"
#include "memory.hpp"
#include "kernel.hpp"
#include "kernel/rtld.hpp"
#include "util.hpp"
#include "allocator.hpp"
#include <sys/_stdint.h>

class Hijacker {

	UniquePtr<SharedObject> obj;

	protected:
		friend class Spawner;
		ProcessMemoryAllocator textAllocator;
		ProcessMemoryAllocator dataAllocator;

	private:
		mutable UniquePtr<SharedLib> libkernel;
	protected:
		uintptr_t pSavedRsp = 0;
	private:
		int mainThreadId = -1;
		bool isMainThreadRunning = true;

		Hijacker(SharedObject *obj) : obj(obj), textAllocator(nullptr), dataAllocator(nullptr), libkernel(nullptr) {
			auto eboot = this->obj->getEboot();
			while (textAllocator == nullptr) {
				textAllocator = ProcessMemoryAllocator(eboot->getTextSection());
			}
			while (dataAllocator == nullptr) {
				dataAllocator = ProcessMemoryAllocator(eboot->getDataSection());
			}
		}

		RtldMeta *getLibKernelMetaData() const {
			auto *meta = getLibKernel();
			return meta ? meta->getMetaData() : nullptr;
		}

		uintptr_t getLibKernelBase() {
			RtldMeta *meta = getLibKernelMetaData();
			return meta ? meta->imageBase : 0;
		}

		int getMainThreadId();

	public:
		static UniquePtr<Hijacker> getHijacker(const StringView &processName);
		static UniquePtr<Hijacker> getHijacker(int pid) {
			auto p = ::getProc(pid);
			auto obj = p->getSharedObject();

			// obj may be a nullptr when racing process creation
			return obj ? new Hijacker{obj.release()} : nullptr;
		}

		UniquePtr<KProc> getProc() const {
			return ::getProc(getPid());
		}

		void suspend() {
			if (isMainThreadRunning) {
				dbg::suspend(obj->pid);
				isMainThreadRunning = false;
			}
		}

		void resume() {
			if (!isMainThreadRunning) {
				dbg::resume(obj->pid);
				isMainThreadRunning = true;
			}
		}

		int getPid() const {
			return obj->pid;
		}

		SharedLib *getEboot() const {
			return obj->getEboot();
		}

		uintptr_t imagebase() const {
			auto eboot = obj->getEboot();
			return eboot ? eboot->imagebase() : 0;
		}

		SharedLib *getLibKernel() const {
			if (libkernel == nullptr) [[unlikely]] {
				libkernel = obj->getLib(0x2001);
			}
			return libkernel.get();
		}

		UniquePtr<SharedLib> getLib(int handle) const {
			return obj->getLib(handle);
		}

		UniquePtr<SharedLib> getLib(const StringView &name) const {
			return obj->getLib(name);
		}

		SharedLibIterator getLibs() const {
			return obj->getLibs();
		}

		UniquePtr<TrapFrame> getTrapFrame();
		void jailbreak() const;
		uintptr_t getFunctionAddress(SharedLib *lib, const Nid &fname) const;
		uintptr_t getLibKernelFunctionAddress(const Nid &fname) const {
			return getFunctionAddress(getLibKernel(), fname);
		}

		ProcessMemoryAllocator &getDataAllocator() {
			return dataAllocator;
		}

		ProcessMemoryAllocator &getTextAllocator() {
			return textAllocator;
		}

		UniquePtr<uint8_t[]> read(uintptr_t vaddr, size_t size) {
			return dbg::read(getPid(), vaddr, size).release();
		}

		void read(uintptr_t vaddr, void *buf, size_t size) {
			dbg::read(getPid(), vaddr, buf, size);
		}

		template <typename T>
		T read(uintptr_t vaddr) {
			T t;
			read(vaddr, &t, sizeof(t));
			return t;
		}

		template <size_t size>
		bool write(uintptr_t vaddr, const uint8_t(&buf)[size]) {
			return dbg::write(getPid(), vaddr, buf, size);
		}

		bool write(uintptr_t vaddr, const void *buf, size_t size) {
			return dbg::write(getPid(), vaddr, buf, size);
		}

		template <typename T>
		ProcessPointer<T> getPointer(uintptr_t addr) const {
			return {getPid(), addr};
		}

		uintptr_t getSavedRsp() const {
			return *getPointer<uintptr_t>(pSavedRsp);
		}
};
