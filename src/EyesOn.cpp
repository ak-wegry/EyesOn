/*
 * �t�@�C���ω��Ď��c�[��
 *======================================================================
 *[�ύX����]
 * Ver0.00  2020/04/16 �쐬�J�n
 * Ver1.00  2020/07/16 �V�K�쐬
 * Ver1.01  2020/07/20 �t�H���_������t�@�C�������o�̕s��C��
 *                     ���j���[�́u�J���v���s���A��ԕύX�̕s��C��
 *                     �t�H���_�I���ɂ��V�K���X�g�o�^�̒ǉ�
 * Ver1.02  2020/08/19 ���j���[����V�K�t�@�C��/�t�H���_�o�^�폜
 * Ver1.03  2020/09/29 ���X�g�폜���̃^�C�g���̃C���f���g�\���C��
 *                     ��ԑJ�ځu�A�N�Z�X�s�ˊĎ��Ώہv�̕s��C��
 *                     ���X�g�I�����́u�A�N�Z�X�s�v��ԕ\���ǉ�
 * Ver1.04  2021/02/02 �Ď�������3/6/9���Ԃ�ǉ�
 */

#include <windows.h>
#include <psapi.h>
#include <commctrl.h>
#include <winnetwk.h>
#include <stdio.h>
#include <time.h>
#include <htmlhelp.h>
#include <shlobj.h>
#include "resource.h"
#include "EyesOn.h"
#include "CalTime.h"
#include "Variable.h"
#include "OpeString.h"
#include "DialogBox.h"

// �Œ�l 
#define MAX_BUF			1024
#define PARAM_FILE		"EyesOn.ini"
#define KEYPARAM		"RESTART"	// �V���[�g�J�b�g����̋N�����������ċN��
#define WAKEUP_TIMER	300000	// �^�C�}�[5��
//#define WAKEUP_TIMER	60000	// �^�C�}�[1��(�f�o�b�O�p)
#define SORT_ASCEND		1		// �����\�[�g
#define SORT_DESCEND	2		// �~���\�[�g
#define SRCH_NONE		0		// �����Ȃ�
#define SRCH_HEAD		1		// �擪1��������
#define SRCH_INCLUDE	2		// 1�����܂ތ���
#define SRCH_MATCH		3		// �����񌟍�
#define FONT_HEIGHT		19		// �t�H���g�̍���

// �O���[�o���ϐ�
HWND hMainWnd; 
int g_nRegWithWatch = 0;// �o�^���̊Ď����(0:�Ď��Ȃ��A1:�Ď�����)
int g_nCharSrchPtn = 0;	// 1�����̃��X�g����(0:�Ȃ��A1:�擪1�����A2:1�������܂�)

POPMENU_INFO ActInfo[] = {
	{ "����"          , "1"   , IDM_LIST_ON    },
	{ "�Ȃ�"          , "0"   , IDM_LIST_OFF   },
	{ NULL            , NULL  , 0              }};

POPMENU_INFO IntvlInfo[] = {
	{ "9����"         , "9:00", IDM_LIST_9HOUR },
	{ "6����"         , "6:00", IDM_LIST_6HOUR },
	{ "3����"         , "3:00", IDM_LIST_3HOUR },
	{ "1����"         , "1:00", IDM_LIST_1HOUR },
	{ "30��"          , "0:30", IDM_LIST_30MIN },
	{ "5��"           , "0:05", IDM_LIST_5MIN  },
//	{ "1��"           , "0:01", IDM_LIST_1MIN  },
	{ NULL            , NULL  , 0              }};

POPMENU_INFO PtnInfo[] = {
	{ "�X�V+�ǉ�"     , "0"   , IDM_LIST_PTN0 },
	{ "�X�V+�ǉ�+�폜", "1"   , IDM_LIST_PTN1 },
	{ NULL            , NULL  , 0             }};

char szDebug[MAX_BUF+1];		// �f�o�b�O�\���p

// �t�@�C�����[�J���ϐ�
static HFONT hFont_MainWnd;			// �t�H���g
static WNDPROC lpfnMainProc;		// CallBack�֐�
static char ParamFile[MAX_PATH+1];	// �p�����[�^�t�@�C����
static int nTimerId = 1;			// �^�C�}�[ID
static int nSortItem[NUM_ITEM];		// �e�����̃\�[�g��
static CHOOSEFONT cf;				// �t�H���g�ݒ�_�C�A���O�̏��
static LOGFONT logfont;				// �t�H���g���
static char *pExcludeFile = NULL;	// �A�N�Z�X�����擾���̏��O�t�@�C��

void DebugLog(char *pLog);

//----------------------------------------------------------------------
//�����C���̊֐� 
//----------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    HWND hWnd;		// ���C���E�C���h�E�̃n���h��
    WNDCLASS wc;	// WNDCLASS�\����
	MSG msg;
	int nStat, nRestart = TRUE;
	char *pCmd;
	Variable Param;

	// �u�V���[�g�J�b�g����̋N���v���������ċN��
	pCmd = GetCommandLine();
	if (pCmd) {
		Param.StrSplit(pCmd, " ", TRUE, TRUE);
		if (Param.size() == 2) {
			if (!strcmp(Param.get(1), KEYPARAM)) nRestart = FALSE;
		}

		if (nRestart) {
			ShellExecute(NULL, NULL, Param.get(0), KEYPARAM,
						 NULL, SW_SHOWNORMAL);
			return 0; 
		}
	} else {
		return 0; 
	}

	// WNDCLASS�\���̂�0�ŏ�����
	ZeroMemory(&wc,sizeof(WNDCLASS));
	// WNDCLASS�\���̂̐ݒ�
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= EventCall_MainWnd;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	//wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIcon			= LoadIcon(hInstance, "IDR_ICON");
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName		= MAKEINTRESOURCE(IDR_MENU);	// ���j���[��t����
	wc.lpszClassName	= "EyesOn";
	// �E�B���h�E �N���X��o�^
	if (RegisterClass(&wc) == 0)
		return 0; 

	// ���C���E�C���h�E�̐���
	hWnd = CreateWindowEx(
	           WS_EX_ACCEPTFILES | WS_EX_CONTROLPARENT | WS_EX_WINDOWEDGE,
	           wc.lpszClassName, "EyesOn",
	           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
	           450, 300, NULL, NULL, hInstance, NULL); 
	if (hWnd == NULL)
		return 0;
	hMainWnd = hWnd;
 
    // �E�C���h�E�̕\��
    ShowWindow(hWnd, nCmdShow);

	// �t�H���g�ݒ�_�C�A���O�̐ݒ�
	logfont.lfHeight = -1 * FONT_HEIGHT;
	logfont.lfCharSet = SHIFTJIS_CHARSET;
	strcpy(logfont.lfFaceName, "�l�r �S�V�b�N");
	setcf(hWnd, &cf);
 
	// �A�N�Z�X�����擾���̏��O�t�@�C�����ݒ�
	pExcludeFile = strdup("~*;.*");

	// ���b�Z�[�W���[�v
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg); 
		DispatchMessage(&msg);   
	} 
	return (msg.wParam);
}

//----------------------------------------------------------------------
//�����C���E�C���h�E�̃��b�Z�[�W����
//----------------------------------------------------------------------
LRESULT CALLBACK EventCall_MainWnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hList;
	INITCOMMONCONTROLSEX ic;
	LONG nStyle, nStat;
	LV_DISPINFO *lvinfo;
	NM_LISTVIEW *pNMLV;
	MENUITEMINFO MenuInfo;

	switch (uMsg) {
		case WM_CREATE: 
			MainWnd_Create(hWnd, (LPCREATESTRUCT)lParam);

			nStat = SetTimer(hWnd, nTimerId, WAKEUP_TIMER, NULL);
			if (nStat != 0) nTimerId = nStat;
			break;

		case WM_TIMER:
			KillTimer(hWnd, nTimerId);

			MainWnd_Timer(hWnd, wParam);
			nStat = SetTimer(hWnd, nTimerId, WAKEUP_TIMER, NULL);
			if (nStat != 0) nTimerId = nStat;
			break;

		case WM_SIZE:
			MainWnd_Resize(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_ACTIVATE:
			MainWnd_Activate(hWnd, LOWORD(wParam), HIWORD(wParam));
			break;

		case WM_DROPFILES:
			hList = GetDlgItem(hMainWnd, ListView);
			ListView_DropFiles(hList, (HDROP)wParam);
			break;

		case WM_DESTROY:
			KillTimer(hWnd, nTimerId);
			SaveParamFile();
			DeleteObject(hFont_MainWnd);
			if (pExcludeFile) free(pExcludeFile);
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
			MainWnd_Command(hWnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			if ((int)wParam == ListView) {
				lvinfo = (LV_DISPINFO *)lParam;
			   switch (lvinfo->hdr.code) {
					case LVN_COLUMNCLICK:
						pNMLV = (NM_LISTVIEW *)lParam;
						if (nSortItem[pNMLV->iSubItem] == SORT_ASCEND) {
							nSortItem[pNMLV->iSubItem] = SORT_DESCEND;
						} else {
							nSortItem[pNMLV->iSubItem] = SORT_ASCEND;
						}
						hList = GetDlgItem(hMainWnd, ListView);
						ListView_SortItems(hList, SortComp, pNMLV->iSubItem);
						if (nSortItem[pNMLV->iSubItem] == SORT_ASCEND) {
							DspStatus("Sort lists in ascending order.");
						} else {
							DspStatus("Sort lists in descending order.");
						}
						break;

				}
			}
			break;

		case WM_MENUSELECT:
			MainWnd_MenuSelect(hWnd);
			break;

		case WM_MENURBUTTONUP:
			Event_MenuRButton((HMENU)lParam, wParam);
			break;

		case WM_CLOSE:
			// �E�B���h�E���A�N�e�B�u�ɂ��A���̈ʒu�ƃT�C�Y�ŕ\��
			ShowWindow(hWnd, SW_SHOWNORMAL);
			// break�Ȃ�
		default:
			return(DefWindowProc(hWnd, uMsg, wParam, lParam)); 
	}
	return 0;
}

LRESULT MainWnd_Create(HWND hWnd, LPCREATESTRUCT cs)
{
	HWND hList, hStatus;
	DWORD dwStyle;

	// �t�H���g�I�u�W�F�N�g�̍쐬
	hFont_MainWnd = CreateFont(-1 * FONT_HEIGHT, 0, 0, 0, 400, 0, 0, 0,
							   128, 3, 2, 1, 49, "�l�r �S�V�b�N");

	// ���X�g�r���[�̍쐬
	hList =
	CreateWindowEx(0, WC_LISTVIEW, "",
	    WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS,
	    0, 0, 450, 300, hWnd, (HMENU)ListView, cs->hInstance, NULL);
	if (hList == NULL) return 0;
    dwStyle = ListView_GetExtendedListViewStyle(hList);
    dwStyle |= LVS_EX_FULLROWSELECT;
    ListView_SetExtendedListViewStyle(hList, dwStyle);

	lpfnMainProc = (WNDPROC)SetWindowLong(hList, GWL_WNDPROC,
	                                      (int)EventCall_ListView);
	//�^�C�g���ݒ�
	SetListTitle(hList, C_STAT ,  30, "�@"      , LVCFMT_LEFT);
	SetListTitle(hList, C_TITLE, 385, "�^�C�g��", LVCFMT_LEFT);
	SetListTitle(hList, C_LINK ,   0, "�����N��", LVCFMT_LEFT);
	SetListTitle(hList, C_ACT  ,   0, "�Ď��L��", LVCFMT_LEFT);
	SetListTitle(hList, C_INTVL,   0, "�Ď�����", LVCFMT_LEFT);
	SetListTitle(hList, C_STIME,   0, "�J�n����", LVCFMT_LEFT);
	SetListTitle(hList, C_GID  ,   0, "GID"     , LVCFMT_LEFT);
	SetListTitle(hList, C_PTN  ,   0, "�Ď�PTN" , LVCFMT_LEFT);

	// �t�H���g�ݒ�
	SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);

	// �X�e�[�^�X�o�[�̍쐬
	hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
	                         WS_CHILD|SBARS_SIZEGRIP|CCS_BOTTOM|WS_VISIBLE,
	                         0, 0, 0, 0, hWnd, (HMENU)StatusBar,
	                         cs->hInstance, NULL);
	if (hStatus != NULL) {
		SendMessage(hStatus, SB_SIMPLE, TRUE, 0L);
	}

	PostMessage(hWnd, WM_COMMAND, IDM_INIT_PARAM, 0);

	return 0;
}

