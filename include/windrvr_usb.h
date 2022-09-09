/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*  This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
   v2 for more details.
 * You should have received a copy of the GNU General Public License
   along with this program. If not, contact Jungo Connectivity Ltd. at
   support@jungo.com */

/*  Alternately, if compiling for Microsoft Windows, this file is licensed
  under the WinDriver commercial license provided with the Software. */

#if !defined(_WINDRVR_USB_H_)
#define _WINDRVR_USB_H_

/* Use it to pad struct size to 64 bit, when using 32 on 64 bit application */
#ifndef PAD_TO_64
#if defined (i386) && defined(KERNEL_64BIT)
#define PAD_TO_64(pName) DWORD dwPad_##pName;
#else
#define PAD_TO_64(pName)
#endif
#endif

#if defined (i386) && defined(KERNEL_64BIT)
#define PAD_TO_64_PTR_ARR(pName,size) PVOID ptPad_##pName[size];
#else
#define PAD_TO_64_PTR_ARR(pName, size)
#endif

#if defined(LINUX)
    #if !defined(__P_TYPES__)
        #define __P_TYPES__
        #include <wd_types.h>
        typedef void VOID;
        typedef unsigned char UCHAR;
        typedef unsigned short USHORT;
        typedef unsigned int UINT;
        typedef unsigned long ULONG;
        typedef u32 BOOL;
        typedef void *PVOID;
        typedef unsigned char *PBYTE;
        typedef char CHAR;
        typedef char *PCHAR;
        typedef unsigned short *PWORD;
        typedef u32 DWORD, *PDWORD;
        typedef int PRCHANDLE;
        typedef PVOID HANDLE;
        typedef long LONG;
    #endif
    #if !defined(TRUE)
        #define TRUE 1
    #endif
    #if !defined(FALSE)
        #define FALSE 0
    #endif
#endif

typedef enum {
    PIPE_TYPE_CONTROL     = 0,
    PIPE_TYPE_ISOCHRONOUS = 1,
    PIPE_TYPE_BULK        = 2,
    PIPE_TYPE_INTERRUPT   = 3
} USB_PIPE_TYPE;

#define WD_USB_MAX_PIPE_NUMBER 32
#define WD_USB_MAX_ENDPOINTS WD_USB_MAX_PIPE_NUMBER
#define WD_USB_MAX_INTERFACES 30
#define WD_USB_MAX_ALT_SETTINGS 255

typedef enum {
    WDU_DIR_IN     = 1,
    WDU_DIR_OUT    = 2,
    WDU_DIR_IN_OUT = 3
} WDU_DIR;

/* USB TRANSFER options */
enum {
    USB_ISOCH_RESET = 0x10,
    USB_ISOCH_FULL_PACKETS_ONLY = 0x20,
    /* Windows only, ignored on other OS: */
    USB_ABORT_PIPE = 0x40,
    USB_ISOCH_NOASAP = 0x80,
    USB_BULK_INT_URB_SIZE_OVERRIDE_128K = 0x100, /* Force a 128KB maximum
                                                    URB size */
    /* All OS */
    USB_STREAM_OVERWRITE_BUFFER_WHEN_FULL = 0x200,

    /* The following flags are no longer used beginning with v6.0: */
    USB_TRANSFER_HALT = 0x1,
    USB_SHORT_TRANSFER = 0x2,
    USB_FULL_TRANSFER = 0x4,
    USB_ISOCH_ASAP = 0x8
};

typedef PVOID WDU_REGISTER_DEVICES_HANDLE;

/* Descriptor types */
#define WDU_DEVICE_DESC_TYPE       0x01
#define WDU_CONFIG_DESC_TYPE       0x02
#define WDU_STRING_DESC_STRING     0x03
#define WDU_INTERFACE_DESC_TYPE    0x04
#define WDU_ENDPOINT_DESC_TYPE     0x05

/* Endpoint descriptor fields */
#define WDU_ENDPOINT_TYPE_MASK 0x03
#define WDU_ENDPOINT_DIRECTION_MASK 0x80
#define WDU_ENDPOINT_ADDRESS_MASK 0x0f
/* test direction bit in the bEndpointAddress field of an endpoint
 * descriptor. */
#define WDU_ENDPOINT_DIRECTION_OUT(addr) \
    (!((addr) & WDU_ENDPOINT_DIRECTION_MASK))
#define WDU_ENDPOINT_DIRECTION_IN(addr) \
    ((addr) & WDU_ENDPOINT_DIRECTION_MASK)
#define WDU_GET_MAX_PACKET_SIZE(x) \
    ((USHORT) (((x) & 0x7ff) * (1 + (((x) & 0x1800) >> 11))))

