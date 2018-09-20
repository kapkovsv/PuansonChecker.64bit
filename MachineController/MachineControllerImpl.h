#pragma once

#include "MachineController.h"

#ifndef PRINTF
#define PRINTF ::MachineController::Noop
#endif

namespace MachineController {

#include "STLink.h"
#include "mach_controller.h"

	void Noop();
	template<class... T> void Noop(T... t) {
	}

	template<class F> struct OnLeave_ {
		F f;
		bool engaged;
		OnLeave_(const F& f, bool engage = true) : f(f), engaged(engage) {
		}
		OnLeave_(OnLeave_&& e) : f(e.f), engaged(e.engaged) {
			e.engaged = false;
		}
		OnLeave_(const OnLeave_&) = delete;
		OnLeave_& operator=(const OnLeave_&) = delete;
		OnLeave_& operator=(OnLeave_&&) = delete;
		void Engage() {
			engaged = true;
		}
		void Disengage() {
			engaged = false;
		}
		void operator()() {
			if ( engaged ) {
				f();
				engaged = false;
			}
		}
		~OnLeave_() {
			operator()();
		}
	};

	struct OnLeaveHelper {
		template<class F> OnLeave_<F> operator+(const F& f) {
			return OnLeave_<F>(f);
		}
	};

#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)
#define ON_LEAVE auto CONCAT(OnLeave_, __LINE__) = OnLeaveHelper() + [&]
#define ON_LEAVE2(f) auto CONCAT(OnLeave_, __LINE__) = OnLeave([&] { f });

	template<class F> OnLeave_<F> OnLeave(const F& f, bool engage = true) {
		return OnLeave_<F>(f, engage);
	}

	inline void CheckFileOpen(HANDLE hFile) {
		if ( hFile == INVALID_HANDLE_VALUE )
			throw FileOpenException();
	}
	inline void CheckFileRead(BOOL bResult, DWORD cbExpected, DWORD cbActual) {
		if ( !bResult )
			throw FileReadException();
		if ( cbActual != cbExpected )
			throw IncompleteTransfer(cbExpected, cbActual);
	}

	struct CodeDownloader {
		virtual void InitDownload(uint32_t CodeBase) = 0;
		virtual void DownloadBlock(uint32_t Address, uint8_t* pBlock) = 0;
		virtual void CompleteDownload(uint32_t Address, uint32_t EntryPoint) = 0;
	};

	void LoadElf(CodeDownloader* pDownloader, LPCSTR Path, uint32_t cbBlock);

	class DeviceEnumeratorImpl : public DeviceEnumerator {
	protected:
		const GUID& guid;
		HDEVINFO hDevInfo;
		int i;

	public:
		DeviceEnumeratorImpl(const GUID& guid);
		virtual ~DeviceEnumeratorImpl();
		virtual LPSTR GetNext();
	};

	class MachineControllerImpl : public MachineController, protected STLink, CodeDownloader {
	protected:
		enum { cbBlock = 0x200 };
        enum { ERAM = 0x20002000, ExchangeAddr = ERAM - sizeof(ExchangeData) };
		HANDLE hDevice;
		ExchangeData exchange;
		int ReferencePoint[2];

	private:
		void InitDownload(uint32_t CodeBase);
		void DownloadBlock(uint32_t Address, uint8_t* pBlock);
		void CompleteDownload(uint32_t Address, uint32_t EntryPoint);

	public:
        MachineControllerImpl() { }
		MachineControllerImpl(LPCSTR path);
		virtual ~MachineControllerImpl();
		virtual void FindReferencePos(uint8_t AxesMask);
		virtual void MoveTo(int x, int y);
		virtual void MoveBy(int x, int y);
		virtual void WaitCompletion();
		virtual void LoadElf(LPCSTR path);

		void ReadData() {
			ReadMemory32(ExchangeAddr, sizeof exchange, &exchange);
		}
		void WriteData() {
			WriteMemory32(ExchangeAddr, sizeof exchange, &exchange);
		}
		void ReadState(int iAxis) {
			ReadMemory32(ExchangeAddr + FIELD_OFFSET(ExchangeData, axis[iAxis].state), sizeof exchange.axis[iAxis].state, &exchange.axis[iAxis].state);
		}
		void WriteState(int iAxis) {
			WriteMemory32(ExchangeAddr + FIELD_OFFSET(ExchangeData, axis[iAxis].state), sizeof exchange.axis[iAxis].state, &exchange.axis[iAxis].state);
		}
	};

}