LRESULT MainWnd_Timer(HWND hWnd, LONG id)
{
	HWND hList;
	int i, nMax, nStat;
	char szTime[MAX_BUF+1], szStat[MAX_BUF+1];
	char szAct[MAX_BUF+1], szPtn[MAX_BUF+1], szBuf[MAX_BUF+1];
	char *pFile;
	double dCurTime, dStime, dIntvl, dAccTime;

	// ���ݎ����̎擾
	GetTime(szTime);
	dCurTime = Date2Bin(szTime);

	hList = GetDlgItem(hMainWnd, ListView);
	nMax = ListView_GetItemCount(hList);
	for (i = 0; i < nMax; ++i) {
		ListView_GetItemText(hList, i, C_STAT, szStat, MAX_BUF);
		ListView_GetItemText(hList, i, C_ACT , szAct , MAX_BUF);
		if (!strcmp(szAct, "1") && strcmp(szStat, "��")) { // �Ď����� And ��ԁ��ω�����
			// �Ď��J�n�����ƊĎ������̎擾
			ListView_GetItemText(hList, i, C_STIME, szBuf, MAX_BUF);
			dStime = Date2Bin(szBuf);
			ListView_GetItemText(hList, i, C_INTVL, szBuf, MAX_BUF);
			dIntvl = Date2Bin(szBuf);

			if ((dStime + dIntvl) <= dCurTime) { // �Ď������̔���
				// �����N��擾
				pFile = (char *)malloc(MAX_FULL_PATH + 1);
				ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
				// �A�N�Z�X�����擾
				ListView_GetItemText(hList, i, C_PTN, szPtn, MAX_BUF);
				nStat = GetFileAccTime(pFile, szBuf, atoi(szPtn), pExcludeFile);
				if (nStat == TRUE) {
					// �����N��̕ω�����
					dAccTime = Date2Bin(szBuf);
					if (dStime < dAccTime) { // �A�N�Z�X�����ω��̔���
						// ���=�ω����� �֍X�V
						SetListData(hList, i, C_STAT, "��", 0);
						SetWinIcon(TRUE);
					} else {
						if (!strcmp(szStat, "�~")) { // ���=�A�N�Z�X�s��
							// ���=�Ď��Ώ� �֍X�V
							SetListData(hList, i, C_STAT, "��", 0);
						}

						// �J�n���������ݎ����ōX�V
						SetListData(hList, i, C_STIME, szTime, 0);
					}
				} else {
					// ���=�A�N�Z�X�s�� �֍X�V
					SetListData(hList, i, C_STAT, "�~", 0);
					// �J�n���������ݎ����ōX�V
					// �� �A�N�Z�X�s���̕ω��͔���ł��Ȃ��̂ōX�V���Ȃ�
					// SetListData(hList, i, C_STIME, szTime, 0);
				}
				free(pFile);
			}
		}
	}
	DspListStat(hWnd);
	SaveParamFile();

	return 0;
}

LRESULT MainWnd_Resize(HWND hWnd, LONG SizeType, WORD cx, WORD cy)
{
	HWND hList, hStatus;
	RECT rc;
	LONG width, height;
	int nOfset;
	static DWORD dwMajor = 0, dwMinor = 0;

	if (dwMajor == 0) {
		// Windows��OS�o�[�W�����擾
		GetWinVer(&dwMajor, &dwMinor);
	}

	/* �N���C�A���g�̈�̃T�C�Y���擾 */
	GetClientRect(hWnd, &rc);
	nOfset = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top - nOfset;

	// Windows10�̏ꍇ�̂ݕ␳
	if (dwMajor == 10) {
		height += 10;
	}

	// ListView�̃E�B���h�E�̑傫����ύX����
	hList = GetDlgItem(hWnd, ListView);
	MoveWindow(hList, 0, 0, width, height, 0);

	// ListView�̃J�������𒲐�����
	GetClientRect(hList, &rc);
	ListView_SetColumnWidth(hList, 0, 30);
	ListView_SetColumnWidth(hList, 1, (rc.right - rc.left) - 30);
	ListView_SetColumnWidth(hList, 2, 0);
	ListView_SetColumnWidth(hList, 3, 0);
	ListView_SetColumnWidth(hList, 4, 0);
	ListView_SetColumnWidth(hList, 5, 0);
	ListView_SetColumnWidth(hList, 6, 0);
	ListView_SetColumnWidth(hList, 7, 0);

	// �E�C���h�E�̍X�V
	SendMessage(hWnd, WM_ACTIVATE, 0, 0);

	// Status�o�[�̍X�V
	hStatus = GetDlgItem(hMainWnd, StatusBar);
	SendMessage(hStatus, WM_SIZE, 0, 0);

	return 0;
}

LRESULT MainWnd_Activate(HWND hWnd, WORD state, WORD minimized)
{
	HWND hList;

	// �E�C���h�E�̍X�V
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	// ���X�g�r���[�Ƀt�H�[�J�X
	hList = GetDlgItem(hMainWnd, ListView);
	SetFocus(hList);

	return 0;
}


LRESULT MainWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	HWND hList, hHelp;
	HINSTANCE hInstance;
	Variable Edit;
	char *pCmd, *pArg, *pFile, *pSrchStr, *pExcStr, szBuf[MAX_BUF+1];
	int nMax, nIdx, nAtr, nLen;
	int i, nStat, nStt, nEnd, nPos, nSelIdx;

	hList = GetDlgItem(hMainWnd, ListView);
	nIdx = ListView_GetSelectionMark(hList);

	switch (wID) {
		case IDM_APLI_NEW_D:
			SelectFolder();
			break;

		case IDM_APLI_NEW_F:
			SelectFile();
			break;

		case IDM_APLI_ICON:
			SetWinIcon(FALSE);
			break;

		case IDM_APLI_WATCH:
			if (g_nRegWithWatch == 1) {
				g_nRegWithWatch = 0;
			} else {
				g_nRegWithWatch = 1;
			}
			break;

		case IDM_APLI_HEAD:
			g_nCharSrchPtn = SRCH_HEAD;
			break;
		case IDM_APLI_INC:
			g_nCharSrchPtn = SRCH_INCLUDE;
			break;
		case IDM_APLI_NONE:
			g_nCharSrchPtn = SRCH_NONE;
			break;

		case IDM_APLI_OMIT:
			pExcStr = InputBox(hWnd, "�t�H���_�Ď��ł̏��O�t�@�C�������",
								"���O�t�@�C����", pExcludeFile);
			if (pExcStr) {
				if (pExcludeFile) free(pExcludeFile);
				pExcludeFile = strdup(pExcStr);
			}
			break;

		case IDM_APLI_FONT:
			if(ChooseFont(&cf) == TRUE) {
				// �t�H���g��ύX����
				if (hFont_MainWnd != NULL)	DeleteObject(hFont_MainWnd);
				hFont_MainWnd = CreateFontIndirect(cf.lpLogFont);

				SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);

				// ��ʂ��X�V����
				InvalidateRect(hMainWnd, NULL, TRUE);
				UpdateWindow(hMainWnd);
			}

			break;

		case IDM_APLI_EXPL:
			Event_MenuRButton(NULL, wID);
			break;

		case IDM_APLI_EXIT:
			SendMessage(hMainWnd, WM_CLOSE, 0, 0);
			break;

		case IDM_LIST_OPEN:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					pFile = (char *)malloc(MAX_FULL_PATH + 1);
					ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
					ExecFile(pFile);
					free(pFile);
					ResetListStat(hList, i);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_PATH:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					pFile = (char *)malloc(MAX_FULL_PATH + 1);
					ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
					nLen = GetPathLen(pFile);
					if (nLen > 0) pFile[nLen] = '\0';
					ExecFile(pFile);
					free(pFile);
					ResetListStat(hList, i);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_CMD:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					// �p�X���擾
					pFile = (char *)malloc(MAX_FULL_PATH + 1);
					ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
					nLen = GetPathLen(pFile);
					if (nLen > 0) pFile[nLen] = '\0';

					// �R�}���h�v�����v�g�̋N���p�����[�^�ݒ�
					pArg = (char *)malloc(MAX_FULL_PATH + 1);
					sprintf(pArg, "/Q /S /K \"pushd %s\"", pFile);

					// �R�}���h���s
					ShellExecute(NULL, NULL, "cmd", pArg, NULL, SW_SHOWNORMAL);

					free(pArg);
					free(pFile);
					ResetListStat(hList, i);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_ACT:
			if (nIdx >= 0) {
				ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
				if (!strcmp(szBuf, "1")) {
					SendMessage(hWnd, WM_COMMAND, IDM_LIST_OFF, 0);
				} else {
					SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
				}
			}
			break;

		case IDM_LIST_ON:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_STAT , "��" , 0);
					SetListData(hList, i, C_ACT  , "1"  , 0);
					GetTime(szBuf);
					SetListData(hList, i, C_STIME, szBuf, 0);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_OFF:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_STAT, ""  , 0);
					SetListData(hList, i, C_ACT , "0" , 0);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_9HOUR:
		case IDM_LIST_6HOUR:
		case IDM_LIST_3HOUR:
		case IDM_LIST_1HOUR:
		case IDM_LIST_30MIN:
		case IDM_LIST_5MIN:
		case IDM_LIST_1MIN:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				// IDM�l����ListView�ݒ蕶��������
				for (i = 0; IntvlInfo[i].pDspStr; ++i) {
					if (wID == IntvlInfo[i].nIdm) {
						nSelIdx = i;
						break;
					}
				}

				// ListView�ݒ蕶�������X�g�f�[�^�ɐݒ�
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_INTVL,
					            IntvlInfo[nSelIdx].pSetStr, 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_PTN0:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_PTN, "0", 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_PTN1:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_PTN, "1", 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_DETAIL:
			if (nIdx >= 0) {
				// �^�C�g���̃C���f���g�폜
				IndGrpTitle(hList, nIdx, FALSE);

				// �ڍאݒ�_�C�A���O
				hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
				DialogBox(hInstance, "IDD_DIALOG2",
						  hWnd, (DLGPROC)EventCall_RegWnd);

				// �^�C�g���̃C���f���g�ǉ�
				IndGrpTitle(hList, nIdx, TRUE);
			}
			break;

		case IDM_LIST_GROUP:
			GrpListData(hList, TRUE);
			break;

		case IDM_LIST_UNGROUP:
			GrpListData(hList, FALSE);
			break;

		case IDM_LIST_UP:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nStt; i <= nEnd; ++i) {
					nStat = MovupListData(hList, i, TRUE);
					if (nStat == FALSE) break;
				}
				ListView_EnsureVisible(hList, nStt - 1, FALSE);
			}
			break;

		case IDM_LIST_DOWN:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				for (i = nEnd; i >= nStt; --i) {
					nStat = MovupListData(hList, i, FALSE);
					if (nStat == FALSE) break;
				}
				ListView_EnsureVisible(hList, nEnd + 1, FALSE);
			}
			break;

		case IDM_LIST_SRCH:
			pSrchStr = InputBox(hWnd, "���X�g�������镶�������",
								"���X�g����", NULL);
			if (pSrchStr) {
				SrchListData(hList, (int)pSrchStr, SRCH_MATCH);
				return 1;
			}
			break;

		case IDM_LIST_DELETE:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // ���X�g�I������
				nPos = nStt;
				for (i = nStt; i <= nEnd; ++i) {
					nStat = DelListData(hList, nPos);
					if (nStat == FALSE) ++nPos;
				}
			}
			break;

		case IDM_HELP_OUTLINE:
		case IDM_HELP_METHOD:
		case IDM_HELP_NOMENU:
			Event_MenuRButton(NULL, wID);
			break;

		case IDM_HELP_VERSION:
			hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
			DialogBox(hInstance, "IDD_DIALOG1",
			          hWnd, (DLGPROC)EventCall_MsgWnd);
			break;

		case IDM_INIT_PARAM:
			// �p�����[�^�t�@�C���Ǎ���
			pCmd = GetCommandLine();
			if (pCmd != NULL) {
				nMax = GetElement(pCmd, 0, NULL);
				GetElement(NULL, 1, &pArg);
				LoadParamFile(pArg);
				SendMessage(hMainWnd, WM_TIMER, nTimerId, 0);
			}
			break;
	}
	return 0;
}

