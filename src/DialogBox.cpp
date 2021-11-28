/*
 * DialogBoxに関する処理
 */
 
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "EyesOn.h"
#include "DialogBox.h"

// 固定値 
#define MAX_BUF		1024
#define MAX_INPUTHIST	10

// MsgBoxパラメータ
static char *l_pMsgStr = NULL;
static char *l_pMsgTitle = NULL;
static int l_nMsgType;

// InputBoxパラメータ
static char *l_pInputGuide = NULL;
static char *l_pInputTitle = NULL;
static Variable l_InputHist;

//----------------------------------------------------------------------
//■Dialogのメッセージ処理
//----------------------------------------------------------------------
// ダイアログ1(バージョン表示)プロシージャ
BOOL CALLBACK EventCall_MsgWnd(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	int stat;

	switch (msg) {
		case WM_INITDIALOG:
			return MsgWnd_Init(hWnd);
		case WM_COMMAND:
			stat = MsgWnd_Command(hWnd, HIWORD(wparam),
			                      LOWORD(wparam), (HWND)lparam);
			SendMessage(hMainWnd, WM_ACTIVATE, 0, 0);
			return stat;
	}
	return FALSE;
}

BOOL MsgWnd_Init(HWND hWnd)
{
	int x, y;
	char szBuf[MAX_BUF+1];

	// ダイアログの表示位置取得
	GetDispLoc(hWnd, &x, &y);
	// ダイアログを適切な位置に移動
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	sprintf(szBuf, "Version %s (%s)", VERSION, DATE);
	SetDlgItemText(hWnd, IDC_VERSION, szBuf);

	return FALSE;
}

BOOL MsgWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	switch (wID) {
		case IDOK:
			EndDialog(hWnd, IDOK);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

// ダイアログ2(詳細設定)プロシージャ
BOOL CALLBACK EventCall_RegWnd(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	int nStat;

	switch (msg) {
		case WM_INITDIALOG:
			return RegWnd_Init(hWnd);
		case WM_COMMAND:
			nStat = RegWnd_Command(hWnd, HIWORD(wparam),
			                      LOWORD(wparam), (HWND)lparam);
			SendMessage(hMainWnd, WM_ACTIVATE, 0, 0);
			return nStat;
	}
	return FALSE;
}

BOOL RegWnd_Init(HWND hWnd)
{
	HWND hList, hCombo1, hCombo2;
	int x, y, i, nIdx;
	char *pFile;
	char szBuf[MAX_BUF+1];

	// ダイアログの表示位置取得
	GetDispLoc(hWnd, &x, &y);
	// ダイアログを適切な位置に移動
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	hList = GetDlgItem(hMainWnd, ListView);
	nIdx = ListView_GetSelectionMark(hList);

	// タイトルの設定
	ListView_GetItemText(hList, nIdx, C_TITLE, szBuf, MAX_BUF);
	SetDlgItemText(hWnd, IDC_TITLE, szBuf);

	// リンク先の設定
	pFile = (char *)malloc(MAX_FULL_PATH + 1);
	ListView_GetItemText(hList, nIdx, C_LINK, pFile, MAX_FULL_PATH);
	SetDlgItemText(hWnd, IDC_LINK, pFile);
	free(pFile);

	// 監視周期の設定
	ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
	hCombo1 = GetDlgItem(hWnd, IDC_INTVL);
	for (i = 0; IntvlInfo[i].pDspStr; ++i) {
		SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)IntvlInfo[i].pDspStr);
		if (!strcmp(szBuf, IntvlInfo[i].pSetStr)) {
			SendMessage(hCombo1, CB_SETCURSEL, i, 0);
		}
	}

	// 監視パターンの設定
	ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
	hCombo2 = GetDlgItem(hWnd, IDC_PTN);
	for (i = 0; PtnInfo[i].pDspStr; ++i) {
		SendMessage(hCombo2, CB_ADDSTRING, 0, (LPARAM)PtnInfo[i].pDspStr);
		if (!strcmp(szBuf, PtnInfo[i].pSetStr)) {
			SendMessage(hCombo2, CB_SETCURSEL, i, 0);
		}
	}

	// 変化監視有無の設定
	ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
	if (!strcmp(szBuf, "1")) {
		Button_SetCheck(GetDlgItem(hWnd, IDC_ACT), BST_CHECKED);
		EnableWindow(hCombo1, TRUE);
		EnableWindow(hCombo2, TRUE);
	} else {
		Button_SetCheck(GetDlgItem(hWnd, IDC_ACT), BST_UNCHECKED);
		EnableWindow(hCombo1, FALSE);
		EnableWindow(hCombo2, FALSE);
	}

	return FALSE;
}

