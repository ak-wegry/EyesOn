/*
 * DialogBox�Ɋւ��鏈��
 */
 
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "EyesOn.h"
#include "DialogBox.h"

// �Œ�l 
#define MAX_BUF		1024
#define MAX_INPUTHIST	10

// MsgBox�p�����[�^
static char *l_pMsgStr = NULL;
static char *l_pMsgTitle = NULL;
static int l_nMsgType;

// InputBox�p�����[�^
static char *l_pInputGuide = NULL;
static char *l_pInputTitle = NULL;
static Variable l_InputHist;

//----------------------------------------------------------------------
//��Dialog�̃��b�Z�[�W����
//----------------------------------------------------------------------
// �_�C�A���O1(�o�[�W�����\��)�v���V�[�W��
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

	// �_�C�A���O�̕\���ʒu�擾
	GetDispLoc(hWnd, &x, &y);
	// �_�C�A���O��K�؂Ȉʒu�Ɉړ�
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

// �_�C�A���O2(�ڍאݒ�)�v���V�[�W��
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

	// �_�C�A���O�̕\���ʒu�擾
	GetDispLoc(hWnd, &x, &y);
	// �_�C�A���O��K�؂Ȉʒu�Ɉړ�
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	hList = GetDlgItem(hMainWnd, ListView);
	nIdx = ListView_GetSelectionMark(hList);

	// �^�C�g���̐ݒ�
	ListView_GetItemText(hList, nIdx, C_TITLE, szBuf, MAX_BUF);
	SetDlgItemText(hWnd, IDC_TITLE, szBuf);

	// �����N��̐ݒ�
	pFile = (char *)malloc(MAX_FULL_PATH + 1);
	ListView_GetItemText(hList, nIdx, C_LINK, pFile, MAX_FULL_PATH);
	SetDlgItemText(hWnd, IDC_LINK, pFile);
	free(pFile);

	// �Ď������̐ݒ�
	ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
	hCombo1 = GetDlgItem(hWnd, IDC_INTVL);
	for (i = 0; IntvlInfo[i].pDspStr; ++i) {
		SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)IntvlInfo[i].pDspStr);
		if (!strcmp(szBuf, IntvlInfo[i].pSetStr)) {
			SendMessage(hCombo1, CB_SETCURSEL, i, 0);
		}
	}

	// �Ď��p�^�[���̐ݒ�
	ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
	hCombo2 = GetDlgItem(hWnd, IDC_PTN);
	for (i = 0; PtnInfo[i].pDspStr; ++i) {
		SendMessage(hCombo2, CB_ADDSTRING, 0, (LPARAM)PtnInfo[i].pDspStr);
		if (!strcmp(szBuf, PtnInfo[i].pSetStr)) {
			SendMessage(hCombo2, CB_SETCURSEL, i, 0);
		}
	}

	// �ω��Ď��L���̐ݒ�
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

			// �����N��̐ݒ�
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
				pMsg = "�����N�悪���݂��Ȃ����A\n�A�N�Z�X�o���܂���B";
				MsgBox(hMainWnd, pMsg, "Warning", MB_OK);

				// �����N��ɃJ�[�\���ݒ�
				hDlg = GetDlgItem(hWnd, IDC_LINK);
				SetFocus(hDlg);
				return FALSE;
			}

			// �^�C�g���̐ݒ�
			GetDlgItemText(hWnd, IDC_TITLE, szBuf, MAX_BUF);
			SetListData(hList, nIdx, C_TITLE, szBuf, 0);

			// �ω��Ď��L���̐ݒ�
			iCheck = Button_GetCheck(GetDlgItem(hWnd, IDC_ACT));
			if (iCheck == BST_CHECKED) {
				SendMessage(hMainWnd, WM_COMMAND, IDM_LIST_ON, 0);
			} else {
				SendMessage(hMainWnd, WM_COMMAND, IDM_LIST_OFF, 0);
			}

			// �Ď������̐ݒ�
			hCombo1 = GetDlgItem(hWnd, IDC_INTVL);
			nCidx = SendMessage(hCombo1, CB_GETCURSEL, 0, 0);
			SetListData(hList, nIdx, C_INTVL, IntvlInfo[nCidx].pSetStr, 0);

			// �Ď��p�^�[���̐ݒ�
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

// �_�C�A���O0(MsgBox�ݒ�)�v���V�[�W��
int MsgBox(HWND hWnd, char *pMsg, char *pTitle, int nType)
{
	HINSTANCE hInstance;
	int nStat;

	// �p�����[�^�̕ۑ�
	if (l_pMsgStr) free(l_pMsgStr);
	l_pMsgStr = strdup(pMsg);
	if (l_pMsgTitle) free(l_pMsgTitle);
	l_pMsgTitle = strdup(pTitle);
	l_nMsgType = nType;

	// �_�C�A���O�\��
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

	// �{�^���̐ݒ�
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
			SetWindowText(hBtn3, "��ݾ�(&C)");
			if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON2) {
				ChgWinStyle(hBtn3, BS_DEFPUSHBUTTON, 0xF);
			} else {
				ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			}
			break;
		case MB_YESNOCANCEL:
			SetWindowText(hBtn1, "�͂�(&Y)");
			SetWindowText(hBtn2, "������(&N)");
			SetWindowText(hBtn3, "��ݾ�(&C)");
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
			SetWindowText(hBtn2, "�͂�(&Y)");
			SetWindowText(hBtn3, "������(&N)");
			if ((l_nMsgType & MB_DEFMASK) == MB_DEFBUTTON2) {
				ChgWinStyle(hBtn3, BS_DEFPUSHBUTTON, 0xF);
			} else {
				ChgWinStyle(hBtn2, BS_DEFPUSHBUTTON, 0xF);
			}
			break;
	}

	// �\���ɕK�v�ȃT�C�Y�擾
	hDlg = MsgBox_GetDispSize(hWnd, IDC_NOTE, &nCurWd, &nCurHt,
	                                          &nNedWd, &nNedHt, &nFntHt);
	// �e�_�C�A���O�̈ʒu�A�T�C�Y����
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

	// �_�C�A���O�̕\���ʒu�擾
	GetDispLoc(hWnd, &x, &y);
	// �_�C�A���O��K�؂Ȉʒu�Ɉړ�
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	// �^�C�g���ƃ��b�Z�[�W�ݒ�
	SetWindowText(hWnd, l_pMsgTitle);
	hDlg = GetDlgItem(hWnd, IDC_NOTE);
	SetWindowText(hDlg, l_pMsgStr);

	return FALSE;
}