LRESULT MainWnd_MenuSelect(HWND hWnd)
{
	int i, nMax, nCnt, nIdx, nSelIdx;
	int nStat, nFnd, nStt, nEnd;
	char szBuf[MAX_BUF+1];
	HWND hList;
	HMENU hMenu;

	hMenu = GetMenu(hWnd);

	if (g_nRegWithWatch == 0) {
		CheckMenuItem(hMenu, IDM_APLI_WATCH, MF_BYCOMMAND|MF_UNCHECKED);
	} else {
		CheckMenuItem(hMenu, IDM_APLI_WATCH, MF_BYCOMMAND|MF_CHECKED);
	}

	CheckMenuRadioItem(hMenu, IDM_APLI_NONE, IDM_APLI_INC,
		               IDM_APLI_NONE + g_nCharSrchPtn, MF_BYCOMMAND);

	hList = GetDlgItem(hMainWnd, ListView);
	nIdx = ListView_GetSelectionMark(hList);
	if (nIdx >= 0) {
		nCnt = ListView_GetSelectedCount(hList);
		if (nCnt > 1) { // �������X�g�I��
			EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_GRAYED);
			EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_ENABLED);

			// �Ď��L��
			for (i = 0; ActInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, ActInfo[0].nIdm, ActInfo[i - 1].nIdm,
							   ActInfo[i].nIdm, MF_BYCOMMAND);

			// �Ď�����
			for (i = 0; IntvlInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
							   IntvlInfo[i].nIdm, MF_BYCOMMAND);

			// �t�H���_�Ď��p�^�[��
			for (i = 0; PtnInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
							   PtnInfo[i].nIdm, MF_BYCOMMAND);
		} else {	// �P�ꃊ�X�g�I��
			EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_ENABLED);
			EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_GRAYED);

			// �Ď��L��
			ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
			for (i = 0; ActInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, ActInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, ActInfo[0].nIdm, ActInfo[i - 1].nIdm,
							   ActInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		
			// �Ď�����
			ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
			for (i = 0; IntvlInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, IntvlInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
							   IntvlInfo[nSelIdx].nIdm, MF_BYCOMMAND);

			// �t�H���_�Ď��p�^�[��
			ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
			for (i = 0; PtnInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, PtnInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
							   PtnInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		}

		// �O���[�v�����̐ݒ�
		nFnd = FALSE;
		GetSelList(hList, &nStt, &nEnd);
		for (i = nStt; i <= nEnd; ++i) {
			ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
			// GID�ݒ肠��
			if (*szBuf) {
				nFnd = TRUE;
				break;
			}
		}
		if (nFnd == FALSE) { // GID�ݒ�Ȃ�
			EnableMenuItem(hMenu, IDM_LIST_UNGROUP, MF_GRAYED);
		} else {
			EnableMenuItem(hMenu, IDM_LIST_UNGROUP, MF_ENABLED);
		}

	}

	return 0;
}

int CALLBACK SortComp(LPARAM lParam1, LPARAM lParam2, LPARAM lCidx)
{
	HWND hList;
	LV_FINDINFO lvf;
	int nIdx1, nIdx2, nStat;
	char szBuf1[MAX_BUF+1], szBuf2[MAX_BUF+1];

	hList = GetDlgItem(hMainWnd, ListView);

	// ���X�g�̃C���f�b�N�X����
	lvf.flags = LVFI_PARAM;
	lvf.lParam = lParam1;
	nIdx1 = ListView_FindItem(hList, -1, &lvf);

	lvf.lParam = lParam2;
	nIdx2 = ListView_FindItem(hList, -1, &lvf);
	
	if ((int)lCidx == C_TITLE) { // �^�C�g����̃\�[�g
		// GID�擾
		ListView_GetItemText(hList, nIdx1, C_GID, szBuf1, MAX_BUF);
		ListView_GetItemText(hList, nIdx2, C_GID, szBuf2, MAX_BUF);
		// GID��r
		nStat = CompExcNull(szBuf1, szBuf2, nSortItem[(int)lCidx]);

		if (nStat != 0) return nStat;
	}

	// ���X�g�̕�����擾
	ListView_GetItemText(hList, nIdx1, (int)lCidx, szBuf1, MAX_BUF);
	ListView_GetItemText(hList, nIdx2, (int)lCidx, szBuf2, MAX_BUF);

	// ���X�g�̕������r
	return CompExcNull(szBuf1, szBuf2, nSortItem[(int)lCidx]);
}

// Null�����ɍs���悤�ȃ\�[�g�̔�r
int CompExcNull(char *pStr1, char *pStr2, int nSort)
{
	if (nSort == SORT_ASCEND) {
		if ((*pStr1 == '\0') && *pStr2) {
			return 1; // �󔒂�����
		} else if (*pStr1 && (*pStr2 == '\0')) {
			return -1; // �󔒂�����
		} else {
			return(strcmp(pStr1, pStr2));
		}
	} else {
		return(strcmp(pStr2, pStr1));
	}
}

//----------------------------------------------------------------------
//��ListView�̃��b�Z�[�W����
//----------------------------------------------------------------------
LRESULT CALLBACK EventCall_ListView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int nStat;

	switch(message) {
		case WM_COMMAND:
			return ListView_Command(hWnd, HIWORD(wParam), LOWORD(wParam),
			                        (HWND)lParam);
		case WM_DROPFILES:
			ListView_DropFiles(hWnd, (HDROP)wParam);
			break;

		case WM_KEYDOWN:
			nStat = ListView_KeyDown(hWnd, wParam);
			if (nStat) return 0;
			break;

		case WM_LBUTTONDOWN:
			nStat = ListView_LButtonDown(hWnd, wParam, lParam);
			if (nStat) return 0;
			break;

		case WM_LBUTTONUP:
			ListView_LButtonUp(hWnd, wParam, lParam);
			break;

		case WM_LBUTTONDBLCLK:
			return ListView_DoubleClick(hWnd);

		case WM_RBUTTONDOWN:
			return ListView_RButtonDown(hWnd, lParam);

		case WM_MENURBUTTONUP:
			Event_MenuRButton((HMENU)lParam, wParam);
			break;
	}

	return CallWindowProc(lpfnMainProc, hWnd, message, wParam, lParam);
}

LRESULT ListView_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	return MainWnd_Command(hMainWnd, wNotifyCode, wID, hWndCtl);
}

LRESULT ListView_DropFiles(HWND hWnd, HDROP hDrop)
{
	int i, nMax, nStat;
	char *pFile, *pFn;
	char szDrv[MAX_BUF+1], szNwDrv[MAX_BUF+1], *pSave;
	char szBuf[MAX_BUF+1];
   	DWORD nSize;
	HWND hEdit;
	Variable Files;

	// �h���b�v���ꂽ�t�@�C�������擾
	nMax = DragQueryFile(hDrop, -1, 0, 0);

	pFile = (char *)malloc(MAX_FULL_PATH + 1);
	if (pFile == NULL) return 0;
	for (i = 0; i < nMax; ++i) {
		// �h���b�v���ꂽ�t�@�C�������擾
		nStat = DragQueryFile(hDrop, i, pFile, MAX_FULL_PATH);
		pFile[nStat] = '\0';

		// �l�b�g���[�N�h���C�u���̓W�J
		if (*pFile) {
			if (pFile[1] == ':') {
				*szDrv = *pFile;
				strcpy(szDrv + 1, ":");
				*szNwDrv = '\0';
				nSize = MAX_BUF;
				WNetGetConnection(szDrv, szNwDrv, &nSize);
				if (*szNwDrv) {
					pSave = strdup(pFile + 2);
					strcpy(pFile, szNwDrv);
					strcat(pFile, pSave);
					free(pSave);
				}
			}
		}

		// �t�@�C������o�^����
		Files.add(pFile);
	}
	free(pFile);
	DragFinish(hDrop);

	// �t�@�C������ʂ֐ݒ�
	for (i = 0; i < Files.size(); ++i) {
		// ListView�ւ̃f�[�^�o�^
		pFile = Files.get(i);
		RegListData(pFile, i);
	}

	// �ǉ��f�[�^��I�����āA�X�e�[�^�X�o�[�֏������e�\��
	SelListData(hWnd, -1, FALSE);
	for (i = 0; i < Files.size(); ++i) {
		SelListData(hWnd, i, TRUE);
	}
	//SendMessage(hWnd, WM_LBUTTONDOWN, NULL, NULL);
	DspListStat(hWnd);

	// Window��O�ʂɕ\��
	SetForegroundWindow(hMainWnd);
	SendMessage(hWnd, WM_ACTIVATE, 0, 0);

	return 0;
}

LRESULT ListView_KeyDown(HWND hWnd, DWORD KeyCode)
{
	int nPos, nCtrl, nShift;
	static int bEchoFlag = 0;
	RECT rc;
	DWORD dwMajor, dwMinor;
	char *pSrchStr, szBuf[MAX_BUF], cCode;
	SCROLLINFO si;

	// Ctrl�L�[��Ԏ擾
	nCtrl = GetKeyState(VK_LCONTROL);
	nCtrl |= GetKeyState(VK_RCONTROL);
	// Shift�L�[��Ԏ擾
	nShift = GetKeyState(VK_LSHIFT);
	nShift |= GetKeyState(VK_RSHIFT);

	cCode = (char)KeyCode;
	switch (cCode) {
		case VK_UP:
		case VK_DOWN:
			if (bEchoFlag == 0) {
				bEchoFlag = 1;
				SendMessage(hWnd, WM_KEYDOWN, KeyCode, 0);
				DspListStat(hWnd);

				return 1;
			} else {
				bEchoFlag = 0;

				if(nCtrl & 0x80) {	// Ctrl�L�[����
					if ((char)KeyCode == VK_UP) {
						SendMessage(hWnd, WM_COMMAND, IDM_LIST_UP, 0);
					} else {
						SendMessage(hWnd, WM_COMMAND, IDM_LIST_DOWN, 0);
					}
					return 1;
				}
				return 0;
			}

		case VK_RETURN:
			if(nCtrl & 0x80) {	// Ctrl�L�[����
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_CMD, 0);
			} else if(nShift & 0x80) {	// Shift�L�[����
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_PATH, 0);
			} else {
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_OPEN, 0);
			}

			return 0;

		case VK_SPACE:
			memset(&si, 0, sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			GetScrollInfo(hWnd, SB_VERT, &si);
			if(nShift & 0x80) {	// Shift�L�[����
				ListView_Scroll(hWnd, 0, logfont.lfHeight * si.nPage);
			} else {
				ListView_Scroll(hWnd, 0, -1 * logfont.lfHeight * si.nPage);
			}

			return 0;

		case VK_F1:
			DebugLog(NULL);
			return 0;

		default:
			if(nCtrl & 0x80) {	// Ctrl�L�[����
				if (cCode == 'F') {
					SendMessage(hWnd, WM_COMMAND, IDM_LIST_SRCH, 0);
				}
			} else {
				if (('0' <= cCode && cCode <= '9')
				 || ('A' <= cCode && cCode <= 'Z')) {
					SrchListData(hWnd, (int)cCode, g_nCharSrchPtn);
					return 1;
				}
			}
	}

	return 0;
}

LRESULT ListView_LButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int nCtrl;
	static int bEchoFlag = 0;

	if (bEchoFlag == 0) {
		// Ctrl�L�[��Ԏ擾
		nCtrl = GetKeyState(VK_LCONTROL);
		nCtrl |= GetKeyState(VK_RCONTROL);

		if(nCtrl & 0x80) {	// Ctrl�L�[����
			// Ctrl�L�[�������������X�g�I���̗}�~
			return 1;
		} else {
			bEchoFlag = 1;
			SendMessage(hWnd, WM_LBUTTONDOWN, wParam, lParam);
			DspListStat(hWnd);
			return 1;
		}
	} else {
		bEchoFlag = 0;
		return 0;
	}
}