BOOL RegWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	HWND hList, hCombo1, hCombo2, hDlg;
	int nIdx, nCidx, iCheck, nStat, bErr;
	char *pFile, *pMsg;
	char szBuf[MAX_BUF+1];

	bErr = FALSE;
	switch (wID) {
		case IDOK:
			hList = GetDlgItem(hMainWnd, ListView);
			nIdx = ListView_GetSelectionMark(hList);

			// リンク先の設定
			pFile = (char *)malloc(MAX_FULL_PATH + 1);
			GetDlgItemText(hWnd, IDC_LINK, pFile, MAX_FULL_PATH);
			nStat = GetFileAttributes(pFile);
			if (nStat != -1 &&
				((nStat & FILE_ATTRIBUTE_ARCHIVE) ||
				 (nStat & FILE_ATTRIBUTE_DIRECTORY))) {
				SetListData(hList, nIdx, C_LINK, pFile, 0);
			} else {
				bErr = TRUE;
			}
			free(pFile);

			if (bErr) {
				pMsg = "リンク先が存在しないか、\nアクセス出来ません。";
				MsgBox(hMainWnd, pMsg, "Warning", MB_OK);

				// リンク先にカーソル設定
				hDlg = GetDlgItem(hWnd, IDC_LINK);
				SetFocus(hDlg);
				return FALSE;
			}

			// タイトルの設定
			GetDlgItemText(hWnd, IDC_TITLE, szBuf, MAX_BUF);
			SetListData(hList, nIdx, C_TITLE, szBuf, 0);

			// 変化監視有無の設定
			iCheck = Button_GetCheck(GetDlgItem(hWnd, IDC_ACT));
			if (iCheck == BST_CHECKED) {
				SendMessage(hMainWnd, WM_COMMAND, IDM_LIST_ON, 0);
			} else {
				SendMessage(hMainWnd, WM_COMMAND, IDM_LIST_OFF, 0);
			}

			// 監視周期の設定
			hCombo1 = GetDlgItem(hWnd, IDC_INTVL);
			nCidx = SendMessage(hCombo1, CB_GETCURSEL, 0, 0);
			SetListData(hList, nIdx, C_INTVL, IntvlInfo[nCidx].pSetStr, 0);

			// 監視パターンの設定
			hCombo2 = GetDlgItem(hWnd, IDC_PTN);
			nCidx = SendMessage(hCombo2, CB_GETCURSEL, 0, 0);
			SetListData(hList, nIdx, C_PTN, PtnInfo[nCidx].pSetStr, 0);

			EndDialog(hWnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;
		case IDC_ACT:
			hCombo1 = GetDlgItem(hWnd, IDC_INTVL);
			hCombo2 = GetDlgItem(hWnd, IDC_PTN);
			iCheck = Button_GetCheck(GetDlgItem(hWnd, IDC_ACT));
			if (iCheck == BST_CHECKED) {
				EnableWindow(hCombo1, TRUE);
				EnableWindow(hCombo2, TRUE);
			} else {
				EnableWindow(hCombo1, FALSE);
				EnableWindow(hCombo2, FALSE);
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

// ダイアログ0(MsgBox設定)プロシージャ
int MsgBox(HWND hWnd, char *pMsg, char *pTitle, int nType)
{
	HINSTANCE hInstance;
	int nStat;

	// パラメータの保存
	if (l_pMsgStr) free(l_pMsgStr);
	l_pMsgStr = strdup(pMsg);
	if (l_pMsgTitle) free(l_pMsgTitle);
	l_pMsgTitle = strdup(pTitle);
	l_nMsgType = nType;

	// ダイアログ表示
	hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	nStat = DialogBox(hInstance, "IDD_DIALOG0",
	                  hWnd, (DLGPROC)EventCall_MsgBox);

	return nStat;
}

BOOL CALLBACK EventCall_MsgBox(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int nStat;

	switch (msg) {
		case WM_INITDIALOG:
			return MsgBox_Init(hWnd);
		case WM_COMMAND:
			nStat = MsgBox_Command(hWnd, HIWORD(wParam),
			                      LOWORD(wParam), (HWND)lParam);
			SendMessage(hMainWnd, WM_ACTIVATE, 0, 0);
			return nStat;
	}
	return FALSE;
}

BOOL MsgBox_Init(HWND hWnd)
{
	HWND hDlg, hBtn1, hBtn2, hBtn3;
	int x, y, style;
	int nCurWd, nCurHt, nNedWd, nNedHt, nFntHt, nDifHt;

	// ボタンの設定
	hBtn1 = GetDlgItem(hWnd, IDBTN1);
	hBtn2 = GetDlgItem(hWnd, IDBTN2);
	hBtn3 = GetDlgItem(hWnd, IDBTN3);
	switch (l_nMsgType & MB_TYPEMASK) {
		case MB_OK:
			EnableWindow(hBtn1, FALSE);
			EnableWindow(hBtn3, FALSE);
			SetWindowText(hBtn2, "OK");
			ChgWinStyle(hBtn1, 0, WS_VISIBLE);
			ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			ChgWinStyle(hBtn3, 0, WS_VISIBLE);
			break;
		case MB_OKCANCEL:
			EnableWindow(hBtn1, FALSE);
			ChgWinStyle(hBtn1, 0, WS_VISIBLE);
			SetWindowText(hBtn2, "OK(&O)");
			SetWindowText(hBtn3, "ｷｬﾝｾﾙ(&C)");
			if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON2) {
				ChgWinStyle(hBtn3, BS_DEFPUSHBUTTON, 0xF);
			} else {
				ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			}
			break;
		case MB_YESNOCANCEL:
			SetWindowText(hBtn1, "はい(&Y)");
			SetWindowText(hBtn2, "いいえ(&N)");
			SetWindowText(hBtn3, "ｷｬﾝｾﾙ(&C)");
			if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON2) {
				ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			} else if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON3) {
				ChgWinStyle(hBtn3, BS_DEFPUSHBUTTON, 0xF);
			} else {
				ChgWinStyle(hBtn1, BS_DEFPUSHBUTTON, 0xF);
			}
			break;
		case MB_YESNO:
			EnableWindow(hBtn1, FALSE);
			ChgWinStyle(hBtn1, 0, WS_VISIBLE);
			SetWindowText(hBtn2, "はい(&Y)");
			SetWindowText(hBtn3, "いいえ(&N)");
			if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON2) {
				ChgWinStyle(hBtn3, BS_DEFPUSHBUTTON, 0xF);
			} else {
				ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			}
			break;
	}

	// 表示に必要なサイズ取得
	hDlg = MsgBox_GetDispSize(hWnd, IDC_NOTE, &nCurWd, &nCurHt,
	                                          &nNedWd, &nNedHt, &nFntHt);
	// 各ダイアログの位置、サイズ調整
	nDifHt = nNedHt - nCurHt;
	if (nDifHt > 0) {
		MsgBox_ChgWinRel(hWnd, 0, 0, 0, nDifHt);
		MsgBox_ChgWinRel(hDlg, 0, 0, 0, nDifHt);
		MsgBox_ChgWinRel(hBtn1, 0, nDifHt, 0, 0);
		MsgBox_ChgWinRel(hBtn2, 0, nDifHt, 0, 0);
		MsgBox_ChgWinRel(hBtn3, 0, nDifHt, 0, 0);
	} else if (nNedHt + nFntHt == 0) {
		MsgBox_ChgWinRel(hDlg, 0, nNedHt, 0, 0);
		ChgWinStyle(hDlg, SS_CENTER, SS_TYPEMASK);
	}

	// ダイアログの表示位置取得
	GetDispLoc(hWnd, &x, &y);
	// ダイアログを適切な位置に移動
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	// タイトルとメッセージ設定
	SetWindowText(hWnd, l_pMsgTitle);
	hDlg = GetDlgItem(hWnd, IDC_NOTE);
	SetWindowText(hDlg, l_pMsgStr);

	return FALSE;
}