#ifndef LINUX
typedef enum {
    USB_DIR_IN     = 1,
    USB_DIR_OUT    = 2,
    USB_DIR_IN_OUT = 3
} USB_DIR;
#endif

typedef struct
{
    DWORD dwNumber; /**< Pipe number; zero for the default control pipe */
    DWORD dwMaximumPacketSize; /**< Maximum size of packets that can be
                                   transferred using this pipe */
    DWORD type;            /**< Transfer type for this pipe */
    DWORD direction;       /**< Direction of the transfer:
                                - WDU_DIR_IN or WDU_DIR_OUT for isochronous,
                                bulk or interrupt pipes.
                                - WDU_DIR_IN_OUT for control pipes. */
    DWORD dwInterval;      /**< Interval in milliseconds.
                               Relevant only to interrupt pipes. */
} WDU_PIPE_INFO;

typedef struct
{
    UCHAR bLength; /**< Size, in bytes, of the descriptor (9 bytes) */
    UCHAR bDescriptorType; /**< Interface descriptor (0x04) */
    UCHAR bInterfaceNumber; /**< Interface number */
    UCHAR bAlternateSetting; /**< Alternate setting number */
    UCHAR bNumEndpoints; /**< Number of endpoints used by this interface */
    UCHAR bInterfaceClass; /**< The interface's class code, as assigned by
                               USB-IF */
    UCHAR bInterfaceSubClass; /**< The interface's sub-class code, as assigned
                                  by USB-IF */
    UCHAR bInterfaceProtocol; /**< The interface's protocol code, as assigned
                                  by USB-IF */
    UCHAR iInterface; /**< Index of string descriptor that describes this
                          interface */
} WDU_INTERFACE_DESCRIPTOR;

typedef struct
{
    UCHAR bLength; /**< Size, in bytes, of the descriptor (7 bytes) */
    UCHAR bDescriptorType; /**< Endpoint descriptor (0x05) */
    UCHAR bEndpointAddress; /**< Endpoint address: Use bits 0–3 for endpoint
                            number, set bits 4–6 to zero (0), and set bit 7
                            to zero (0) for outbound data and to one (1)
                            for inbound data (ignored for control endpoints).
                            */
    UCHAR bmAttributes; /**< Specifies the transfer type for this endpoint
                        (control, interrupt, isochronous or bulk).
                        See the USB specification for further information. */
    USHORT wMaxPacketSize; /**< Maximum size of packets this endpoint can
                           send or receive */
    UCHAR bInterval; /**< Interval, in frame counts, for polling endpoint data
                     transfers. Ignored for bulk and control endpoints.
                     Must equal 1 for isochronous endpoints.
                     May range from 1 to 255 for interrupt endpoints. */
} WDU_ENDPOINT_DESCRIPTOR;

typedef struct
{
    UCHAR bLength; /**< Size, in bytes, of the descriptor */
    UCHAR bDescriptorType; /**< Configuration descriptor (0x02) */
    USHORT wTotalLength; /**< Total length, in bytes, of data returned */
    UCHAR bNumInterfaces; /**< Number of interfaces */
    UCHAR bConfigurationValue; /**< Configuration number */
    UCHAR iConfiguration; /**< Index of string descriptor that describes
                          this configuration */
    UCHAR bmAttributes; /**< Power settings for this configuration:
                            - self-powered
                            - remote wakeup (allows device to wake up the host)
                        */
    UCHAR MaxPower; /** Maximum power consumption for this configuration,
                    in 2mA units */
} WDU_CONFIGURATION_DESCRIPTOR;

typedef struct
{
    UCHAR bLength; /**< Size, in bytes, of the descriptor (18 bytes) */
    UCHAR bDescriptorType; /**< Device descriptor (0x01) */
    USHORT bcdUSB; /**< Number of the USB specification with which the
                   device complies */
    UCHAR bDeviceClass; /**< The device's class */
    UCHAR bDeviceSubClass; /**< The device's sub-class */
    UCHAR bDeviceProtocol; /**< The device's protocol */
    UCHAR bMaxPacketSize0; /**< Maximum size of transferred packets */

    USHORT idVendor; /**< Vendor ID, as assigned by USB-IF */
    USHORT idProduct; /**< Product ID, as assigned by the product manufacturer
                      */
    USHORT bcdDevice; /**< Device release numbe */
    UCHAR iManufacturer; /**< Index of manufacturer string descriptor */
    UCHAR iProduct; /**< Index of product string descriptor */
    UCHAR iSerialNumber; /**< Index of serial number string descriptor */
    UCHAR bNumConfigurations; /**< Number of possible configurations */
} WDU_DEVICE_DESCRIPTOR;