LRESULT ListView_LButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LRESULT ListView_DoubleClick(HWND hWnd)
{
	ListView_KeyDown(hWnd, VK_RETURN);
	return 0;
}

LRESULT ListView_RButtonDown(HWND hWnd, LPARAM lParam)
{
	HWND hList;
	HMENU hMenu, hIntvl, hPtn, hAct = NULL;
	POINT pt;
	int i, nIdx, nCnt, nMax, nSelIdx, nPos;
	int nFnd, nStat, nStt, nEnd;
	char szBuf[MAX_BUF+1], szCvt[MAX_BUF+1], szMsg[MAX_BUF+1];

	hList = GetDlgItem(hMainWnd, ListView);
	nIdx = ListView_GetSelectionMark(hList);
	if (nIdx < 0) return 0;

	nCnt = ListView_GetSelectedCount(hList);
	nMax = ListView_GetItemCount(hList);

	hMenu  = CreatePopupMenu();
	hIntvl = CreatePopupMenu();
	hPtn   = CreatePopupMenu();

	// �E�N���b�N���j���[�̐ݒ�
	nPos = 0;
	InsMenu(hMenu, nPos++, "�J��*\tEnter", IDM_LIST_OPEN, NULL);
	InsMenu(hMenu, nPos++, "�t�H���_���J��*\tShift+Enter", IDM_LIST_PATH, NULL);
	InsMenu(hMenu, nPos++, "�R�}���h�v�����v�g*\tCtrl+Enter", IDM_LIST_CMD, NULL);
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);

	// �u�Ď��L���v�̐ݒ�
	if (nCnt == 1) { // �P�ꃊ�X�g�I��
		ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
		for (i = 0; ActInfo[i].pDspStr; ++i) {
			if (!strcmp(szBuf, ActInfo[i].pSetStr))	nSelIdx = i;
		}
		sprintf(szMsg, "�ω��Ď��F%s", ActInfo[nSelIdx].pDspStr);
		InsMenu(hMenu, nPos++, szMsg, IDM_LIST_ACT, NULL);
		if (!strcmp(ActInfo[nSelIdx].pSetStr, "1")) {
			CheckMenuItem(hMenu, IDM_LIST_ACT, MF_BYCOMMAND|MF_CHECKED);
		} else {
			CheckMenuItem(hMenu, IDM_LIST_ACT, MF_BYCOMMAND|MF_UNCHECKED);
		}
	} else { // �������X�g�I��
		hAct = CreatePopupMenu();
		for (i = 0; ActInfo[i].pDspStr; ++i) {
			InsMenu(hAct, i, ActInfo[i].pDspStr, ActInfo[i].nIdm, NULL);
		}
		sprintf(szMsg, "�ω��Ď�");
		InsMenu(hMenu, nPos++, szMsg, NULL, hAct);
	}

	// �u�Ď������v�̐ݒ�
	ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
	for (i = 0; IntvlInfo[i].pDspStr; ++i) {
		InsMenu(hIntvl, i, IntvlInfo[i].pDspStr, IntvlInfo[i].nIdm, NULL);
		if (!strcmp(szBuf, IntvlInfo[i].pSetStr))	nSelIdx = i;
	}
	if (nCnt == 1) { // �P�ꃊ�X�g�I��
		CheckMenuRadioItem(hIntvl, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
			               IntvlInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		sprintf(szMsg, "�Ď������F%s", IntvlInfo[nSelIdx].pDspStr);
	} else { // �������X�g�I��
		sprintf(szMsg, "�Ď�����");
	}
	InsMenu(hMenu, nPos++, szMsg, NULL, hIntvl);

	// �u�t�H���_�Ď��p�^�[���v�̐ݒ�
	ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
	for (i = 0; PtnInfo[i].pDspStr; ++i) {
		sprintf(szCvt, "%s*", PtnInfo[i].pDspStr);
		InsMenu(hPtn, i, szCvt, PtnInfo[i].nIdm, NULL);
		if (!strcmp(szBuf, PtnInfo[i].pSetStr))	nSelIdx = i;
	}
	if (nCnt == 1) { // �P�ꃊ�X�g�I��
		CheckMenuRadioItem(hPtn, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
			               PtnInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		sprintf(szMsg, "�Ď��p�^�[���F%s", PtnInfo[nSelIdx].pDspStr);
	} else { // �������X�g�I��
		sprintf(szMsg, "�Ď��p�^�[��");
	}
	InsMenu(hMenu, nPos++, szMsg, NULL, hPtn);

	// �u�ڍאݒ�v�̐ݒ�
	InsMenu(hMenu, nPos++, "�ڍאݒ�...", IDM_LIST_DETAIL, NULL);
	if (nCnt > 1) { // �������X�g�I��
		EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_GRAYED);
	}

	// �u��/���ړ��v�̐ݒ�
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "��ֈړ�*\tCtrl+��", IDM_LIST_UP, NULL);
	InsMenu(hMenu, nPos++, "���ֈړ�*\tCtrl+��", IDM_LIST_DOWN, NULL);

	// �u���X�g�����v�̐ݒ�
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "���X�g����\tCtrl+F", IDM_LIST_SRCH, NULL);

	// �u�O���[�v���v�̐ݒ�
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "�O���[�v�ݒ�*", IDM_LIST_GROUP, NULL);
	if (nCnt == 1) { // �P�ꃊ�X�g�I��
		EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_GRAYED);
	}

	InsMenu(hMenu, nPos++, "�O���[�v����*", IDM_LIST_UNGROUP, NULL);
	nFnd = FALSE;
	nStat = GetSelList(hList, &nStt, &nEnd);
	if (nStat) { // ���X�g�I������
		for (i = nStt; i <= nEnd; ++i) {
			ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
			// GID�ݒ肠��
			if (*szBuf) {
				nFnd = TRUE;
				break;
			}
		}
	}
	if (nFnd == FALSE) { // GID�ݒ�Ȃ�
		EnableMenuItem(hMenu, IDM_LIST_UNGROUP, MF_GRAYED);
	}

	// �u�폜�v�̐ݒ�
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "�폜", IDM_LIST_DELETE, NULL);

	// �E�N���b�N���j���[�\��
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	ClientToScreen(hWnd, &pt);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

	if (hAct) DestroyMenu(hAct);
	DestroyMenu(hPtn);
	DestroyMenu(hIntvl);
	DestroyMenu(hMenu);

	return 0;
}

//----------------------------------------------------------------------
//�����j���[���ڂ̉E�N���b�N����
//----------------------------------------------------------------------
// Args:
// 1	HMENU  	���j���[�n���h��
// 2	int		�n���h��!=0:���j���[���ڂ̑��Έʒu
// 				�n���h��=0:���j���[���ڂ�ID
// Return:		TRUE:�������/FALSE:����Ȃ�
int Event_MenuRButton(HMENU hMenu, UINT uItem)
{
	int nStat, nDsp, nId;
	char *pMsg;
	MENUITEMINFO MenuInfo;

	if (hMenu) {
		// ���j���[���ڂ̑��Έʒu����ID�擾
		MenuInfo.cbSize = sizeof(MENUITEMINFO);
		MenuInfo.fMask  = MIIM_ID;
		nStat = GetMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &MenuInfo);
		nId = MenuInfo.wID;
	} else {
		nStat = TRUE;
		nId = uItem;
	}

	if (nStat) {
		nDsp = TRUE;
		switch(nId) {
			case IDM_APLI_ICON:
				 pMsg = "�y�A�C�R���́u�ω�����v�\���N���A�z\n�t�H�C����t�H���_�ɕω����L�����ۂ�\n�u�ω�����v�A�C�R��(���F)��\n�u�ω��Ȃ��v�A�C�R���֖߂��B";
				 break;
			case IDM_APLI_WATCH:
				 pMsg = "�y�V�K���X�g�o�^�̊Ď��L���z\n�t�H�C����t�H���_��\n�h���b�O���h���b�v�����ہA\n�u�Ď�����v�Ń��X�g�o�^���邩�A\n�u�Ď��Ȃ��v�Ń��X�g�o�^���邩\n��ؑւ���B";
				 break;
			case IDM_APLI_NONE:
			case IDM_APLI_HEAD:
			case IDM_APLI_INC:
				 pMsg = "�y1�������X�g�����z\n�p�����L�[�����������ہA\n��v���郊�X�g�փJ�[�\�����ړ�����B\n�E���X�g�̐擪1�����Ɉ�v\n�E���X�g����1�����Ɉ�v\n�E�J�[�\���ړ����Ȃ�";
				 break;
			case IDM_APLI_OMIT:
				 pMsg = "�y���O�t�@�C���z\n�t�H���_�Ď�����ہA�ω������m����K�v�̖����ꎞ�t�@�C�������w�肷��B\n���C���h�J�[�h(*,?)���g�p���A';'�ŋ�؂��ĕ����w�肷�邱�Ƃ��\�B";
				 break;
			case IDM_APLI_EXPL:
				 pMsg = "�y����z\n���j���[���ڂ��E�N���b�N�����\n���j���[���ڂ̉����\������B";
				 break;
			case IDM_LIST_OPEN:
				 pMsg = "�y�J���z\nEnter�L�[����/�_�u���N���b�N�ɂ��A\n���X�g�o�^�����t�@�C��/�t�H���_���J���B";
				 break;
			case IDM_LIST_PATH:
				 pMsg = "�y�t�H���_���J���z\nShift�L�[�������Ȃ���AEnter�L�[����/�_�u���N���b�N�ɂ��A���X�g�o�^�����t�@�C���̂���t�H���_���J���B\n���X�g�o�^���t�H���_�̏ꍇ�́A���̃t�H���_���J���B";
				 break;
			case IDM_LIST_CMD:
				 pMsg = "�y�R�}���h�v�����v�g�z\nCtrl�L�[�������Ȃ���AEnter�L�[����/�_�u���N���b�N�ɂ��A���X�g�o�^�����t�@�C���̂���t�H���_�ŁA�R�}���h�v�����v�g���J���B\n���X�g�o�^���t�H���_�̏ꍇ�́A���̃t�H���_�ŊJ���B";
				 break;
			case IDM_LIST_PTN0:
			case IDM_LIST_PTN1:

				 pMsg = "�y�Ď��p�^�[���z\n�t�H���_���Ď�����ۂ̊Ď��p�^�[����ݒ肷��B\n�E�t�@�C���̍X�V/�ǉ����Ď�\n�E�t�@�C���̍X�V/�ǉ�/�폜���Ď�\n\n��MS-Office�̓t�@�C�����J�������ňꎞ�t�@�C������������A�t�@�C���̍X�V�������Ă��t�H���_�̓������X�V����Ă��܂��B�s�v�Ȍ��m��h�~���邽�߂�2�̊Ď��p�^�[�����g��������B";
				 break;
			case IDM_LIST_UP:
			case IDM_LIST_DOWN:
				 pMsg = "�y��/���ւ̈ړ��z\n���X�g��I�����Ă����܂��͉��Ɉړ�����B�������X�g��I�����Ă��ړ��\�B";
				 break;
			case IDM_LIST_GROUP:
				 pMsg = "�y�O���[�v�ݒ�z\n���X�g�𕡐��I�����Ă���ݒ肷��B\n�O���[�v�ݒ肷��Ɛ擪���u�e���X�g�v�A\n���̑����u�q���X�g�v�ƂȂ�A�u�q���X�g�v�̓^�C�g�������������ĕ\�������B\n�O���[�v�ݒ肳�ꂽ���X�g�́A�\�[�g�����ꍇ�ł��A��������Ԃŕ\�������B";
				 break;
			case IDM_LIST_UNGROUP:
				 pMsg = "�y�O���[�v�����z\n�O���[�v�ݒ肵�����X�g��I�����Ă����������B\n�O���[�v�ݒ肵���ꕔ�݂̂̉������\�B";
				 break;
			case IDM_HELP_OUTLINE:
				 pMsg = "�y�T���z\n�t�@�C����t�H���_�̕ω����Ď�����B\n�E�h���b�O���h���b�v�Ń��X�g�o�^\n�E�Ď��L��/����/�p�^�[����ݒ�\n�E�Ď����\n  ��:�Ď��Ώ�\n  ��:�ω�����(�A�C�R�����ω�)\n  �~:�A�N�Z�X�s��";
				 break;
			case IDM_HELP_METHOD:
				 pMsg = "�y�Ď����@�z\n�X�V�����̕ω��ɂ�茟�m����B\n�X�V�����̓t�@�C���̍X�V�����łȂ��A\n�A�N�Z�X�����ɂ�蔻�肷��B\n(�ʃt�H���_����t�@�C���R�s�[�����\n �X�V�����͕ω������A�A�N�Z�X������\n �R�s�[���������ƂȂ��)\n\n�t�@�C���͂��ꎩ�̂̕ω��Ŕ��肷��B\n�t�H���_�͈ȉ�2�p�^�[���Ŕ��肷��B\n�E�t�H���_�z���̃t�@�C���̕ω�\n  �� �X�V+�ǉ������m\n�E�t�H���_�z���̃t�@�C���̕ω���\n  �t�H���_�̕ω�\n  �� �X�V+�ǉ�+�폜�����m\n\n���t�H���_�z���̃t�@�C���������ł́A\n���m�s�v�̈ꎞ�t�@�C���������O����B\n(MS-Office�t�@�C���͊J��������\n �ꎞ�t�@�C������������A\n �t�H���_�̍X�V�������X�V������)";
				 break;
			case IDM_HELP_NOMENU:
				 pMsg = "�y���j���[�ɖ�������z\n�E�p�����L�[\n  �� ���X�g��1��������\n      �擪1�����Ɉ�v or 1�������܂�\n      ���X�g�փJ�[�\���ړ�\n�ESpace\n  �� �y�[�WDOWN\n�EShift+Space\n  �� �y�[�WUP\n�E���X�g�̃^�C�g�����N���b�N\n  �� �N���b�N������Ń\�[�g\n      (�N���b�N���ɏ���/�~���ؑւ�)";
				 break;
			default:
				 nDsp = FALSE;
				 break;
		}

		if (nDsp) {
			MsgBox(hMainWnd, pMsg, "���", MB_OK);
		}
	} else {
		nDsp = FALSE;
	}

	return nDsp;
}

