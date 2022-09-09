#ifndef _PCI_STRINGS_H_
#define _PCI_STRINGS_H_

#include "wdc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
*  Reads data from a PCI device's configuration space and parses the data to
*  a string, if such a parsing is available. The parsing is based upon
*  information obtained from the official PCI Specification, 3rd Generation.
*
*   @param [in] hDev:       Handle to a WDC PCI device structure,
*                           returned by WDC_PciDeviceOpen()
*   @param [in] dwOffset:   Offset of a register in the configuration space
*                           to be read and parsed to a string, if a parsing
*                           is available.
*   @param [out] pBuf:      A pointer to a user preallocated buffer to be
*                           filled by the function. The buffer will not be
*                           filled if no parsing is available.
*   @param [in] dwInLen:    Size of the user preallocated buffer.
*   @param [out] pdwOutLen: A pointer to a DWORD which will hold the actual
*                           length of the string the function wrote to the
*                           buffer. If the buffer is too small for the text
*                           written by the function then the text will be
*                           truncated. We recommend preallocating a 1024 byte
*                           buffer to avoid any truncation.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV PciConfRegData2Str(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _Outptr_ PCHAR pBuf, _In_ DWORD dwInLen,
    _Outptr_ DWORD *pdwOutLen);

/**
* Reads data from a PCI Express device's extended configuration space and
* parses the data to a string, if such a parsing is available. The parsing
* is based upon information obtained from the official PCI Specification,
* 3rd Generation.
*
*   @param [in] hDev:       Handle to a WDC PCI device structure,
*                           returned by WDC_PciDeviceOpen()
*   @param [in] dwOffset:   Offset of a register in the configuration space
*                           to be read and parsed to a string, if a parsing
*                           is available.
*                           Offset should be the sum of the value obtained
*                           from WDC_PciGetExpressOffset()
*                           WDC_PciGetExpressOffset() and the desired
*                           PCI Express offset.
*   @param [out] pBuf:      A pointer to a user preallocated buffer to be
*                           filled by the function. The buffer will not be
*                           filled if no parsing is available.
*   @param [in] dwInLen:    Size of the user preallocated buffer.
*   @param [out] pdwOutLen: A pointer to a DWORD which will hold the actual
*                           length of the string the function wrote to the
*                           buffer. If the buffer is too small for the text
*                           written by the function then the text will be
*                           truncated. We recommend preallocating a 1024 byte
*                           buffer to avoid any truncation.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV PciExpressConfRegData2Str(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _Outptr_ PCHAR pBuf, _In_ DWORD dwInLen,
    _Outptr_ DWORD *pdwOutLen);
#ifdef __cplusplus
}
#endif

#endif /* _PCI_STRINGS_ */
