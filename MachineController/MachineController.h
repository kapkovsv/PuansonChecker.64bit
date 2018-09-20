#pragma once

#include <cmath>

namespace MachineController {

	struct Exception {
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

        MachineController() { }
		virtual ~MachineController() { }
		virtual void FindReferencePos(uint8_t AxesMask) = 0;
		virtual void MoveTo(int x, int y) = 0;
		virtual void MoveBy(int x, int y) = 0;
		virtual void WaitCompletion() = 0;
		virtual void LoadElf(LPCSTR path) = 0;

		static double ScrewPitch() { return 5; }
		static int MillimetersToEncoderSteps(double x) {
            return (int)round(EncoderStepsPerRevolution / ScrewPitch() * x);
		}
		static int MillimetersToMotorSteps(double x) {
			return (int)round(MicrostepsPerRevolution / ScrewPitch() * x);
		}
		void MoveTo(double x, double y) {
			MoveTo(MillimetersToEncoderSteps(x), MillimetersToEncoderSteps(y));
		}
	};

	MachineController* CreateMachineController(LPSTR devicePath);
}