//----------------------------------------------------------------------
//�����ʏ���
//----------------------------------------------------------------------
// �X�e�[�^�X�\��
void DspStatus(char *pStr)
{
	HWND hStatus;
	char szBuf[MAX_BUF+1];

	sprintf(szBuf, "%s ", pStr);
	hStatus = GetDlgItem(hMainWnd, StatusBar);
	SendMessage(hStatus, SB_SETTEXT, 255 | 0, (WPARAM)szBuf);
	SendMessage(hStatus, WM_SIZE, 0, 0);
}

// CHOOSEFONT�̐ݒ�
int setcf(HWND hWnd, CHOOSEFONT *cf)
{
	cf->lStructSize = sizeof(CHOOSEFONT);
	cf->hwndOwner = hWnd;
	cf->lpLogFont = &logfont;
	cf->Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
	cf->rgbColors = RGB(0, 0, 0);
	cf->nFontType = SCREEN_FONTTYPE;

	return 0;
}

// ListView�ւ̃^�C�g���ݒ�
// Args:
// 1	HWND	ListView�n���h��
// 2	int		��ʒu(>=0)
// 3	int		��
// 4	char *	�ݒ蕶����
// 5	int		LVCFMT_LEFT:����, LVCFMT_RIGHT:�E��, LVCFMT_CENTER:����
// Return:		�Ȃ�
void SetListTitle(HWND hList, int nCidx, int nCx, char *pText, int nFmt)
{
	LV_COLUMN lvc;

	ZeroMemory(&lvc, sizeof(lvc));	//lvc�ϐ���Null�𖄂߂�

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = nFmt;

	lvc.cx = nCx;
	lvc.pszText = pText;
	lvc.iSubItem = nCidx;

	ListView_InsertColumn(hList, nCidx, &lvc);
}

// ListView�ւ̃f�[�^�ݒ�
// Args:
// 1	HWND	ListView�n���h��
// 2	int		�s�ʒu(>=0)
// 3	int		��ʒu(>=0) 0:�s�}��+�ݒ�/0>:�����s�ւ̐ݒ�
// 4	char *	�ݒ蕶����
// 5	int		0:�s�㏑/1:�s�}��
// 				�~(������/1:������+�C���[�W)
// Return:		�Ȃ�
void SetListData(HWND hList, int nRidx, int nCidx, char *pText, int bFlg)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(lvi));	//lvi�̕ϐ���Null�𖄂߂�

	lvi.mask     = LVIF_TEXT;
	lvi.pszText  = pText;
	lvi.iItem    = nRidx;
	lvi.iSubItem = nCidx;
	lvi.lParam   = nRidx;	// �\�[�g�����p�ɐݒ�

	if (bFlg == 1) {
		lvi.mask = lvi.mask | LVIF_PARAM;
		ListView_InsertItem(hList, &lvi);
	} else {
		ListView_SetItem(hList, &lvi);
	}
}

// ListView�ւ̃f�[�^�o�^
// Args:
// 1	char *	�o�^����t�@�C���p�X��
// 2	int		�o�^����s�ʒu(>=0)
// Return:		�Ȃ�
void RegListData(char *pFile, int nPos)
{
	HWND hList; 
	char *pFn, szBuf[MAX_BUF+1];
	Variable Edit;

	// ListView�̃n���h���擾
	hList = GetDlgItem(hMainWnd, ListView);

	// �t�@�C�����̎擾
	Edit.StrSplit(pFile, "\\", FALSE, FALSE);
	pFn = Edit.get(-1);

	// ���X�g��������Ԃœo�^
	GetTime(szBuf);
	if (g_nRegWithWatch == 0) {
		SetListData(hList, nPos, C_STAT , ""    , 1);
		SetListData(hList, nPos, C_ACT  , "0"   , 0);
	} else {
		SetListData(hList, nPos, C_STAT , "��"  , 1);
		SetListData(hList, nPos, C_ACT  , "1"   , 0);
	}
	SetListData(hList, nPos, C_TITLE, pFn   , 0);
	SetListData(hList, nPos, C_LINK , pFile , 0);
	SetListData(hList, nPos, C_INTVL, "1:00", 0);
	SetListData(hList, nPos, C_STIME, szBuf , 0);
	SetListData(hList, nPos, C_GID  , ""    , 0);
	SetListData(hList, nPos, C_PTN  , "0"   , 0);
}

