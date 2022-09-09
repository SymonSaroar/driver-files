/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _DIAG_LIB_H_
#define _DIAG_LIB_H_

/******************************************************************************
*  File: diag_lib.h - Shared WD user-mode diagnostics API header.             *
*******************************************************************************/

#if !defined(__KERNEL__)

#ifdef __cplusplus
extern "C" {
#endif

#include "windrvr.h"

#if defined(UNIX)
#include <sys/time.h>
    typedef struct timeval TIME_TYPE;
#else
    typedef LARGE_INTEGER TIME_TYPE;
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Cancel selection */
#define DIAG_CANCEL 'x'

/* Exit menu */
#define DIAG_EXIT_MENU 99

/* Invalid menu */
#define DIAG_INVALID_MENU (unsigned int)-1

/* Menu Tree */
#define MAX_NAME 128
#define MAX_TITLE_NAME 256
#define MENU_MAX_CHILDREN 16

struct _DIAG_MENU_OPTION;

typedef PVOID MENU_CALLBACK_CTX;
typedef DWORD (*MENU_CALLBACK)(MENU_CALLBACK_CTX cbCtx);
typedef BOOL (*MENU_IS_HIDDEN_CALLBACK)(struct _DIAG_MENU_OPTION *pMenu);

typedef struct _DIAG_MENU_OPTION {
    CHAR cOptionName[MAX_NAME]; /**< The name of the option that will be
                                     printed before selecting this menu */
    CHAR cTitleName[MAX_TITLE_NAME]; /**< The title name that will be printed at
                                    the top of the menu(can be empty) */

    MENU_CALLBACK cbEntry;     /**< Callback function that runs when entering
                                    this menu */
    MENU_CALLBACK cbExit;      /**< Callback function that runs when exiting
                                    this menu */
    PVOID pCbCtx;               /**< The context needed for this menu
                                    (varibles that the menu needs to use)*/

    BOOL fIsHidden;            /**< Determine if this option should be visible
                                    to the user, depending on its context*/
    MENU_IS_HIDDEN_CALLBACK cbIsHidden; /**< Callback that returns whether
                                               this menu should be hidden,
                                               depending on the context.
                                               This function is called
                                               each time inside the menu loop */

    DWORD dwCurrentNumberOfChildren; /**< Current number of child menus in the
                                          pChildMenus array */
    struct _DIAG_MENU_OPTION *pChildMenus[MENU_MAX_CHILDREN]; /**< Array of all
                                                            menus that can be
                                                            accessed from this
                                                            menu */
    struct _DIAG_MENU_OPTION *pParentMenu; /**< Parent of this menu. When
                                                exiting the menu, the parent
                                                menu will be shown */
} DIAG_MENU_OPTION;

#define OPTIONS_SIZE(options) (sizeof(options) / sizeof(DIAG_MENU_OPTION))

/**
*  Run the menu loop with the given menu root
*
*   @param [in] pMenuRoot: Pointer to the root of the menu
*
* @return  Returns the menu exit code(e.g the program exit code)
*/
DWORD DIAG_MenuRun(DIAG_MENU_OPTION *pMenuRoot);

/**
*  Sets the context and the parent members for each given menu
*  in the menu array
*
*   @param [in,out] pMenusArr:   Array of menu options
*   @param [in] dwSize:      Size of pMenusArr array
*   @param [in] pCtx:        Context to set for each menu
*   @param [in] pParentMenu: Parent of each menu in the array
*
*/
void DIAG_MenuSetCtxAndParentForMenus(DIAG_MENU_OPTION *pMenusArr,
    DWORD dwSize, PVOID pCtx, DIAG_MENU_OPTION *pParentMenu);

/****************************************************************
* Function: DIAG_PrintHexBuffer()                               *
*   Print a buffer in hexadecimal format                        *
* Parameters:                                                   *
*   pBuf [in] - Pointer to buffer                               *
*   dwBytes [in] - Number of bytes to print                     *
*   fAscii [in] - If TRUE, print the buffer also as an ASCII    *
*                 string                                        *
* Return Value:                                                 *
*   None                                                        *
*****************************************************************/
void DIAG_PrintHexBuffer(PVOID pBuf, DWORD dwBytes, BOOL fAscii);

/****************************************************************
* Function: DIAG_GetHexChar                                     *
*   Get a hexadecimal character from user                       *
* Parameters:                                                   *
*   None                                                        *
* Return Value:                                                 *
*   Character received                                          *
*****************************************************************/
int DIAG_GetHexChar(void);

/****************************************************************
* Function: DIAG_GetHexBuffer()                                 *
*   Get a hexadecimal buffer from user                          *
* Parameters:                                                   *
*   pBuffer [in/out] Pointer to buffer to be filled with data   *
*   dwBytes [in] Length of buffer                               *
* Return Value:                                                 *
*   Size of buffer received                                     *
*****************************************************************/
DWORD DIAG_GetHexBuffer(PVOID pBuffer, DWORD dwBytes);
void DIAG_FillHexBuffer(PVOID pBuffer, DWORD dwBytes, BYTE ch, BOOL fRandNum);
double time_diff(TIME_TYPE *end, TIME_TYPE *start);
void DIAG_PrintPerformance(UINT64 qwBytes, TIME_TYPE *startTime);
BOOL get_cur_time(TIME_TYPE *time);

typedef enum {
    DIAG_INPUT_CANCEL = -1,
    DIAG_INPUT_FAIL = 0,
    DIAG_INPUT_SUCCESS = 1
} DIAG_INPUT_RESULT;

/* Get menu option from user */
DIAG_INPUT_RESULT DIAG_GetMenuOption(DWORD *pdwOption, DWORD dwMax);

/* Get numeric value from user
   To avoid range check, set min == max (e.g. set both min and max to 0) */
DIAG_INPUT_RESULT DIAG_InputNum(PVOID pInput, const CHAR *sInputText,
    BOOL fHex, DWORD dwSize, UINT64 min, UINT64 max);

#define DIAG_InputDWORD(pdwInput, sInputText, fHex, min, max) \
    DIAG_InputNum((PVOID)(pdwInput), sInputText, fHex, sizeof(DWORD), min, max)

#define DIAG_InputWORD(pwInput, sInputText, fHex, min, max) \
    DIAG_InputNum((PVOID)(pwInput), sInputText, fHex, sizeof(WORD), min, max)

#define DIAG_InputBYTE(pbInput, sInputText, fHex, min, max) \
    DIAG_InputNum((PVOID)(pwInput), sInputText, fHex, sizeof(BYTE), min, max)

#define DIAG_InputUINT32(pu32Input, sInputText, fHex, min, max) \
    DIAG_InputNum((PVOID)(pu32Input), sInputText, fHex, sizeof(UINT32), min, \
        max)

#define DIAG_InputUINT64(pu64Input, sInputText, fHex, min, max) \
    DIAG_InputNum((PVOID)(pu64Input), sInputText, fHex, sizeof(UINT64), min, \
        max)

#ifdef __cplusplus
}
#endif

#endif

#endif /* _DIAG_LIB_H_ */

