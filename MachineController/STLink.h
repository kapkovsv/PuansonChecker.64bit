#pragma once
#include "STLinkConsts.h"
#include "UsbDefs.h"

struct STLink {
	WINUSB_INTERFACE_HANDLE hUsbIface;
	void ReadDescriptors() {
		UsbDeviceDescriptor DevDescr;
		ULONG cbTransferred;
		BOOL bResult = WinUsb_GetDescriptor(hUsbIface, UD_DEVICE, 0, 0, (PUCHAR)&DevDescr, sizeof DevDescr, &cbTransferred);
		union {
			UCHAR Buf[0x100];
            struct s {
				UsbConfigurationDescriptor ConfDescr;
				UsbInterfaceDescriptor IfaceDescr;
				UsbEndpointDescriptor EpDescrs[3];
			};
		};
		//bResult = WinUsb_GetDescriptor(hUsbIface, UD_CONFIGURATION, 0, 0, Buf, sizeof Buf, &cbTransferred);
		WINUSB_SETUP_PACKET SetupPacket1 = { 0x80, UR_GET_DESCRIPTOR, UD_CONFIGURATION << 8, 0, sizeof Buf };
		bResult = WinUsb_ControlTransfer(hUsbIface, SetupPacket1, Buf, sizeof Buf, &cbTransferred, 0);
		UCHAR DevSpeed;
		ULONG cbBuf = sizeof DevSpeed;
		bResult = WinUsb_QueryDeviceInformation(hUsbIface, DEVICE_SPEED, &cbBuf, &DevSpeed);
		USB_INTERFACE_DESCRIPTOR IfaceDescr1;
		bResult = WinUsb_QueryInterfaceSettings(hUsbIface, 0, &IfaceDescr1);
		//WINUSB_SETUP_PACKET SetupPacket2 = { 0x00, UR_SET_CONFIGURATION, 1, 0, 0 };
		//bResult = WinUsb_ControlTransfer(hUsbIface, SetupPacket2, 0, 0, &cbTransferred, 0);
	}
	void UsbSend(const void* pRequest, uint16_t cbRequest) {
		ULONG cbSent;
		BOOL bResult = WinUsb_WritePipe(hUsbIface, 0x02, (PUCHAR)pRequest, cbRequest, &cbSent, 0);
		if ( !bResult ) throw Win32Error();
		if ( cbSent != cbRequest )
			throw IncompleteTransfer(cbRequest, cbSent);
	}
	void UsbReceive(void* pResponse, uint16_t cbResponse) {
		ULONG cbReceived;
		BOOL bResult = WinUsb_ReadPipe(hUsbIface, 0x81, (PUCHAR)pResponse, cbResponse, &cbReceived, 0);
		if ( !bResult ) throw Win32Error();
		if ( cbReceived != cbResponse )
			throw IncompleteTransfer(cbResponse, cbReceived);
	}
	void UsbSendReceive(void* pRequest, uint16_t cbRequest, void* pResponse, uint16_t cbResponse) {
		UsbSend(pRequest, cbRequest);
		UsbReceive(pResponse, cbResponse);
	}
	void GetVersion(uint16_t(&Version)[3]) {
		uint8_t cmd[0x10] = { STLINK_GET_VERSION };
		UsbSendReceive(cmd, sizeof cmd, Version, sizeof Version);
	}
	void GetCurrentMode(uint16_t& Mode) {
		uint8_t cmd[0x10] = { STLINK_GET_CURRENT_MODE };
		UsbSendReceive(cmd, sizeof cmd, &Mode, sizeof Mode);
	}
	uint16_t GetStatus() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_GETSTATUS };
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	void ExitDFUMode() {
		uint8_t cmd[0x10] = { STLINK_DFU_COMMAND, STLINK_DFU_EXIT };
		UsbSend(cmd, sizeof cmd);
	}
	void GetDfuData(uint32_t(&DfuData)[5]) {
		uint8_t cmd[0x10] = { STLINK_DFU_COMMAND, 0x08 };
		UsbSendReceive(cmd, sizeof cmd, DfuData, sizeof DfuData);
	}
	void GetDfuStatus(uint16_t(&DfuStatus)[3]) {
		uint8_t cmd[0x10] = { STLINK_DFU_COMMAND, 0x03 };
		UsbSendReceive(cmd, sizeof cmd, DfuStatus, sizeof DfuStatus);
	}
	void DfuPrepareBlockWrite(uint32_t Address, uint8_t SubCommand) {
		uint8_t data[5];
		data[0] = SubCommand;
        (uint32_t&)data[1] = Address;
		uint16_t checksum = 0;
		for ( int i = 0; i < sizeof data; ++i )
			checksum += data[i];
		uint8_t cmd[0x10] = { STLINK_DFU_COMMAND, 0x01 };
		(uint16_t&)cmd[4] = checksum;
		(uint16_t&)cmd[6] = sizeof data;
		UsbSend(cmd, sizeof cmd);
		UsbSend(data, sizeof data);
	}
	void DfuWriteBlock(uint8_t* pData, uint16_t cbData, uint16_t Checksum, uint8_t iBlock) {
		uint8_t cmd[0x10] = { STLINK_DFU_COMMAND, 0x01, (uint8_t)(iBlock + 2) };
		(uint16_t&)cmd[4] = Checksum;
		(uint16_t&)cmd[6] = cbData;
		UsbSend(cmd, sizeof cmd);
		UsbSend(pData, cbData);
	}
	void ReadSTLinkMemory32(uint32_t Addr, uint16_t cbData, void* pData) {
		uint8_t cmd[0x10] = { 0xFD, 0x01 };
		(uint32_t&)cmd[2] = Addr;
		(uint16_t&)cmd[6] = cbData;
		UsbSendReceive(cmd, sizeof cmd, pData, cbData);
	}
	void WriteSTLinkMemory32(uint32_t Addr, uint16_t cbData, void* pData) {
		uint8_t cmd[0x10] = { 0xFD, 0x02 };
		(uint32_t&)cmd[2] = Addr;
		(uint16_t&)cmd[6] = cbData;
		UsbSend(cmd, sizeof cmd);
		UsbSend(pData, cbData);
	}
	void STLinkExecuteCode(uint32_t Addr) {
		uint8_t cmd[0x10] = { 0xFD, 0x03 };
		(uint32_t&)cmd[2] = Addr;
		UsbSend(cmd, sizeof cmd);
	}
	void EnterSWDDebugMode() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_ENTER, STLINK_DEBUG_ENTER_SWD };
		UsbSend(cmd, sizeof cmd);
	}
	void ExitDebugMode() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_EXIT };
		UsbSend(cmd, sizeof cmd);
	}
	uint16_t ResetSys() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_RESETSYS };
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	uint16_t DriveNrst(uint8_t Value) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_JTAG_DRIVE_NRST, Value };
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	uint16_t ForceDebug() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_FORCEDEBUG };
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	uint16_t Run() {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_RUNCORE };
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	void ReadCoreID(uint32_t& CoreId) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_READCOREID };
		UsbSendReceive(cmd, sizeof cmd, &CoreId, sizeof CoreId);
	}
	void ReadMemory32(uint32_t Addr, uint16_t cbData, void* pData) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_READMEM_32BIT };
		(uint32_t&)cmd[2] = Addr;
		(uint16_t&)cmd[6] = cbData;
		UsbSendReceive(cmd, sizeof cmd, pData, cbData);
	}
	void WriteMemory32(uint32_t Addr, uint16_t cbData, const void* pData) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_WRITEMEM_32BIT };
		(uint32_t&)cmd[2] = Addr;
		(uint16_t&)cmd[6] = cbData;
		UsbSend(cmd, sizeof cmd);
		UsbSend(pData, cbData);
	}
	uint16_t ReadDebug32(uint32_t Addr, uint32_t& Data) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_JTAG_READDEBUG_32BIT };
		(uint32_t&)cmd[2] = Addr;
		struct {
			uint16_t Status;
			uint16_t Reserved;
			uint32_t Data;
		} Response;
		UsbSendReceive(cmd, sizeof cmd, &Response, sizeof Response);
		Data = Response.Data;
		return Response.Status;
	}
	uint16_t WriteDebug32(uint32_t Addr, uint32_t Data) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_JTAG_WRITEDEBUG_32BIT };
		(uint32_t&)cmd[2] = Addr;
		(uint32_t&)cmd[6] = Data;
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
	void ReadAllRegs(uint32_t(&Values)[21]) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_READALLREGS };
		UsbSendReceive(cmd, sizeof cmd, Values, sizeof Values);
	}
	void ReadReg(uint8_t Index, uint32_t& Value) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_READREG, Index };
		UsbSendReceive(cmd, sizeof cmd, &Value, sizeof Value);
	}
	uint16_t WriteReg(uint8_t Index, uint32_t Value) {
		uint8_t cmd[0x10] = { STLINK_DEBUG_COMMAND, STLINK_DEBUG_WRITEREG, Index };
		(uint32_t&)cmd[3] = Value;
		uint16_t Status;
		UsbSendReceive(cmd, sizeof cmd, &Status, sizeof Status);
		return Status;
	}
};
