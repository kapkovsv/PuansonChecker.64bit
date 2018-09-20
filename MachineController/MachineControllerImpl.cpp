#include "StdAfx.h"
#include "MachineControllerImpl.h"

namespace MachineController {

	typedef uint32_t u_int32_t;
	typedef uint16_t u_int16_t;
	typedef uint8_t  u_int8_t;

#include "sys\elf32.h"

	// {DBCE1CD9-A320-4B51-A365-A0C3F3C5FB29}
	const GUID guidSTLink = { 0xDBCE1CD9, 0xA320, 0x4B51, { 0xA3, 0x65, 0xA0, 0xC3, 0xF3, 0xC5, 0xFB, 0x29 } };

    void LoadElf(CodeDownloader* pDownloader, LPCSTR Path, uint32_t cbBlock) {
		enum { cbHeaders = 0x200 };
        const bool bOptimizeSectionRead = true;
		//static_assert((cbBlock & cbBlock - 1) == 0, "Must be power of 2");
		HANDLE hFile = CreateFile(Path, FILE_READ_ACCESS, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        CheckFileOpen(hFile);
        BOOL bResult;
		auto OnLeave1 = OnLeave([&] { bResult = CloseHandle(hFile); });
		ULONG cbTransferred;
        uint8_t Headers[cbHeaders];
		bResult = ReadFile(hFile, Headers, cbHeaders, &cbTransferred, 0);
		CheckFileRead(bResult, cbHeaders, cbTransferred);
		Elf32_Ehdr* pEhdr = (Elf32_Ehdr*)Headers;
		static_assert(sizeof *pEhdr <= sizeof Headers, "Insufficient buffer size");
		Elf32_Phdr* pPhdr = (Elf32_Phdr*)(Headers + pEhdr->e_phoff);
        Elf32_Addr paddrPrev = 0;
		auto cProgHeaders = pEhdr->e_phnum;
		Elf32_Size ProgHeaderEndOffset = pEhdr->e_phoff + pEhdr->e_phentsize * cProgHeaders;
		if ( ProgHeaderEndOffset > sizeof Headers )
			throw ElfFormatException();
        uint32_t CodeBase = (uint32_t)-1;
		std::vector<Elf32_Phdr*> ProgHeaders;
        ProgHeaders.reserve(cProgHeaders);
		//DbgPrint("%X-%X %X(%X) %d\n", pEhdr->e_phoff, ProgHeaderEndOffset, pEhdr->e_phentsize, sizeof *pPhdr, pEhdr->e_phnum);
		//DbgPrint("\nT FiOfs VirtAddr PhisAddr FiSz MeSz F Algn\n");
		for ( int i = 0; i < cProgHeaders; ++i, (uint8_t*&)pPhdr += pEhdr->e_phentsize ) {
			//DbgPrint("%d %05X %08X %08X %04X %04X %X %4X\n", pPhdr->p_type, pPhdr->p_offset, pPhdr->p_vaddr, pPhdr->p_paddr, pPhdr->p_filesz, pPhdr->p_memsz, pPhdr->p_flags, pPhdr->p_align);
            ProgHeaders.push_back(pPhdr);
			if ( paddrPrev > pPhdr->p_paddr ) {
				//throw ElfFormatException();
			}
			if ( pPhdr->p_filesz > 0 && pPhdr->p_paddr < CodeBase )
				CodeBase = pPhdr->p_paddr;
			paddrPrev = pPhdr->p_paddr + pPhdr->p_memsz;
        }
		//DbgPrint("\n");
        sort(ProgHeaders.begin(), ProgHeaders.end(), [](Elf32_Phdr* x, Elf32_Phdr* y) { return x->p_paddr < y->p_paddr; });
        if ( CodeBase == (uint32_t)-1 )
            throw ElfFormatException();
        pDownloader->InitDownload(CodeBase);
		//pPhdr = (Elf32_Phdr*)(Headers + pEhdr->e_phoff);
		pPhdr = ProgHeaders.front();
		uint8_t* pBuf = new uint8_t[cbBlock];
		ON_LEAVE{ delete[] pBuf; };
        Elf32_Addr bufPhysAddr = 0;
		auto Write = [&] { pDownloader->DownloadBlock(bufPhysAddr, pBuf); };
        Elf32_Size cBufBytes = 0;
		for ( int i = 0; i < cProgHeaders; ) {
			if ( pPhdr->p_filesz > 0 && pPhdr->p_type == PT_LOAD && (pPhdr->p_flags & 8) == 0 ) {
				if ( cBufBytes > 0 && pPhdr->p_paddr - bufPhysAddr >= cbBlock ) {
					memset(pBuf + cBufBytes, 0, cbBlock - cBufBytes);
					Write();
					cBufBytes = 0;
				}
				bufPhysAddr = CodeBase + ((pPhdr->p_paddr - CodeBase) & (0 - cbBlock));
				Elf32_Size bufOffset = pPhdr->p_paddr - bufPhysAddr;
				memset(pBuf + cBufBytes, 0, bufOffset - cBufBytes);
				uint32_t physFileDelta = pPhdr->p_paddr - pPhdr->p_offset;
				Elf32_Size cRemBytes;
				for ( ; ; ) {
					cRemBytes = pPhdr->p_paddr - bufPhysAddr + pPhdr->p_filesz;
					//(uint8_t*&)pPhdr += pEhdr->e_phentsize;
					pPhdr = ++i < cProgHeaders ? ProgHeaders[i] : 0;
					if ( /*++*/i >= cProgHeaders || !bOptimizeSectionRead )
						break;
					if ( (pPhdr->p_paddr & (0 - cbBlock)) > bufPhysAddr + (cRemBytes & (0 - cbBlock)) )
						break;
					if ( !(pPhdr->p_filesz > 0 && pPhdr->p_type == PT_LOAD && (pPhdr->p_flags & 8) == 0) )
						break;
					if ( pPhdr->p_paddr - pPhdr->p_offset != physFileDelta )
						break;
				}
				cBufBytes = bufOffset;
				while ( cRemBytes > bufOffset ) {
					cBufBytes = cRemBytes > cbBlock ? cbBlock : cRemBytes;
					OVERLAPPED Ovp;
					Ovp.hEvent = 0;
					Ovp.OffsetHigh = 0;
					Ovp.Offset = bufPhysAddr - physFileDelta + bufOffset;
					bResult = ReadFile(hFile, pBuf + bufOffset, cBufBytes - bufOffset, &cbTransferred, &Ovp);
					CheckFileRead(bResult, cBufBytes - bufOffset, cbTransferred);
					cRemBytes -= cBufBytes;
					if ( cBufBytes == cbBlock ) {
						Write();
						bufPhysAddr += cbBlock;
						cBufBytes = 0;
					}
					bufOffset = 0;
				}
			}
			else {
				//++i, (uint8_t*&)pPhdr += pEhdr->e_phentsize;
				pPhdr = ++i < cProgHeaders ? ProgHeaders[i] : 0;
			}
        }
		if ( cBufBytes > 0 ) {
			memset(pBuf + cBufBytes, 0, cbBlock - cBufBytes);
			Write();
			bufPhysAddr += cbBlock;
        }
		OnLeave1();
        pDownloader->CompleteDownload(bufPhysAddr, pEhdr->e_entry);
	}

	void Noop() {
	}

	DeviceEnumeratorImpl::DeviceEnumeratorImpl(const GUID& guid) :
		guid(guid)
	{
		hDevInfo = SetupDiGetClassDevs(&guid, 0, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT/* | DIGCF_ALLCLASSES*/);
		if ( !hDevInfo )
			throw EnumerationException();
	}

	DeviceEnumeratorImpl::~DeviceEnumeratorImpl() {
		BOOL b = SetupDiDestroyDeviceInfoList(hDevInfo);
	}

	LPSTR DeviceEnumeratorImpl::GetNext() {
		SP_DEVICE_INTERFACE_DATA InterfaceData;
		InterfaceData.cbSize = sizeof InterfaceData;
		BOOL b = SetupDiEnumDeviceInterfaces(hDevInfo, 0, &guid, i, &InterfaceData);
		++i;
		if ( !b ) {
			DWORD err = GetLastError();
			if ( err == ERROR_NO_MORE_ITEMS )
				return 0;
			throw EnumerationException(err);
		}
		union {
			SP_DEVICE_INTERFACE_DETAIL_DATA InterfaceDetail;
			char InterfaceDetailPlace[0x100];
		};
		InterfaceDetail.cbSize = sizeof InterfaceDetail;
		DWORD cbRequired;
		b = SetupDiGetDeviceInterfaceDetail(hDevInfo, &InterfaceData, &InterfaceDetail, sizeof InterfaceDetailPlace, &cbRequired, 0);
		return _strdup(InterfaceDetail.DevicePath);
	}

	DeviceEnumerator* CreateDeviceEnumerator(const GUID& guid) {
		return new DeviceEnumeratorImpl(guid);
	}

	MachineControllerImpl::MachineControllerImpl(LPCSTR path) {
		hDevice = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if ( hDevice == INVALID_HANDLE_VALUE )
			throw FileOpenException();
		BOOL b;
		auto OnLeave1 = OnLeave([&] { b = CloseHandle(hDevice); });
		b = WinUsb_Initialize(hDevice, &hUsbIface);
		if ( !b )
			throw WinUsbException();
		auto OnLeave2 = OnLeave([&] { b = WinUsb_Free(hUsbIface); });
		ULONG TransferTimeout = 3000;
		b = WinUsb_SetPipePolicy(hUsbIface, 0x02, PIPE_TRANSFER_TIMEOUT, sizeof TransferTimeout, &TransferTimeout);
		b = WinUsb_SetPipePolicy(hUsbIface, 0x81, PIPE_TRANSFER_TIMEOUT, sizeof TransferTimeout, &TransferTimeout);
		UCHAR AllowPartialReads = 0;
		b = WinUsb_SetPipePolicy(hUsbIface, 0x81, ALLOW_PARTIAL_READS, sizeof AllowPartialReads, &AllowPartialReads);
		ReferencePoint[1] = ReferencePoint[0] = 0;
		OnLeave2.Disengage();
		OnLeave1.Disengage();
	}
	MachineControllerImpl::~MachineControllerImpl() {
		BOOL b;
		b = WinUsb_Free(hUsbIface);
		b = CloseHandle(hDevice);
	}
	void MachineControllerImpl::InitDownload(uint32_t CodeBase) {
		uint16_t Status;
		uint16_t Mode1;
		GetCurrentMode(Mode1);
		if ( (Mode1 & 0xFF) == 0 )
			ExitDFUMode();
		EnterSWDDebugMode();
		Status = DriveNrst(0);
		//Status = ResetSys();
		Status = WriteDebug32(0xE000EDFC, 0x01000401); // DEMCR = DWTENA | VC_HARDERR | VC_CORERESET
		Status = DriveNrst(1);
	}
	void MachineControllerImpl::DownloadBlock(uint32_t Address, uint8_t* pBlock) {
		WriteMemory32(Address, cbBlock, pBlock);
		uint16_t Status = GetStatus();
		if ( Status != 0x80 )
			Noop();
	}
	void MachineControllerImpl::CompleteDownload(uint32_t Address, uint32_t EntryPoint) {
		uint16_t Status = GetStatus();
		uint32_t Buf[0x1C0];
		ReadMemory32(0x20000400, sizeof Buf, Buf);
		uint32_t Regs[21];
		ReadAllRegs(Regs);
		Status = WriteReg(15, EntryPoint);

		WriteData();
		Status = Run();
	}
	void MachineControllerImpl::FindReferencePos(uint8_t AxesMask) {
		for ( int i = 0; i < 2; ++i ) {
			if ( AxesMask & 1 << i ) {
				exchange.axis[i].targetPos = exchange.axis[i].pos;
				exchange.axis[i].cSteps = 200000;
				exchange.axis[i].findReference = 1;
				exchange.axis[i].state = 1;
			}
		}
		WriteData();
		WaitCompletion();
		for ( int i = 0; i < 2; ++i ) {
			if ( AxesMask & 1 << i ) {
				ReferencePoint[i] = exchange.axis[i].pos;
				exchange.axis[i].findReference = 0;
			}
		}
	}
	void MachineControllerImpl::MoveTo(int x, int y) {
		exchange.axis[0].targetPos = ReferencePoint[0] + x;
		exchange.axis[1].targetPos = ReferencePoint[1] + y;
		exchange.axis[0].cSteps = 1 << 31;
		exchange.axis[1].cSteps = 1 << 31;
		exchange.axis[0].state = 1;
		exchange.axis[1].state = 1;
		WriteData();
	}
	void MachineControllerImpl::MoveBy(int x, int y) {
		exchange.axis[0].targetPos = exchange.axis[0].pos;
		exchange.axis[1].targetPos = exchange.axis[1].pos;
		exchange.axis[0].cSteps = x;
		exchange.axis[1].cSteps = y;
		exchange.axis[0].state = 1;
		exchange.axis[1].state = 1;
		WriteData();
	}
	void MachineControllerImpl::WaitCompletion() {
		do {
			ReadData();
			PRINTF("%8d %8d  \r", exchange.axis[0].pos, exchange.axis[1].pos);
		} while ( exchange.axis[0].state != 0 || exchange.axis[1].state != 0 );
		PRINTF("%8d %8d %5d   %8d %8d %5d\r\n", exchange.axis[0].pos, exchange.axis[0].d, exchange.axis[0].minSlack, exchange.axis[1].pos, exchange.axis[1].d, exchange.axis[1].minSlack);
	}
    void MachineControllerImpl::LoadElf(LPCSTR path) {
		::MachineController::LoadElf(this, path, cbBlock);
	}

	MachineController* CreateMachineController(LPSTR devicePath) {
		return new MachineControllerImpl(devicePath);
	}

}