typedef struct
{
    WDU_INTERFACE_DESCRIPTOR Descriptor; /**< Interface descriptor information
                                         structure */
    PAD_TO_64(Descriptor)
    WDU_ENDPOINT_DESCRIPTOR *pEndpointDescriptors; /**< Pointer to the beginning
                                                   of an array of endpoint
                                                   descriptor information
                                                   structures for
                                                   the alternate setting's
                                                   endpoints */
    PAD_TO_64(pEndpointDescriptors)
    WDU_PIPE_INFO *pPipes; /**< Pointer to the beginning of an array of pipe
                         information structures for the alternate
                         setting's pipes */
    PAD_TO_64(pPipes)
} WDU_ALTERNATE_SETTING;

typedef struct
{
    WDU_ALTERNATE_SETTING *pAlternateSettings; /**< Pointer to the beginning of
                                               an array of alternate setting
                                               information structures
                                               for the interface's alternate
                                               settings */
    PAD_TO_64(pAlternateSettings)
    DWORD dwNumAltSettings; /**< Pointer to the beginning of an array of
                            endpoint descriptor information structures
                           for the alternate setting's endpoints */
    PAD_TO_64(dwNumAltSettings)
    WDU_ALTERNATE_SETTING *pActiveAltSetting; /**< Pointer to the beginning of an
                                              array of pipe information
                                              structures for the
                                              alternate setting's pipes */
    PAD_TO_64(pActiveAltSetting)
} WDU_INTERFACE;

typedef struct
{
    WDU_CONFIGURATION_DESCRIPTOR Descriptor; /**< Configuration descriptor
                                             information structure */
    DWORD dwNumInterfaces; /**< Number of interfaces supported by this
                           configuration */
    WDU_INTERFACE *pInterfaces; /**< Pointer to the beginning of an array of
                                interface information structures
                                for the configuration's interfaces */
    PAD_TO_64(pInterfaces)
} WDU_CONFIGURATION;

typedef struct {
    WDU_DEVICE_DESCRIPTOR Descriptor; /**< CDevice descriptor information
                                      structure */
    WDU_PIPE_INFO Pipe0; /**< Pipe information structure for the
                         device's default control pipe (pipe 0) */
    WDU_CONFIGURATION *pConfigs; /**< Pointer to the beginning of an array of
                                 configuration information structures
                                 describing the device's configurations */
    PAD_TO_64(pConfigs)
    WDU_CONFIGURATION *pActiveConfig; /**< Pointer to the device's active
                                      configuration information structure,
                                      stored in the pConfigs array
                                      */
    PAD_TO_64(pActiveConfig)
    WDU_INTERFACE *pActiveInterface[WD_USB_MAX_INTERFACES]; /**< Array of
                                    pointers to interface information
                                    structures;
                                    the non-NULL elements in the array represent
                                    the device's active interfaces.
                                    On Windows, the number of active interfaces
                                    is the number of interfaces supported by
                                    the active configuration, as stored in
                                    the pActiveConfig->dwNumInterfaces field.
                                    On Linux, the number of active interfaces
                                    is currently always 1, because
                                    the WDU_ATTACH_CALLBACK device-attach
                                    callback is called for each
                                    device interface. */
    PAD_TO_64_PTR_ARR(pActiveInterface, WD_USB_MAX_INTERFACES)
} WDU_DEVICE;

/* Note: Any devices found matching this table will be controlled */
typedef struct
{
    USHORT wVendorId; /**< Required USB Vendor ID to detect,
                      as assigned by USB-IF (*) */
    USHORT wProductId; /**< Required USB Product ID to detect,
                       as assigned by the product manufacturer (*) */
    UCHAR  bDeviceClass; /**< The device's class code, as assigned by
                         USB-IF (*) */
    UCHAR  bDeviceSubClass; /**< The device's sub-class code,
                            as assigned by USB-IF (*) */
    UCHAR  bInterfaceClass; /**< The interface's class code,
                            as assigned by USB-IF (*) */
    UCHAR  bInterfaceSubClass; /**< The interface's sub-class code,
                               as assigned by USB-IF (*) */
    UCHAR  bInterfaceProtocol; /**< The interface's protocol code,
                               as assigned by USB-IF (*) */
} WDU_MATCH_TABLE;

typedef struct
{
    DWORD dwUniqueID;
    PAD_TO_64(dwUniqueID)
    PVOID pBuf;
    PAD_TO_64(pBuf)
    DWORD dwBytes;
    DWORD dwOptions;
} WDU_GET_DEVICE_DATA;

/* these enum values can be used as dwProperty values, see structure
 * WD_GET_DEVICE_PROPERTY below. */
