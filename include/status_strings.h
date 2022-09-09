/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _STATUS_STRINGS_H_
#define _STATUS_STRINGS_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "windrvr.h"

/**
*  Retrieves the status string that corresponds to a status code.
*
*   @param [in] dwStatus: A numeric status code
*
* @return
*  Returns the verbal status description (string) that corresponds
*  to the specified numeric status code.
*
*/
const char * DLLCALLCONV Stat2Str(_In_ DWORD dwStatus);

#ifdef __cplusplus
}
#endif

#endif /* _STATUS_STRINGS_H_ */