// �\���ɕK�v�ȃT�C�Y�擾
HWND MsgBox_GetDispSize(HWND hWnd, int nId, int *pCurWd, int *pCurHt, int *pNedWd, int *pNedHt, int *pFntHt)
{
	HWND hDlg;
	HDC hdc;
	HFONT hFont;
	LOGFONT lf;
	RECT CltRc;
	int w, h, dh, nh, style;

	// ���b�Z�[�W�̈�̃f�o�C�X �R���e�L�X�g�擾
	hDlg = GetDlgItem(hWnd, nId);
	hdc = GetDC(hDlg);

	// ���b�Z�[�W�̈�̃t�H���g��I��
	hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
	if (hFont) SelectObject(hdc, hFont);

	// �t�H���g���̎擾
	GetObject(hFont, sizeof(LOGFONT), &lf);
	*pFntHt = lf.lfHeight;
	
	// ���b�Z�[�W�̈�̃T�C�Y�擾
	GetClientRect(hDlg, &CltRc);
	*pCurWd = CltRc.right - CltRc.left;
	*pCurHt = CltRc.bottom - CltRc.top;

	// ���b�Z�[�W�̈�ŕK�v�ȕ\���T�C�Y�擾
	DrawText(hdc, l_pMsgStr, -1, &CltRc, DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT | DT_EXTERNALLEADING);
	*pNedWd = CltRc.right - CltRc.left;
	*pNedHt = CltRc.bottom - CltRc.top;
	ReleaseDC(hDlg, hdc);

	return hDlg;
}

// ���ΓI�ȃT�C�Y�A�ʒu�ύX
int MsgBox_ChgWinRel(HWND hWnd, int dx, int dy, int dw, int dh)
{
	HWND hPar;
	RECT rc, rc2;
	int ofset;

	// �E�B���h�E�̍��W�擾
	GetWindowRect(hWnd, &rc);

	// �T�C�Y�ύX
	if (dw != 0 || dh != 0) {
		SetWindowPos(hWnd, NULL, 0, 0, 
		             rc.right - rc.left + dw,
		             rc.bottom - rc.top + dh, SWP_NOMOVE);
	}
	
	// �ʒu�ύX
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

// �_�C�A���O3(InputBox�ݒ�)�v���V�[�W��
char *InputBox(HWND hWnd, char *pGuide, char *pTitle, char *pDefStr)
{
	HINSTANCE hInstance;
	int nStat;
	char *pInputStr;

	// �p�����[�^�̕ۑ�
	if (l_pInputGuide) free(l_pInputGuide);
	l_pInputGuide = strdup(pGuide);
	if (l_pInputTitle) free(l_pInputTitle);
	l_pInputTitle = strdup(pTitle);

	// �����l�̐ݒ�
	if (pDefStr) {
		SortHist(pDefStr, &l_InputHist, MAX_INPUTHIST);
	}

	// �_�C�A���O�\��
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

	// �_�C�A���O�̕\���ʒu�擾
	GetDispLoc(hWnd, &x, &y);
	// �_�C�A���O��K�؂Ȉʒu�Ɉړ�
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE);

	// �^�C�g���ƃ��b�Z�[�W�ݒ�
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
			// ���͕�����̕ۑ�
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

// ���ʏ��� --------------------------------------------------------------------
// �_�C�A���O�̕\���ʒu�擾
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

// �����̕��בւ�
void SortHist(char *pInput, Variable *pHist, int nHstMax)
{
	int nIdx, nMax, i;

	// �����������
	nIdx = pHist->index(pInput);
	if (nIdx == -1) {
		// �����ɒǉ�
		pHist->ins(pInput, 0);
	} else {
		// �����̏��ԓ��ւ�
		pHist->move(nIdx, 0);
	}

	// �ő吔�𒴂����������폜
	nMax = pHist->size();
	if (nHstMax < nMax) {
		for (i = nHstMax; i < nMax; ++i) {
			pHist->del(nHstMax);
		}
	}
}

// �_�C�A���O�̃e�L�X�g�擾
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

// �E�B���h�E�̃X�^�C���ύX
void ChgWinStyle(HWND hWnd, int nValue, int nMask)
{
	int nStyle;

	nStyle = GetWindowLong(hWnd, GWL_STYLE);
	nStyle &= ~nMask;
	nStyle |= (nValue & nMask);
	SetWindowLong(hWnd, GWL_STYLE, nStyle);
}

