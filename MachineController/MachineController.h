#pragma once

namespace MachineControllerSpace {
#include "McuRegs.h"

	struct Exception {
		virtual ~Exception() { }
	};
	struct InvalidParameter : Exception {
	};
	struct Win32Error : Exception {
		DWORD ErrorCode;
		Win32Error(DWORD ErrorCode = GetLastError()) : ErrorCode(ErrorCode) {
		}
	};
	struct IncompleteTransfer : Exception {
		DWORD cbExpected;
		DWORD cbActual;
		IncompleteTransfer(DWORD cbExpected, DWORD cbActual) :
			cbExpected(cbExpected),
			cbActual(cbActual)
		{
		}
	};
	struct FileOpenException : Win32Error {
		using Win32Error::Win32Error;
	};
	struct FileReadException : Win32Error {
		using Win32Error::Win32Error;
	};
	struct WinUsbException : Win32Error {
		using Win32Error::Win32Error;
	};
	struct EnumerationException : Win32Error {
		using Win32Error::Win32Error;
	};
	struct ElfFormatException : Exception {
	};
	struct HardwareException : Exception {
	};
	struct StlinkStateError : HardwareException {
	};
	struct StlinkWriteError : HardwareException {
	};
	struct McuStateException : HardwareException {
		uint32_t dhcsr;
		uint32_t dfsr;
		uint32_t icsr;
		uint32_t PcSample;
		std::unique_ptr<uint32_t[]> Regs;
		std::unique_ptr<uint32_t[]> Stack;
		McuStateException(uint32_t dhcsr, uint32_t dfsr, uint32_t icsr, uint32_t PcSample, std::unique_ptr<uint32_t[]>&& Regs, std::unique_ptr<uint32_t[]>&& Stack) :
			dhcsr(dhcsr),
			dfsr(dfsr),
			icsr(icsr),
			PcSample(PcSample),
			Regs(std::move(Regs)),
			Stack(std::move(Stack))
		{
		}
		bool WasReset() {
			return (dhcsr & DHCSR_S_RESET_ST) != 0;
		}
	};

	extern const GUID guidSTLink;

	class DeviceEnumerator {
	public:
		virtual ~DeviceEnumerator() { }
		virtual LPSTR GetNext() = 0;
	};

	DeviceEnumerator* CreateDeviceEnumerator(const GUID& guid);

	class MachineController {
	public:
		enum { MicrostepsPerRevolution = 200 * 16 };
		enum { EncoderStepsPerRevolution = 4000 };

		virtual ~MachineController() { }
		virtual void FindReferencePos(uint8_t AxesMask) = 0;
		virtual void MoveTo(int x, int y) = 0;
		virtual void MoveBy(int x, int y) = 0;
		virtual void WaitCompletion() = 0;
		virtual void LoadElf(LPCSTR path) = 0;

		static double ScrewPitch() { return 5; }
		static int MillimetersToEncoderSteps(double x) {
            return (int)std::round(EncoderStepsPerRevolution / ScrewPitch() * x);
		}
		static int MillimetersToMotorSteps(double x) {
            return (int)std::round(MicrostepsPerRevolution / ScrewPitch() * x);
		}
		void MoveTo(double x, double y) {
			MoveTo(MillimetersToEncoderSteps(x), MillimetersToEncoderSteps(y));
		}
	};

	MachineController* CreateMachineController(LPSTR devicePath);

}
