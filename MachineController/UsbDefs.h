
#pragma pack(push)
#pragma pack(1)

typedef struct {
	UCHAR  bmRequestType;
	UCHAR  bRequest;
	USHORT wValue;
	USHORT wIndex;
	USHORT wLength;
} UsbControlRequest;

typedef struct {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	USHORT bcdUSB;
	UCHAR  bDeviceClass;
	UCHAR  bDeviceSubClass;
	UCHAR  bDeviceProtocol;
	UCHAR  bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR  iManufacturer;
	UCHAR  iProduct;
	UCHAR  iSerialNumber;
	UCHAR  bNumConfigurations;
} UsbDeviceDescriptor;

typedef struct {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	USHORT wTotalLength;
	UCHAR  bNumInterfaces;
	UCHAR  bConfigurationValue;
	UCHAR  iConfiguration;
	UCHAR  bmAttributes; // 7 Bus Powered
						   // 6 Self Powered
						   // 5 Remote Wakeup
						   // 4..0 Reserved (reset to 0)
	UCHAR  bMaxPower;
} UsbConfigurationDescriptor;

typedef struct {
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bInterfaceNumber;
	UCHAR bAlternateSetting;
	UCHAR bNumEndpoints;
	UCHAR bInterfaceClass;
	UCHAR bInterfaceSubClass;
	UCHAR bInterfaceProtocol;
	UCHAR iInterface;
} UsbInterfaceDescriptor;

typedef struct {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	UCHAR  bEndpointAddress; // Bit 0..3 The endpoint number
							   // Bit 4..6 Reserved, reset to zero
							   // Bit 7 Direction (ignored for control endpoints):
							   //   0 - OUT endpoint
							   //   1 - IN endpoint
	UCHAR  bmAttributes; // Bits 1..0: Transfer Type
						   //   00 = Control
						   //   01 = Isochronous
						   //   10 = Bulk
						   //   11 = Interrupt
						   // For isochronous endpoint (reserved for other types):
						   // Bits 3..2: Synchronization Type
						   //   00 = No Synchronization
						   //   01 = Asynchronous
						   //   10 = Adaptive
						   //   11 = Synchronous
						   // Bits 5..4: Usage Type
						   //   00 = Data endpoint
						   //   01 = Feedback endpoint
						   //   10 = Implicit feedback Data endpoint
						   //   11 = Reserved
	USHORT wMaxPacketSize;
	UCHAR  bInterval;
} UsbEndpointDescriptor;

typedef struct {
	UCHAR  bDescriptorType;
	USHORT wDescriptorLength;
} UsbDescriptorListEntry;

typedef struct {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	USHORT bcdHID;
	UCHAR  bCountryCode;
	UCHAR  bNumDescriptors;
	UsbDescriptorListEntry Descriptors[0];
} UsbHidDescriptor;

enum UsbStandardRequests {
	UR_GET_STATUS, UR_CLEAR_FEATURE, UR_RESERVED2, UR_SET_FEATURE, UR_RESERVED4, UR_SET_ADDRESS, UR_GET_DESCRIPTOR,
	UR_SET_DESCRIPTOR, UR_GET_CONFIGURATION, UR_SET_CONFIGURATION, UR_GET_INTERFACE, UR_SET_INTERFACE, UR_SYNCH_FRAME
};

enum UsbHidRequests {
	UR_GET_REPORT = 1, UR_GET_IDLE, UR_GET_PROTOCOL,
	UR_SET_REPORT = 9, UR_SET_IDLE, UR_SET_PROTOCOL
};

enum UsbDescriptorTypes {
	UD_DEVICE = 1, UD_CONFIGURATION, UD_STRING, UD_INTERFACE, UD_ENDPOINT,
	UD_DEVICE_QUALIFIER, UD_OTHER_SPEED_CONFIGURATION, UD_INTERFACE_POWER,

	UD_HID = 0x21, UD_HID_REPORT,
};

enum UsbDfuRequests { DFU_DETACH, DFU_DNLOAD, DFU_UPLOAD, DFU_GETSTATUS, DFU_CLRSTATUS, DFU_GETSTATE, DFU_ABORT };

struct DfuStatus {
	ULONG bStatus: 8;
	ULONG bwPollTimeout: 24;
	UCHAR bState;
	UCHAR iString;
};

struct DfuFunctionalDescriptor {
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bmAttributes;
	USHORT wDetachTimeout;
	USHORT wTransferSize;
	USHORT bcdDFUVersion;
};

enum {
	appIDLE, appDETACH, dfuIDLE, dfuDNLOAD_SYNC, dfuDNBUSY, dfuDNLOAD_IDLE,
	dfuMANIFEST_SYNC, dfuMANIFEST, dfuMANIFEST_WAIT_RESET, dfuUPLOAD_IDLE, dfuERROR
};

enum {
	dfuOK, dfuErrTARGET, dfuErrFILE, dfuErrWRITE, dfuErrERASE, dfuErrCHECK_ERASED, dfuErrPROG, dfuErrVERIFY,
	dfuErrADDRESS, dfuErrNOTDONE, dfuErrFIRMWARE, dfuErrVENDOR, dfuErrUSBR, dfuErrPOR, dfuErrUNKNOWN, dfuErrSTALLEDPKT
};

#pragma pack(pop)