// 表示に必要なサイズ取得
HWND MsgBox_GetDispSize(HWND hWnd, int nId, int *pCurWd, int *pCurHt, int *pNedWd, int *pNedHt, int *pFntHt)
{
	HWND hDlg;
	HDC hdc;
	HFONT hFont;
	LOGFONT lf;
	RECT CltRc;
	int w, h, dh, nh, style;

	// メッセージ領域のデバイス コンテキスト取得
	hDlg = GetDlgItem(hWnd, nId);
	hdc = GetDC(hDlg);

	// メッセージ領域のフォントを選択
	hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
	if (hFont) SelectObject(hdc, hFont);

	// フォント情報の取得
	GetObject(hFont, sizeof(LOGFONT), &lf);
	*pFntHt = lf.lfHeight;
	
	// メッセージ領域のサイズ取得
	GetClientRect(hDlg, &CltRc);
	*pCurWd = CltRc.right - CltRc.left;
	*pCurHt = CltRc.bottom - CltRc.top;

	// メッセージ領域で必要な表示サイズ取得
	DrawText(hdc, l_pMsgStr, -1, &CltRc, DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT | DT_EXTERNALLEADING);
	*pNedWd = CltRc.right - CltRc.left;
	*pNedHt = CltRc.bottom - CltRc.top;
	ReleaseDC(hDlg, hdc);

	return hDlg;
}

