/*
 * DialogBoxÇ…ä÷Ç∑ÇÈèàóù
 */

#ifndef DIALOGBOX_INCLUDED
#define DIALOGBOX_INCLUDED

#include "Variable.h"

BOOL CALLBACK EventCall_MsgWnd(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL MsgWnd_Init(HWND hWnd);
BOOL MsgWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
BOOL CALLBACK EventCall_RegWnd(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL RegWnd_Init(HWND hWnd);
BOOL RegWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
int MsgBox(HWND hWnd, char *pMsg, char *pTitle, int nType);

BOOL CALLBACK EventCall_MsgBox(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL MsgBox_Init(HWND hWnd);
HWND MsgBox_GetDispSize(HWND hWnd, int nId, int *pCurWd, int *pCurHt, int *pNedWd, int *pNedHt, int *pFntHt);
int MsgBox_ChgWinRel(HWND hWnd, int dx, int dy, int dw, int dh);
BOOL MsgBox_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
char *InputBox(HWND hWnd, char *pGuide, char *pTitle, char *pDefStr);
BOOL CALLBACK EventCall_InputBox(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL InputBox_Init(HWND hWnd);
BOOL InputBox_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
void GetDispLoc(HWND hWnd, int *lpX, int *lpY);
void SortHist(char *pInput, Variable *pHist, int nHstMax);
char *GetDlgText(HWND hWnd, int nDlgId);
void ChgWinStyle(HWND hWnd, int nValue, int nMask);

#endif