// ListView�̃f�[�^�ړ�
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	���X�g�̃C���f�b�N�X
// 3	int		1:��ւ̈ړ�/0:���ւ̈ړ�
// Return:		1:�ړ����{/0:�ړ��͈͊O
int MovupListData(HWND hList, int nIdx, int bAct)
{
	int nMax, nStat = FALSE;
	int nOrgIdx, nDstIdx, nMask;
	char *pBuf;
	Variable Edit;

	nMax = ListView_GetItemCount(hList);

	if (bAct == TRUE) {
		// ��ֈړ��̓��ւ��ʒu�ݒ�
		if (nIdx > 0) {
			nStat = TRUE;
			nOrgIdx = nIdx;
			nDstIdx = nIdx - 1;
		}
	} else {
		// ���ֈړ��̓��ւ��ʒu�ݒ�
		if (nIdx < (nMax - 1)) {
			nStat = TRUE;
			nOrgIdx = nIdx;
			nDstIdx = nIdx + 1;
		}
	}

	if (nStat == TRUE) {
		// �ړ���f�[�^�̑ޔ�
		Edit.clear();
		pBuf = (char *)malloc(MAX_FULL_PATH + 1);
		ListView_GetItemText(hList, nDstIdx, C_STAT, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_TITLE, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_LINK, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_ACT, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_INTVL, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_STIME, pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_GID  , pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
		ListView_GetItemText(hList, nDstIdx, C_PTN  , pBuf, MAX_FULL_PATH);
		Edit.add(pBuf);
	
		// �ړ�������ړ���փf�[�^�ړ�
		ListView_GetItemText(hList, nOrgIdx, C_STAT, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_STAT, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_TITLE, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_TITLE, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_LINK, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_LINK, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_ACT, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_ACT, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_INTVL, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_INTVL, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_STIME, pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_STIME, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_GID  , pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_GID, pBuf, 0);
		ListView_GetItemText(hList, nOrgIdx, C_PTN  , pBuf, MAX_FULL_PATH);
		SetListData(hList, nDstIdx, C_PTN, pBuf, 0);

		// �ړ����֑ޔ��f�[�^�ړ�
		SetListData(hList, nOrgIdx, C_STAT , Edit.get(0), 0);
		SetListData(hList, nOrgIdx, C_TITLE, Edit.get(1), 0);
		SetListData(hList, nOrgIdx, C_LINK , Edit.get(2), 0);
		SetListData(hList, nOrgIdx, C_ACT  , Edit.get(3), 0);
		SetListData(hList, nOrgIdx, C_INTVL, Edit.get(4), 0);
		SetListData(hList, nOrgIdx, C_STIME, Edit.get(5), 0);
		SetListData(hList, nOrgIdx, C_GID  , Edit.get(6), 0);
		SetListData(hList, nOrgIdx, C_PTN  , Edit.get(7), 0);

		free(pBuf);

		// �I���ʒu�̏C��
		SelListData(hList, nOrgIdx, FALSE);
		SelListData(hList, nDstIdx, TRUE);
	}

	return nStat;
}

// ListView�̃f�[�^�O���[�v��
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	1:�O���[�v�ݒ�/0:�O���[�v����
// Return:		TRUE:����/FALSE:���s
int GrpListData(HWND hList, int bSet)
{
	int nIdx, nMax, nCnt, i, j, nSelCnt, nMjGid;
	int nStat, nRet = FALSE;
	char szGid[MAX_BUF+1], szTop[MAX_BUF+1];
	char *pDelGid;
	Variable Edit;

	// ListView�̃f�[�^��/�I����Ԑ��擾
	nMax = ListView_GetItemCount(hList);
	nCnt = ListView_GetSelectedCount(hList);

	if (bSet) {	// �O���[�v�ݒ�
		if (nCnt > 1) {
			nSelCnt = 0;
			for (i = 0; i < nMax; ++i) {
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if (nStat & LVIS_SELECTED) { // �I�����
					if (nSelCnt == 0) {	// 1�Ԗڂ̑I���f�[�^
						ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
						if (*szGid == '\0') { // ���ݒ�
							// �󂢂Ă����O���[�vID�Ő���
							nMjGid = GetMajorGid(hList);
							sprintf(szGid, "%02d", nMjGid);
						}
						// 1�Ԗڂ̃O���[�vID�ۑ�
						strcpy(szTop, szGid);
					} else {
						// 2�Ԗڈȍ~�̃O���[�vID����
						sprintf(szGid, "%s%02d", szTop, nSelCnt);
					}

					// �^�C�g���̃C���f���g�폜
					IndGrpTitle(hList, i, FALSE);
					// �O���[�vID�̍Đݒ�
					SetListData(hList, i, C_GID, szGid, 0);
					// �^�C�g���̃C���f���g�ǉ�
					IndGrpTitle(hList, i, TRUE);
					nRet = TRUE;

					++nSelCnt;
				}
			}
		}
	} else {	// �O���[�v����
		if (nCnt > 0) {
			nSelCnt = 0;
			for (i = 0; i < nMax; ++i) {
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if (nStat & LVIS_SELECTED) { // �I�����
					// �I�����ꂽ�����O���[�vID�ۑ�
					ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
					Edit.add(szGid);

					// �^�C�g���̃C���f���g�폜
					IndGrpTitle(hList, i, FALSE);
					// �O���[�vID����
					SetListData(hList, i, C_GID, "", 0);
				}
			}
			for (i = 0; i < nMax; ++i) {
				ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if ((nStat & LVIS_SELECTED) == 0) { // ��I�����
					// �폜����GID�Ɛ擪���������GID������
					ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
					for (j = 0; j < Edit.size(); ++j) {
						pDelGid = Edit.get(j);
						if (!strncmp(szGid, pDelGid, strlen(pDelGid))) {
							// �^�C�g���̃C���f���g�폜
							IndGrpTitle(hList, i, FALSE);
							// �O���[�vID����
							SetListData(hList, i, C_GID, "", 0);
							break;
						}
					}
				}
			}
			nRet = TRUE;
		}
	}

	return nRet;
}

// �󂢂Ă����O���[�vID�擾
int GetMajorGid(HWND hList)
{
	int nMax, i, nMjGid, nMaxId = 0;
	char szBuf[MAX_BUF+1], szUsed[300+1];

	*szUsed = '\0';
	nMax = ListView_GetItemCount(hList);
	for (i = 0; i < nMax; ++i) {
		ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
		if (strlen(szBuf) == 2) {
			// �ő�l�̔���
			nMjGid = atoi(szBuf);
			if (nMaxId < nMjGid) nMaxId = nMjGid;

			// �g�pID�ۑ�
			strcat(szUsed, szBuf);
			strcat(szUsed, ",");
		}
	}

	if (nMaxId < 99) { // �ő�l��99����
		// �ő�l+1��Ԃ�
		nMjGid = nMaxId + 1;
	} else {
		// �󂢂Ă����O���[�vID��T���Ă�Ԃ�
		nMjGid = 0;
		for (i = 1; i < 99; ++i) {
			sprintf(szBuf, "%02d", i);
			if (!strstr(szUsed, szBuf)) {
				nMjGid = i;
				break;
			}
		}
	}

	return nMjGid;
}

// �O���[�vID�ɉ������^�C�g���̃C���f���g�ݒ�
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	���X�g�̃C���f�b�N�X
// 3	int 	1:�C���f���g�ǉ�/0:�C���f���g�폜
// Return:		TRUE:����/FALSE:���s
int IndGrpTitle(HWND hList, int nIdx, int bAdd)
{
	int i, nCnt, nStat = FALSE;
	char szBuf[MAX_BUF+1], szTitle[MAX_BUF+1];
	char *pOrg, *pDst;

	// �O���[�vID�̌����擾
	ListView_GetItemText(hList, nIdx, C_GID, szBuf, MAX_BUF);
	nCnt = strlen(szBuf);
	if (nCnt > 2) { // 2�����傫��
		ListView_GetItemText(hList, nIdx, C_TITLE, szTitle, MAX_BUF);
		pOrg = szTitle;
		pDst = szBuf;
		for (i = 0; i < (nCnt - 2); ++i) { // ���O���[�vID�̌�����
			if (bAdd) { // �s���֋󔒂̒ǉ�
				*pDst = ' ';
				++pDst;
			} else { // �s������󔒂̍폜
				if (szTitle[i] != ' ') break;
				++pOrg;
			}
		}
		strcpy(pDst, pOrg);
		SetListData(hList, nIdx, C_TITLE, szBuf, 0);
		nStat = TRUE;
	}

	return nStat;
}

// ListView�̃f�[�^�폜
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	���X�g�̃C���f�b�N�X
// Return:		TRUE:����/FALSE:���s
int DelListData(HWND hList, int nIdx)
{
	int nMax, nStat, nCnt, nRet = FALSE;
	char szBuf[MAX_BUF+1];
	char szMsg[MAX_BUF+1];

	nMax = ListView_GetItemCount(hList);

	if (nIdx >= 0) {
		// �O���[�vID�̌����擾
		ListView_GetItemText(hList, nIdx, C_GID, szBuf, MAX_BUF);
		nCnt = strlen(szBuf);

		ListView_GetItemText(hList, nIdx, C_TITLE, szBuf, MAX_BUF);
		if (nCnt > 2) { // 2�����傫��
			// �s������󔒂̍폜
			strcpy(szBuf, szBuf + (nCnt - 2));
		}

		sprintf(szMsg, "�u%s�v���폜���܂����H", szBuf);
		nStat = MsgBox(hMainWnd, szMsg, "�m�F", MB_YESNO | MB_DEFBUTTON2);
		if (nStat == IDYES) {
			ListView_DeleteItem(hList, nIdx);
			nRet = TRUE;

			if (nMax > 1) {
				if (nIdx < (nMax - 1)) {
					SelListData(hList, nIdx, TRUE);
				} else {
					SelListData(hList, nIdx - 1, TRUE);
				}
			}
		}
	}

	return nRet;
}

// ListView�̃f�[�^�I��
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	0>:�I������f�[�^�̃C���f�b�N�X/-1:�S�C���f�b�N�X
// 3	int 	1:�I����Ԑݒ�/0:�I����ԉ���
// Return:		�Ȃ�
void SelListData(HWND hList, int nIdx, int bAct)
{
	int nMask, i, nStt, nEnd;

	if (nIdx == -1) { // �S�C���f�b�N�X�w��
		nStt = 0;
		nEnd = ListView_GetItemCount(hList) - 1;
	} else {  // �ʃC���f�b�N�X�w��
		nStt = nIdx;
		nEnd = nIdx;
	}

	nMask = LVIS_SELECTED | LVIS_FOCUSED;
	for (i = nStt; i <= nEnd; ++i) {
		if (bAct == TRUE) {
			ListView_SetSelectionMark(hList, i);
			ListView_SetItemState(hList, i, nMask, nMask);
			DspListStat(hList);
		} else {
			ListView_SetItemState(hList, i,     0, nMask);
		}
	}
}

// ListView�̃f�[�^����
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int 	1�����R�[�h(SRCH_HEAD/SRCH_INCLUDE�̏ꍇ)
// 				������̃|�C���^(SRCH_MATCH�̏ꍇ)
// 3	int 	SRCH_HEAD	:�擪1��������
// 				SRCH_INCLUDE:1�������܂ތ���
// 				SRCH_MATCH	:��������܂ތ���
// Return:		�Ȃ�
void SrchListData(HWND hList, int SrchParam, int nSrchPtn)
{
	int nIdx, nMax, nStt, nCnt, nFnd = FALSE;
	char szBuf[MAX_BUF+1], szSrch[MAX_BUF+1];
	char cCode, cChar, *pPos, *pStr;
	Variable Edit;

	if (g_nCharSrchPtn > 0) {
		nIdx = ListView_GetSelectionMark(hList);
		nMax = ListView_GetItemCount(hList);

		nStt = nIdx;
		for (nCnt = 1; nCnt < nMax; ++nCnt) {
			if (++nIdx >= nMax) nIdx = 0;
			ListView_GetItemText(hList, nIdx, C_TITLE, szBuf, MAX_BUF);

			if (nSrchPtn == SRCH_HEAD) { // �擪1�����Ɉ�v
				// �s���̋󔒍폜
				Edit.set(szBuf, 0);
				Edit.gsub(" +", "");
				pStr = Edit.get(0);
				// �擪1�����̈�v�𔻒�
				cCode = (char)SrchParam;
				cChar = *pStr;
				if ('A' <= cCode && cCode <= 'Z') {
					if (cCode == cChar || (cCode + 0x20) == cChar) nFnd = TRUE;
				} else {
					if (cCode == cChar) nFnd = TRUE;
				}
			} else if (nSrchPtn == SRCH_INCLUDE) { // 1�������܂�
				cCode = (char)SrchParam;
				szSrch[0] = cCode;
				szSrch[1] = '\0';
				pPos = StrChr(szBuf, szSrch);
				if (pPos) {
					nFnd = TRUE;
				} else if ('A' <= cCode && cCode <= 'Z') {
					// �������ōČ���
					szSrch[0] += 0x20;
					pPos = StrChr(szBuf, szSrch);
					if (pPos) nFnd = TRUE;
				}
			} else if (nSrchPtn == SRCH_MATCH) { // �����񌟍�
				pStr = (char *)SrchParam;
				pPos = strstr(szBuf, pStr);
				if (pPos) nFnd = TRUE;
			}

			if (nFnd) {
				SelListData(hList, nStt, FALSE);
				SelListData(hList, nIdx, TRUE);
				ListView_EnsureVisible(hList, nIdx, FALSE);
				DspStatus("Found.");
				break;
			} else {
				DspStatus("Not Found.");
			}
		}
	}
}

// ListView�f�[�^�̊Ď���Ԗ߂�
void ResetListStat(HWND hList, int nIdx)
{
	char szBuf[MAX_BUF+1];

	if (nIdx >= 0) {
		ListView_GetItemText(hList, nIdx, C_STAT, szBuf, MAX_BUF);
		if (!strcmp(szBuf, "��")) {
			SetListData(hList, nIdx, C_STAT , "��" , 0);
			GetTime(szBuf);
			SetListData(hList, nIdx, C_STIME, szBuf, 0);
			SetWinIcon(FALSE);
		}
	}
}

// �I��ListView�f�[�^�̏�ԕ\��
void DspListStat(HWND hList)
{
	int nIdx;
	char szBuf[MAX_BUF+1], szMsg[MAX_BUF+1];

	*szMsg = '\0';
	nIdx = ListView_GetSelectionMark(hList);
	if (nIdx >= 0) {
		ListView_GetItemText(hList, nIdx, C_STAT, szBuf, MAX_BUF);
		if (!strcmp(szBuf, "��")) {
			strcat(szMsg, "�Ď���");
			strcat(szMsg, "(");
			ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
			strcat(szMsg, szBuf);
			ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
			if (!strcmp(szBuf, "1")) {
				strcat(szMsg, ", �X�V+�ǉ�+�폜");
			} else {
				strcat(szMsg, ", �X�V+�ǉ�");
			}
			strcat(szMsg, ")");
		} else if (!strcmp(szBuf, "��")) {
			strcat(szMsg, "�ω�����");
			strcat(szMsg, "(");
			ListView_GetItemText(hList, nIdx, C_STIME, szBuf, MAX_BUF);
			strcat(szMsg, szBuf);
			strcat(szMsg, "�`)");
		} else if (!strcmp(szBuf, "�~")) {
			strcat(szMsg, "�A�N�Z�X�s��");
		} else {
			strcat(szMsg, "");
		}
	}
	DspStatus(szMsg);
}

// �I�����X�g�̍ŏ�/�ő�C���f�b�N�X�擾(�A�������I���݂̂̑O��)
// Args:
// 1	HWND 	���X�g�r���[�̃n���h��
// 2	int * 	�I�����ꂽ���X�g�̍ŏ��C���f�b�N�X(���ʏ�����)
// 3	int * 	�I�����ꂽ���X�g�̍ő�C���f�b�N�X(���ʏ�����)
// Return:		TRUE:�I�����ꂽ���X�g����AFALSE:�I�����ꂽ���X�g�Ȃ�
int GetSelList(HWND hList, int *lpStart, int *lpEnd)
{
	int i, nMax, nCnt, nStt, nEnd, nStat, nRet;

	// ListView�̃f�[�^��/�I����Ԑ��擾
	nMax = ListView_GetItemCount(hList);
	nCnt = ListView_GetSelectedCount(hList);

	if (nCnt >= 0) {
		nStt = -1;
		nEnd = -1;
		for (i = 0; i < nMax; ++i) {
			nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
			if (nStat & LVIS_SELECTED) { // �I�����
				if (nStt == -1) {
					// �ŏ�/�ő�C���f�b�N�X�̐ݒ�
					nStt = i;
					nEnd = i;
				} else {
					// �ő�C���f�b�N�X�̐ݒ�
					nEnd = i;
				}
			} else if (nStt != -1) { // �C���f�b�N�X�ݒ�ς�
				break;
			}
		}

		*lpStart = nStt;
		*lpEnd = nEnd;
		nRet = TRUE;
	} else {
		nRet = FALSE;
	}

	return nRet;
}

// ���j���[���ڂ̒ǉ�
void InsMenu(HMENU hMenu, int nPos, char *pTypeData, LONG wID, HMENU hSubMenu)
{
	MENUITEMINFO MenuInfo;

	MenuInfo.cbSize = sizeof(MENUITEMINFO);

	if (hSubMenu == NULL) {
		// �T�u���j���[�ȊO
		if (pTypeData == NULL) {
			MenuInfo.fMask    = MIIM_TYPE | MIIM_STATE;
		} else {
			MenuInfo.fMask    = MIIM_TYPE | MIIM_STATE | MIIM_ID;
			MenuInfo.wID      = wID;
		}
		MenuInfo.fState   = MFS_ENABLED;
	} else {
		// �T�u���j���[�̐ݒ�
		MenuInfo.fMask    = MIIM_TYPE | MIIM_SUBMENU;
		MenuInfo.hSubMenu = hSubMenu;
	}

	if (pTypeData == NULL) {
		// �Z�p���[�^�̐ݒ�
		MenuInfo.fType      = MFT_SEPARATOR;;
	} else {
		// ���j���[���ڂ̐ݒ�
		MenuInfo.fType      = MFT_STRING;;
		MenuInfo.dwTypeData = pTypeData;
		MenuInfo.cch        = strlen(MenuInfo.dwTypeData);
	}
	InsertMenuItem(hMenu, nPos, TRUE, &MenuInfo);
}

// �Ď��������Ԃ̑��ݕϊ�(h:mm �� h����/mm��)
void CvtDspTime(char *pOrgStr, char *pDstStr)
{
	Variable Edit;
	int nHour, nMin;

	Edit.split(pOrgStr, ":");
	if (Edit.size() == 2) {
		// h:mm �� h����/mm��
		nHour = atoi(Edit.get(0));
		nMin  = atoi(Edit.get(1));

		if (nHour == 0) {
			sprintf(pDstStr, "%d��", nMin);
		} else {
			if (nMin == 0) {
				sprintf(pDstStr, "%d����", nHour);
			} else {
				sprintf(pDstStr, "%d����%d��", nHour, nMin);
			}
		}
	} else {
		// h����/mm�� �� h:mm
		Edit.split(pOrgStr, "����");
		if (Edit.size() == 2) {
			nHour = atoi(Edit.get(0));
			Edit.gsub("��", "");
			nMin  = atoi(Edit.get(1));
		} else {
			nHour = 0;
			nMin  = atoi(Edit.get(0));
		}
		sprintf(pDstStr, "%d:%02d", nHour, nMin);
	}
}

// �t�@�C���̎��s
int ExecFile(char *pFile)
{
	HWND hEdit;
	int nStat, nLen, nLidx, nMax;
	char exec[MAX_PATH+1], *pParam;

	// �t�@�C�������_�u���N�H�[�g�Ŋ���
	nLen = strlen(pFile);
	pParam = (char *)malloc(nLen + 3);
	sprintf(pParam, "\"%s\"", pFile);

	/* �t�@�C�������擾 */
	nStat = GetFileAttributes(pFile);
	if (nStat != -1 && (nStat & FILE_ATTRIBUTE_DIRECTORY)) {
		/* �f�B���N�g���̏ꍇ�AExplorer�N�� */
		nStat = (int)ShellExecute(NULL, "explore", pParam,
		                          NULL, NULL, SW_SHOWNORMAL);
	} else {
		/* �֘A�t����ꂽ���s�t�@�C��������΋N�� */
		nStat = (int)FindExecutable(pParam, NULL, exec);
		if (nStat > 32) {
			nStat = (int)ShellExecute(NULL, NULL, exec, pParam,
			                          NULL, SW_SHOWNORMAL);
		} else if (!CmpStr(TailStr(pFile, 4), ".lnk") ||
		           !CmpStr(TailStr(pFile, 5), ".html") ||
		           !CmpStr(TailStr(pFile, 4), ".htm") ||
		           !memcmp(pFile, "http://", 7) ||
		           !memcmp(pFile, "https://", 8)) {
			nStat = (int)ShellExecute(NULL, NULL, pFile,
			                          NULL, NULL, SW_SHOWNORMAL);
		} else {
			nStat = -1;
		}
	}

	free(pParam);

	if (nStat > 32) {
		return 1;
	} else {
		return 0;
	}
}

// �p�����[�^�t�@�C���̎捞��
void LoadParamFile(char *pExcFile)
{
	HWND hList;
	int nIdx, nPosFlg = 0, nCnt = 0, bInsFlg;
	char szBuf[MAX_FULL_PATH + 1];
	FILE *fp;
	enum Pid {ID_NONE, ID_WINDOW, ID_OPTION, ID_LISTDATA};
	Pid nPid = ID_NONE;
	INT cx = 0, cy = 0, sx = 0, sy = 0;
	char *pFn;
	Variable Fn, Edit;
	
	// �p�����[�^�t�@�C�����̎擾
	Fn.add(pExcFile);
	Fn.gsub("^\"|\"$", "");
	pFn = Fn.get(0);

	Edit.split(pFn, "\\\\");
	Edit.set(PARAM_FILE, -1);
	pFn = Edit.join("\\");
	strcpy(ParamFile, pFn);
	free(pFn);

	hList = GetDlgItem(hMainWnd, ListView);

	// �p�����[�^�t�@�C���Ǎ���
	fp = fopen(ParamFile, "r");
	if (fp == NULL)	return;
	while (fgets(szBuf, MAX_FULL_PATH, fp) != NULL) {
		// ���s�폜
		SubStr(szBuf, "\n", "", 0);
		if (nPid == ID_NONE) {
			// �J�e�S���̔���
			if (!strcmp(szBuf, "[Window]")) {
				nPid = ID_WINDOW;
				nPosFlg = 1;
			} else if (!strcmp(szBuf, "[Option]")) {
				nPid = ID_OPTION;
			} else if (!strcmp(szBuf, "[ListData]")) {
				nPid = ID_LISTDATA;
			}
		} else if (nPid == ID_WINDOW) {
			// �E�B���h�E�ʒu/�T�C�Y�擾
			if (*szBuf == '\0') {
				nPid = ID_NONE;
			} else if ((nIdx = ChkParam(szBuf, "Left")) > 0) {
				sx = atoi(szBuf + nIdx);
			} else if ((nIdx = ChkParam(szBuf, "Top")) > 0) {
				sy = atoi(szBuf + nIdx);
			} else if ((nIdx = ChkParam(szBuf, "Width")) > 0) {
				cx = atoi(szBuf + nIdx);
			} else if ((nIdx = ChkParam(szBuf, "Height")) > 0) {
				cy = atoi(szBuf + nIdx);
			}
		} else if (nPid == ID_OPTION) {
			// �I�v�V�����擾
			if (*szBuf == '\0') {
				nPid = ID_NONE;
			} else if ((nIdx = ChkParam(szBuf, "RegWithWatch")) > 0) {
				if (atoi(szBuf + nIdx) == 1) {
					g_nRegWithWatch = 1;
				} else {
					g_nRegWithWatch = 0;
				}
			} else if ((nIdx = ChkParam(szBuf, "CharSrchPtn")) > 0) {
				g_nCharSrchPtn = atoi(szBuf + nIdx);
			} else if ((nIdx = ChkParam(szBuf, "ExcludeFile")) > 0) {
				if (pExcludeFile) free(pExcludeFile);
				pExcludeFile = strdup(szBuf + nIdx);
			} else if ((nIdx = ChkParam(szBuf, "FontInfo")) > 0) {
				Edit.clear();
				Edit.split(szBuf + nIdx, ",");
				logfont.lfHeight  = atoi(Edit.get(0));
				logfont.lfCharSet = atoi(Edit.get(1));
				strcpy(logfont.lfFaceName, Edit.get(2));

				// �t�H���g��ύX����
				if (hFont_MainWnd != NULL)	DeleteObject(hFont_MainWnd);
				hFont_MainWnd = CreateFontIndirect(cf.lpLogFont);
				SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);
			}
		} else if (nPid == ID_LISTDATA) {
			// ���X�g�f�[�^�擾
			if (*szBuf == '\0') {
				++nCnt;
			} else {
				Edit.clear();
				Edit.split(szBuf, ",", 2);
				nIdx = atoi(Edit.get(0));
				if (nIdx == C_STAT) {
					bInsFlg = 1;
				} else {
					bInsFlg = 0;
				}
				SetListData(hList, nCnt, nIdx, Edit.get(1), bInsFlg);
			}
		}
	}
	fclose(fp);

	// �E�B���h�E�ʒu/�T�C�Y�̕ύX
	if (nPosFlg == 1) {
		SetWindowPos(hMainWnd, HWND_TOP, sx, sy, cx, cy, 0);
	}

	return;
}

// �p�����[�^�����̔���
int ChkParam(char *pLine, char *pParam)
{
	int nLen;

	nLen = strlen(pParam);
	if (!strncmp(pLine, pParam, nLen) && pLine[nLen] == '=') {
		return nLen + 1;
	} else {
		return 0;
	}
}

// �p�����[�^�t�@�C���ւ̕ۑ�
void SaveParamFile()
{
	HWND hList;
	FILE *fp;
	int i, nMax;
	char *pFile, *pStr;
	char szBuf[MAX_BUF+1];
	RECT OwnWin;

	fp = fopen(ParamFile, "w");
	if (fp == NULL)	return;

	GetWindowRect(hMainWnd, &OwnWin);

	fprintf(fp, "[Window]\n");
	fprintf(fp, "Left=%d\n"  , OwnWin.left);
	fprintf(fp, "Top=%d\n"   , OwnWin.top);
	fprintf(fp, "Width=%d\n" , OwnWin.right - OwnWin.left);
	fprintf(fp, "Height=%d\n", OwnWin.bottom - OwnWin.top);
	fprintf(fp, "\n");

	fprintf(fp, "[Option]\n");
	fprintf(fp, "RegWithWatch=%d\n", g_nRegWithWatch);
	fprintf(fp, "CharSrchPtn=%d\n" , g_nCharSrchPtn);
	fprintf(fp, "ExcludeFile=%s\n", pExcludeFile);
	fprintf(fp, "FontInfo=%d,%d,%s\n",
				logfont.lfHeight, logfont.lfCharSet, logfont.lfFaceName);
	fprintf(fp, "\n");

	fprintf(fp, "[ListData]\n");
	hList = GetDlgItem(hMainWnd, ListView);
	nMax = ListView_GetItemCount(hList);
	for (i = 0; i < nMax; ++i) {
		ListView_GetItemText(hList, i, C_STAT, szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_STAT, szBuf);

		ListView_GetItemText(hList, i, C_TITLE, szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_TITLE, szBuf);

		pFile = (char *)malloc(MAX_FULL_PATH + 1);
		ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
		fprintf(fp, "%d,%s\n", C_LINK, pFile);
		free(pFile);

		ListView_GetItemText(hList, i, C_ACT, szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_ACT, szBuf);

		ListView_GetItemText(hList, i, C_INTVL, szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_INTVL, szBuf);

		ListView_GetItemText(hList, i, C_STIME, szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_STIME, szBuf);

		ListView_GetItemText(hList, i, C_GID  , szBuf, MAX_BUF);
		fprintf(fp, "%d,%s\n", C_GID, szBuf);

		ListView_GetItemText(hList, i, C_PTN  , szBuf, MAX_BUF);
		fprintf(fp, "%d,%d\n", C_PTN, atoi(szBuf));

		fprintf(fp, "\n");
	}

	fclose(fp);
	return;
}

// �t�@�C���I���ɂ��V�K���X�g�o�^
void SelectFile()
{
	OPENFILENAME ofn;
	char *pFiles, *pFd, *pFn, *pBuf;
	int nStat, nOfset;

	pFiles = (char *)malloc(MAX_FULL_PATH + 1);
	pFd    = (char *)malloc(MAX_FULL_PATH + 1);

	// �t�@�C�������擾 
	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(pFiles, MAX_FULL_PATH + 1);
	ofn.lStructSize    = sizeof(ofn);
	ofn.hwndOwner      = hMainWnd;
	ofn.lpstrFilter    = "���ׂẴt�@�C��(*.*)\0*\0\0";
	ofn.nFilterIndex   = 1;
	ofn.nMaxFile       = MAX_FULL_PATH;
	ofn.lpstrFile      = pFiles;
	//ofn.Flags          = OFN_ALLOWMULTISELECT + OFN_FILEMUSTEXIST
	//                     + OFN_LONGNAMES + OFN_EXPLORER;
	ofn.Flags          = OFN_FILEMUSTEXIST + OFN_LONGNAMES + OFN_EXPLORER;
	nStat = GetOpenFileName(&ofn);
	if (nStat == 0) return;

	// �t�@�C���������X�g�f�[�^�o�^
	nOfset = ofn.nFileOffset;
	while (pFiles[nOfset]) {
		pFn = pFiles + nOfset;
		if (pFiles[nOfset - 1]) {
		   pFiles[nOfset - 1] = '\0';
		}
		sprintf(pFd, "%s\\%s", pFiles, pFn);
		RegListData(pFd, 0);

		nOfset += (strlen(pFn) + 1);
	}

	free(pFiles);
	free(pFd);

	return;
}

// �t�H���_�I���ɂ��V�K���X�g�o�^
void SelectFolder()
{
	static char szDefDir[MAX_FULL_PATH + 1];
	char szDir[MAX_FULL_PATH + 1];

	if (*szDefDir == '\0') strcpy(szDefDir , "C:");

	if (GetDir(0, szDefDir, szDir) == TRUE) {
		RegListData(szDir, 0);
		strcpy(szDefDir, szDir);
	}
}

// �_�C�A���O�ɂ��t�H���_���I��
int GetDir(HWND hWnd, char *pDefDir, char *pPath)
{
	BROWSEINFO bInfo;
	LPITEMIDLIST pIDList;

	memset(&bInfo, 0, sizeof(bInfo));
	bInfo.hwndOwner = hWnd;				// �_�C�A���O�̐e�E�C���h�E�̃n���h��
	bInfo.pidlRoot  = NULL;				// ���[�g�t�H���_���f�X�N�g�b�v�t�H���_
	bInfo.pszDisplayName = pPath;		// �I�����ꂽ�t�H���_�����i�[
	bInfo.lpszTitle = "�t�H���_�̑I��"; // �c���[�r���[�̃^�C�g��������
	bInfo.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE;
	bInfo.lpfn      = EventCall_GetDir;	// �R�[���o�b�N�֐��̃A�h���X 
	bInfo.lParam    = (LPARAM)pDefDir;	// �R�[���o�b�N�֐��ɓn������

	pIDList = SHBrowseForFolder(&bInfo);
	if (pIDList == NULL){
		*pPath = '\0';

		return FALSE;	// �����I������Ȃ������ꍇ 
	} else {
		if (!SHGetPathFromIDList(pIDList, pPath))
			return FALSE;	// �ϊ��Ɏ��s 

		CoTaskMemFree(pIDList);	// pIDList�̃��������J�� 

		return TRUE;
	}
}

int __stdcall EventCall_GetDir(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	char szDir[MAX_FULL_PATH + 1];
	ITEMIDLIST *lpid;
	HWND hEdit;

	switch (uMsg) {
		case BFFM_INITIALIZED:	// �_�C�A���O�{�b�N�X��������
			// �R�����_�C�A���O�̏����f�B���N�g��
			SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
			break;

		case BFFM_VALIDATEFAILED:	// �����ȃt�H���_�[�������͂��ꂽ
			MessageBox(hWnd, (char *)lParam,
						"�����ȃt�H���_�[�������͂���܂���", MB_OK);
			// �G�f�B�b�g�{�b�N�X�̃n���h�����擾����
			hEdit = FindWindowEx(hWnd, NULL, "EDIT", NULL);
			SetWindowText(hEdit, "");
			return 1;	//  �_�C�A���O�{�b�N�X����Ȃ�

		case BFFM_SELCHANGED:	// �I���t�H���_�[���ω������ꍇ
			lpid = (ITEMIDLIST *)lParam;
			SHGetPathFromIDList(lpid, szDir);
			// �G�f�B�b�g�{�b�N�X�̃n���h�����擾����
			hEdit = FindWindowEx(hWnd, NULL, "EDIT", NULL);    
			SetWindowText(hEdit, szDir);
			break;
	}

	return 0;
}

// Windows��OS�o�[�W�����擾
int GetWinVer(DWORD *lpMajor, DWORD *lpMinor)
{
typedef void (WINAPI *RtlGetVersion_FUNC)(OSVERSIONINFOEX *);
	HMODULE hMod;
	RtlGetVersion_FUNC func;
	OSVERSIONINFO Ver;
	OSVERSIONINFOEX VerEx;

	hMod = LoadLibrary(TEXT("ntdll.dll"));
	if (hMod) {
		func = (RtlGetVersion_FUNC)GetProcAddress(hMod, "RtlGetVersion");
		if (func == 0) {
			FreeLibrary(hMod);
			return FALSE;
		}

		ZeroMemory(&VerEx, sizeof(VerEx));
		VerEx.dwOSVersionInfoSize = sizeof(VerEx);
		func(&VerEx);
		*lpMajor = VerEx.dwMajorVersion;
		*lpMinor = VerEx.dwMinorVersion;

		FreeLibrary(hMod);
	} else {
		VerEx.dwOSVersionInfoSize = sizeof(VerEx);
		if (GetVersionEx((OSVERSIONINFO *)&VerEx)) {
			*lpMajor = VerEx.dwMajorVersion;
			*lpMinor = VerEx.dwMinorVersion;
		} else {
			Ver.dwOSVersionInfoSize = sizeof(Ver);
			GetVersionEx(&Ver);
			*lpMajor = Ver.dwMajorVersion;
			*lpMinor = Ver.dwMinorVersion;
		}
	}

	return TRUE;
}

// ���ݎ����擾
// Args:
// 1	char *	���ݎ���������(YYYY/M/D h:mm:ss)������|�C���^
// Return:		�Ȃ�
void GetTime(char *pCurTime)
{
	time_t now = time(NULL);
	struct tm *pNow = localtime(&now);

	sprintf(pCurTime, "%d/%d/%d %d:%02d:%02d", pNow->tm_year + 1900, pNow->tm_mon + 1, pNow->tm_mday, pNow->tm_hour, pNow->tm_min, pNow->tm_sec);
}

// �t�@�C��/�t�H���_�̃A�N�Z�X�����擾
// Args:
// 1	char *	�t�@�C����/�t�H���_��
// 2	char *	�A�N�Z�X����������(YYYY/M/D h:mm:ss)������|�C���^
// 3	int 	�t�H���_�����擾�L��(0:�z���̃t�@�C���̂݁A1:�t�H���_���܂�)
// 4	char *	�A�N�Z�X�����擾�����O����t�@�C����(file1;file2;...�`��)
// Return:		TRUE:�擾�����AFALSE:�擾���s
int GetFileAccTime(char *pFile, char *pAccTime, int nPtn, char *pExclude)
{
	HANDLE hFile;
	FILETIME AccessTime, LastTime, LocalTime;
	SYSTEMTIME ft;
	WIN32_FIND_DATA wfd;
	LONG nAtr, nStat, nDif, nMatch;
	char *pFd, *pExcReg;
	Variable Edit;
//	LONG nDebug = FALSE;

	nAtr = GetFileAttributes(pFile);
	if (nAtr == -1) { // �t�@�C���Ȃ�
		return FALSE;
	} else if(nAtr & FILE_ATTRIBUTE_ARCHIVE) { // �t�@�C��
		hFile = FindFirstFile(pFile, &wfd);
		if (hFile == INVALID_HANDLE_VALUE) { // Open�G���[
			return FALSE;
		}

		AccessTime = wfd.ftLastAccessTime;
		GetFileTime(hFile,  NULL, &AccessTime, NULL);
		//CloseHandle(hFile);
		FindClose(hFile);

		FileTimeToLocalFileTime(&AccessTime , &LocalTime);
	} else if(nAtr & FILE_ATTRIBUTE_DIRECTORY) { // �t�H���_
		if (nPtn == 1) {
			// �t�H���_�̃A�N�Z�X�����擾
			hFile = FindFirstFile(pFile, &wfd);
			FindClose(hFile);
			LastTime = wfd.ftLastAccessTime;
		} else {
			ZeroMemory(&LastTime, sizeof(FILETIME));
		}

		// �A�N�Z�X�����擾�̏��O�t�@�C�����̐��K�\���쐬
		if (pExclude) {
			Edit.clear();
			Edit.add(pExclude);
			// ";"��؂�̃��C���h�J�[�h�`�����琳�K�\���֕ϊ�
			Edit.gsub(";", "|");
			Edit.gsub("\\.", "\\.");
			Edit.gsub("?", ".");
			Edit.gsub("\\*", ".*");
			Edit.ins("^(", 0);
			Edit.add(")$");
			pExcReg = Edit.join("");
		} else {
			pExcReg = NULL;
		}

		// �t�H���_�z���̃t�@�C���ōŐV�A�N�Z�X�����擾
		pFd = (char *)malloc(MAX_FULL_PATH + 1);
		sprintf(pFd, "%s\\*", pFile);
		hFile = FindFirstFile(pFd, &wfd);
		if (hFile == INVALID_HANDLE_VALUE) {
			nStat = FALSE;
		} else {
			nStat = TRUE;
		} 

		while (nStat) {
			if (wfd.dwFileAttributes  & FILE_ATTRIBUTE_ARCHIVE) {
				if (pExcReg) {
					// ���O�t�@�C�����̈�v����
					Edit.match(wfd.cFileName, pExcReg);
					if (Edit.size() > 0) {
						nMatch = TRUE;
					} else {
						nMatch = FALSE;
					}
				} else {
					nMatch = FALSE;
				}

				if (nMatch == FALSE) {
					AccessTime = wfd.ftLastAccessTime;
					nDif = CmpFileTime(AccessTime, LastTime);
					if (nDif > 0) {
						LastTime = AccessTime;
					}
				}
			}

			nStat = FindNextFile(hFile, &wfd);
		}
		FindClose(hFile);
		free(pFd);
		if (pExcReg) free(pExcReg);

		FileTimeToLocalFileTime(&LastTime, &LocalTime);
	}
	FileTimeToSystemTime(&LocalTime , &ft);
	sprintf(pAccTime, "%d/%d/%d %d:%02d:%02d",
			ft.wYear, ft.wMonth, ft.wDay, ft.wHour, ft.wMinute, ft.wSecond);

	return TRUE;
}

// FILETIME�̔�r
int CmpFileTime(FILETIME Time1, FILETIME Time2)
{
	int nStat;

	nStat = Time1.dwHighDateTime -  Time2.dwHighDateTime;
	if (nStat == 0) {
		nStat = Time1.dwLowDateTime -  Time2.dwLowDateTime;
	}

	return nStat;
}

// �t���p�X���ɂ�����t�H���_���̒����擾
// Args:
// 1	char *	�t�@�C����(�t���p�X)
// Return:		0:���s�A0>:�t�H���_���̒���
int GetPathLen(char *pFile)
{
	int nAtr, nLen = 0;
	Variable Edit;

	nAtr = GetFileAttributes(pFile);
	if (nAtr == -1) { // �t�@�C���Ȃ�
		;
	} else if(nAtr & FILE_ATTRIBUTE_ARCHIVE) { // �t�@�C��
		Edit.StrSplit(pFile, "\\", FALSE, FALSE);
		nLen = strlen(pFile) - strlen(Edit.get(-1)) - 1;
	} else if(nAtr & FILE_ATTRIBUTE_DIRECTORY) { // �t�H���_
		nLen = strlen(pFile);
	}

	return nLen;
}

// �A�C�R���̐ݒ�
// Args:
// 1	int 	TRUE:�ω�����AFALSE:�W��
// Return:      �Ȃ�
void SetWinIcon(int bFnd)
{
	HINSTANCE hInstance;
	HICON hIcon;

	hInstance = (HINSTANCE)GetWindowLong(hMainWnd, GWL_HINSTANCE);
	if (bFnd) {
		hIcon = LoadIcon(hInstance, "IDR_ICON2");
	} else {
		hIcon = LoadIcon(hInstance, "IDR_ICON");
	}

	SendMessage(hMainWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}
	
// �f�o�b�O�p���O�̕ۑ�/�\��
// Args:
// 1	char *	���O������:�ۑ��ANULL:�\��
// Return:      �Ȃ�
void DebugLog(char *pLog)
{
	static Variable LogList;
	char *pMsg, szBuf[MAX_BUF];

	if (pLog) {
		GetTime(szBuf);
		strcat(szBuf, ": ");
		strcat(szBuf, pLog);
		LogList.add(szBuf);
	} else {
		pMsg = LogList.join("\n");
		LogList.clear();
		MsgBox(hMainWnd, pMsg, "���O", MB_OK);
		free(pMsg);
	}
}