// 相対的なサイズ、位置変更
int MsgBox_ChgWinRel(HWND hWnd, int dx, int dy, int dw, int dh)
{
	HWND hPar;
	RECT rc, rc2;
	int ofset;

	// ウィンドウの座標取得
	GetWindowRect(hWnd, &rc);

	// サイズ変更
	if (dw != 0 || dh != 0) {
		SetWindowPos(hWnd, NULL, 0, 0, 
		             rc.right - rc.left + dw,
		             rc.bottom - rc.top + dh, SWP_NOMOVE);
	}
	
	// 位置変更
	if (dx != 0 || dy != 0) {
		hPar = GetParent(hWnd);
		GetWindowRect(hPar, &rc2);
		ofset =  GetSystemMetrics(SM_CYDLGFRAME)
		       + GetSystemMetrics(SM_CYCAPTION);
		SetWindowPos(hWnd, NULL,
		             rc.left - rc2.left + dx,
		             rc.top - rc2.top + dy - ofset, 0, 0, SWP_NOSIZE);
	}

	return TRUE;
}

BOOL MsgBox_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	WORD wRetId;

	switch (wID) {
		case IDBTN1:
		case IDBTN2:
		case IDBTN3:
			switch (l_nMsgType & MB_TYPEMASK) {
				case MB_OK:
					wRetId = IDOK;
					break;
				case MB_OKCANCEL:
					if (wID == IDBTN2) {
						wRetId = IDOK;
					} else {
						wRetId = IDCANCEL;
					}
					break;
				case MB_YESNOCANCEL:
					if (wID == IDBTN1) {
						wRetId = IDYES;
					} else if (wID == IDBTN2) {
						wRetId = IDNO;
					} else {
						wRetId = IDCANCEL;
					}
					break;
				case MB_YESNO:
					if (wID == IDBTN2) {
						wRetId = IDYES;
					} else {
						wRetId = IDNO;
					}
					break;
				default:
					wRetId = IDCANCEL;
			}
			EndDialog(hWnd, wRetId);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

// ダイアログ3(InputBox設定)プロシージャ
char *InputBox(HWND hWnd, char *pGuide, char *pTitle, char *pDefStr)
{
	HINSTANCE hInstance;
	int nStat;
	char *pInputStr;

	// パラメータの保存
	if (l_pInputGuide) free(l_pInputGuide);
	l_pInputGuide = strdup(pGuide);
	if (l_pInputTitle) free(l_pInputTitle);
	l_pInputTitle = strdup(pTitle);

	// 初期値の設定
	if (pDefStr) {
		SortHist(pDefStr, &l_InputHist, MAX_INPUTHIST);
	}

	// ダイアログ表示
	hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
	nStat = DialogBox(hInstance, "IDD_DIALOG3",
	                  hWnd, (DLGPROC)EventCall_InputBox);

	if (nStat == IDOK) {
		pInputStr = l_InputHist.get(0);
	} else {
		pInputStr = NULL;
	}

	return pInputStr;
}

BOOL CALLBACK EventCall_InputBox(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	int stat;

	switch (msg) {
		case WM_INITDIALOG:
			return InputBox_Init(hWnd);
		case WM_COMMAND:
			stat = InputBox_Command(hWnd, HIWORD(wparam),
			                        LOWORD(wparam), (HWND)lparam);
			SendMessage(hMainWnd, WM_ACTIVATE, 0, 0);
			return stat;
	}
	return FALSE;
}

BOOL InputBox_Init(HWND hWnd)
{	
	HWND hDlg, hCombo;
	int x, y, i;

	// ダイアログの表示位置取得
	GetDispLoc(hWnd, &x, &y);
	// ダイアログを適切な位置に移動
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	// タイトルとメッセージ設定
	SetWindowText(hWnd, l_pInputTitle);
	hDlg = GetDlgItem(hWnd, IDC_GUIDE);
	SetWindowText(hDlg, l_pInputGuide);
	hCombo = GetDlgItem(hWnd, IDC_INPUT);
	for (i = 0; i < l_InputHist.size(); ++i) {
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)l_InputHist.get(i));
	}
	SendMessage(hCombo, CB_SETCURSEL, 0, 0);

	return TRUE;
}