typedef enum
{
    WdDevicePropertyDeviceDescription, /**< Device description */
    WdDevicePropertyHardwareID, /**< The device's hardware IDs */
    WdDevicePropertyCompatibleIDs, /**< The device's compatible IDs */
    WdDevicePropertyBootConfiguration, /**< The hardware resources assigned
                                       to the device by the firmware,
                                       in raw data form */
    WdDevicePropertyBootConfigurationTranslated, /**< The hardware resources
                                                 assigned to the device by the
                                                 firmware, in translated form
                                                 */
    WdDevicePropertyClassName, /**< The name of the device's setup class,
                               in text format */
    WdDevicePropertyClassGuid, /**< The GUID for the device's setup class
                               (string format) */
    WdDevicePropertyDriverKeyName, /**< The name of the driver-specific
                                   registry key */
    WdDevicePropertyManufacturer, /**< Device manufacturer string */
    WdDevicePropertyFriendlyName, /**< Friendly device name (typically defined
                                  by the class installer), which can be used
                                  to distinguish between two similar devices */
    WdDevicePropertyLocationInformation, /**< Information about the device's
                                         Location on the bus (string format).
                                         The interpertation of this information
                                         is bus-specific. */
    WdDevicePropertyPhysicalDeviceObjectName, /**< The name of the Physical
                                              Device Object (PDO) for the
                                              device */
    WdDevicePropertyBusTypeGuid, /**< The GUID for the bus to which the device
                                 is connected */
    WdDevicePropertyLegacyBusType, /**< The bus type (e.g., PCIBus) */
    WdDevicePropertyBusNumber, /**< The legacy bus number of the bus to which
                               the device is connected */
    WdDevicePropertyEnumeratorName, /**< The name of the device's enumerator
                                    (e.g., "PCI" or "root") */
    WdDevicePropertyAddress, /**< The device's bus address.
                                 The interpertation of this address is
                                 bus-specific. */
    WdDevicePropertyUINumber, /**< A number associated with the device that
                              can be displayed in the user interface */
    WdDevicePropertyInstallState, /**< The device's installation state */
    WdDevicePropertyRemovalPolicy /**< The device's current removal
                                  policy (Windows) */
} WD_DEVICE_REGISTRY_PROPERTY;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwInterfaceNum;
    DWORD dwAlternateSetting;
    DWORD dwOptions;
} WDU_SET_INTERFACE;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum;
    DWORD dwOptions;
} WDU_RESET_PIPE;

typedef enum {
    WDU_WAKEUP_ENABLE = 0x1,
    WDU_WAKEUP_DISABLE = 0x2
} WDU_WAKEUP_OPTIONS;

typedef enum {
    WDU_SELECTIVE_SUSPEND_SUBMIT = 0x1,
    WDU_SELECTIVE_SUSPEND_CANCEL = 0x2,
} WDU_SELECTIVE_SUSPEND_OPTIONS;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum;
    DWORD dwOptions;
} WDU_HALT_TRANSFER;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwOptions;
} WDU_WAKEUP;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwOptions;
} WDU_SELECTIVE_SUSPEND;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwOptions;
} WDU_RESET_DEVICE;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum; /**< Pipe number on device. */
    DWORD fRead; /**< TRUE for read (IN) transfers; FALSE for write (OUT)
                  * transfers. */
    DWORD dwOptions; /**< USB_TRANSFER options:
                        USB_ISOCH_FULL_PACKETS_ONLY - For isochronous
                        transfers only. If set, only full packets will be
                        transmitted and the transfer function will return
                        when the amount of bytes left to transfer is less
                        than the maximum packet size for the pipe (the
                        function will return without transmitting the
                        remaining bytes). */
    PVOID pBuffer;   /**< Pointer to buffer to read/write. */
    PAD_TO_64(pBuffer)
    DWORD dwBufferSize; /**< Amount of bytes to transfer. */
    DWORD dwBytesTransferred; /**< Returns the number of bytes actually
                               * read/written */
    UCHAR SetupPacket[8]; /**< Setup packet for control pipe transfer. */
    DWORD dwTimeout; /**< Timeout for the transfer in milliseconds. Set to 0 for
                      * infinite wait. */
    PAD_TO_64(dwTimeout)
} WDU_TRANSFER;

typedef struct
{
    DWORD dwUniqueID;
    UCHAR bType;
    UCHAR bIndex;
    USHORT wLength;
    PVOID pBuffer;
    USHORT wLanguage;
} WDU_GET_DESCRIPTOR;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwOptions;
    DWORD dwPipeNum;
    DWORD dwBufferSize;
    DWORD dwRxSize;
    BOOL  fBlocking;
    DWORD dwRxTxTimeout;
    DWORD dwReserved;
} WDU_STREAM;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwOptions;
    BOOL  fIsRunning;
    DWORD dwLastError;
    DWORD dwBytesInBuffer;
    DWORD dwReserved;
} WDU_STREAM_STATUS;

#endif /* _WINDRVR_USB_H_ */