BOOL InputBox_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	int nIdx, nMax, i;
	char szBuf[MAX_BUF+1];

	switch (wID) {
		case IDOK:
			// 入力文字列の保存
			GetDlgItemText(hWnd, IDC_INPUT, szBuf, MAX_BUF);
			if (*szBuf) {
				SortHist(szBuf, &l_InputHist, MAX_INPUTHIST);
			}

			EndDialog(hWnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

// 共通処理 --------------------------------------------------------------------
// ダイアログの表示位置取得
void GetDispLoc(HWND hWnd, int *lpX, int *lpY)
{
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
	RECT BasRc, OwnRc, Screen;
	int x, y, OwnHt;

	GetWindowRect(hMainWnd, &BasRc);
	GetWindowRect(hWnd, &OwnRc);
	GetMonitorRect(&Screen);

	x = BasRc.left + int(((BasRc.right - BasRc.left + 1)
	                       - (OwnRc.right - OwnRc.left + 1)) / 2);
	if (x < 0) x = BasRc.right;
	OwnHt = OwnRc.bottom - OwnRc.top;
	if (Screen.bottom < (BasRc.top + OwnHt)) {
		y = Screen.bottom - OwnHt - GetSystemMetrics(SM_CYFRAME);
	} else {
		y = BasRc.top + GetSystemMetrics(SM_CYFRAME);
	}

	*lpX = x;
	*lpY = y;
}

// 履歴の並べ替え
void SortHist(char *pInput, Variable *pHist, int nHstMax)
{
	int nIdx, nMax, i;

	// 履歴内を検索
	nIdx = pHist->index(pInput);
	if (nIdx == -1) {
		// 履歴に追加
		pHist->ins(pInput, 0);
	} else {
		// 履歴の順番入替え
		pHist->move(nIdx, 0);
	}

	// 最大数を超えた履歴を削除
	nMax = pHist->size();
	if (nHstMax < nMax) {
		for (i = nHstMax; i < nMax; ++i) {
			pHist->del(nHstMax);
		}
	}
}

// ダイアログのテキスト取得
char *GetDlgText(HWND hWnd, int nDlgId)
{
	HWND hDlg;
	int nLen;
	char *pStr;

	hDlg = GetDlgItem(hWnd, nDlgId);
	nLen = GetWindowTextLength(hDlg);
	pStr = (char *)malloc(nLen + 1);
	GetWindowText(hDlg, pStr, nLen + 1);

	return pStr;
}

// ウィンドウのスタイル変更
void ChgWinStyle(HWND hWnd, int nValue, int nMask)
{
	int nStyle;

	nStyle = GetWindowLong(hWnd, GWL_STYLE);
	nStyle &= ~nMask;
	nStyle |= (nValue & nMask);
	SetWindowLong(hWnd, GWL_STYLE, nStyle);
}

