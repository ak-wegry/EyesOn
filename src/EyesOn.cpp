/*
 * ファイル変化監視ツール
 *======================================================================
 *[変更履歴]
 * Ver0.00  2020/04/16 作成開始
 * Ver1.00  2020/07/16 新規作成
 * Ver1.01  2020/07/20 フォルダ名からファイル名抽出の不具合修正
 *                     メニューの「開く」実行時、状態変更の不具合修正
 *                     フォルダ選択による新規リスト登録の追加
 * Ver1.02  2020/08/19 メニューから新規ファイル/フォルダ登録削除
 * Ver1.03  2020/09/29 リスト削除時のタイトルのインデント表示修正
 *                     状態遷移「アクセス不可⇒監視対象」の不具合修正
 *                     リスト選択時の「アクセス不可」状態表示追加
 * Ver1.04  2021/02/02 監視周期に3/6/9時間を追加
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

// 固定値 
#define MAX_BUF			1024
#define PARAM_FILE		"EyesOn.ini"
#define KEYPARAM		"RESTART"	// ショートカットからの起動を回避する再起動
#define WAKEUP_TIMER	300000	// タイマー5分
//#define WAKEUP_TIMER	60000	// タイマー1分(デバッグ用)
#define SORT_ASCEND		1		// 昇順ソート
#define SORT_DESCEND	2		// 降順ソート
#define SRCH_NONE		0		// 検索なし
#define SRCH_HEAD		1		// 先頭1文字検索
#define SRCH_INCLUDE	2		// 1文字含む検索
#define SRCH_MATCH		3		// 文字列検索
#define FONT_HEIGHT		19		// フォントの高さ

// グローバル変数
HWND hMainWnd; 
int g_nRegWithWatch = 0;// 登録時の監視状態(0:監視なし、1:監視あり)
int g_nCharSrchPtn = 0;	// 1文字のリスト検索(0:なし、1:先頭1文字、2:1文字を含む)

POPMENU_INFO ActInfo[] = {
	{ "あり"          , "1"   , IDM_LIST_ON    },
	{ "なし"          , "0"   , IDM_LIST_OFF   },
	{ NULL            , NULL  , 0              }};

POPMENU_INFO IntvlInfo[] = {
	{ "9時間"         , "9:00", IDM_LIST_9HOUR },
	{ "6時間"         , "6:00", IDM_LIST_6HOUR },
	{ "3時間"         , "3:00", IDM_LIST_3HOUR },
	{ "1時間"         , "1:00", IDM_LIST_1HOUR },
	{ "30分"          , "0:30", IDM_LIST_30MIN },
	{ "5分"           , "0:05", IDM_LIST_5MIN  },
//	{ "1分"           , "0:01", IDM_LIST_1MIN  },
	{ NULL            , NULL  , 0              }};

POPMENU_INFO PtnInfo[] = {
	{ "更新+追加"     , "0"   , IDM_LIST_PTN0 },
	{ "更新+追加+削除", "1"   , IDM_LIST_PTN1 },
	{ NULL            , NULL  , 0             }};

char szDebug[MAX_BUF+1];		// デバッグ表示用

// ファイルローカル変数
static HFONT hFont_MainWnd;			// フォント
static WNDPROC lpfnMainProc;		// CallBack関数
static char ParamFile[MAX_PATH+1];	// パラメータファイル名
static int nTimerId = 1;			// タイマーID
static int nSortItem[NUM_ITEM];		// 各ラムのソート順
static CHOOSEFONT cf;				// フォント設定ダイアログの情報
static LOGFONT logfont;				// フォント情報
static char *pExcludeFile = NULL;	// アクセス日時取得時の除外ファイル

void DebugLog(char *pLog);

//----------------------------------------------------------------------
//■メインの関数 
//----------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    HWND hWnd;		// メインウインドウのハンドル
    WNDCLASS wc;	// WNDCLASS構造体
	MSG msg;
	int nStat, nRestart = TRUE;
	char *pCmd;
	Variable Param;

	// 「ショートカットからの起動」を回避する再起動
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

	// WNDCLASS構造体を0で初期化
	ZeroMemory(&wc,sizeof(WNDCLASS));
	// WNDCLASS構造体の設定
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= EventCall_MainWnd;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	//wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIcon			= LoadIcon(hInstance, "IDR_ICON");
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName		= MAKEINTRESOURCE(IDR_MENU);	// メニューを付ける
	wc.lpszClassName	= "EyesOn";
	// ウィンドウ クラスを登録
	if (RegisterClass(&wc) == 0)
		return 0; 

	// メインウインドウの生成
	hWnd = CreateWindowEx(
	           WS_EX_ACCEPTFILES | WS_EX_CONTROLPARENT | WS_EX_WINDOWEDGE,
	           wc.lpszClassName, "EyesOn",
	           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
	           450, 300, NULL, NULL, hInstance, NULL); 
	if (hWnd == NULL)
		return 0;
	hMainWnd = hWnd;
 
    // ウインドウの表示
    ShowWindow(hWnd, nCmdShow);

	// フォント設定ダイアログの設定
	logfont.lfHeight = -1 * FONT_HEIGHT;
	logfont.lfCharSet = SHIFTJIS_CHARSET;
	strcpy(logfont.lfFaceName, "ＭＳ ゴシック");
	setcf(hWnd, &cf);
 
	// アクセス日時取得時の除外ファイル名設定
	pExcludeFile = strdup("~*;.*");

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg); 
		DispatchMessage(&msg);   
	} 
	return (msg.wParam);
}

//----------------------------------------------------------------------
//■メインウインドウのメッセージ処理
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
			// ウィンドウをアクティブにし、元の位置とサイズで表示
			ShowWindow(hWnd, SW_SHOWNORMAL);
			// breakなし
		default:
			return(DefWindowProc(hWnd, uMsg, wParam, lParam)); 
	}
	return 0;
}

LRESULT MainWnd_Create(HWND hWnd, LPCREATESTRUCT cs)
{
	HWND hList, hStatus;
	DWORD dwStyle;

	// フォントオブジェクトの作成
	hFont_MainWnd = CreateFont(-1 * FONT_HEIGHT, 0, 0, 0, 400, 0, 0, 0,
							   128, 3, 2, 1, 49, "ＭＳ ゴシック");

	// リストビューの作成
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
	//タイトル設定
	SetListTitle(hList, C_STAT ,  30, "　"      , LVCFMT_LEFT);
	SetListTitle(hList, C_TITLE, 385, "タイトル", LVCFMT_LEFT);
	SetListTitle(hList, C_LINK ,   0, "リンク先", LVCFMT_LEFT);
	SetListTitle(hList, C_ACT  ,   0, "監視有無", LVCFMT_LEFT);
	SetListTitle(hList, C_INTVL,   0, "監視周期", LVCFMT_LEFT);
	SetListTitle(hList, C_STIME,   0, "開始時刻", LVCFMT_LEFT);
	SetListTitle(hList, C_GID  ,   0, "GID"     , LVCFMT_LEFT);
	SetListTitle(hList, C_PTN  ,   0, "監視PTN" , LVCFMT_LEFT);

	// フォント設定
	SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);

	// ステータスバーの作成
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

	// 現在時刻の取得
	GetTime(szTime);
	dCurTime = Date2Bin(szTime);

	hList = GetDlgItem(hMainWnd, ListView);
	nMax = ListView_GetItemCount(hList);
	for (i = 0; i < nMax; ++i) {
		ListView_GetItemText(hList, i, C_STAT, szStat, MAX_BUF);
		ListView_GetItemText(hList, i, C_ACT , szAct , MAX_BUF);
		if (!strcmp(szAct, "1") && strcmp(szStat, "●")) { // 監視あり And 状態≠変化あり
			// 監視開始時刻と監視周期の取得
			ListView_GetItemText(hList, i, C_STIME, szBuf, MAX_BUF);
			dStime = Date2Bin(szBuf);
			ListView_GetItemText(hList, i, C_INTVL, szBuf, MAX_BUF);
			dIntvl = Date2Bin(szBuf);

			if ((dStime + dIntvl) <= dCurTime) { // 監視周期の判定
				// リンク先取得
				pFile = (char *)malloc(MAX_FULL_PATH + 1);
				ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
				// アクセス時刻取得
				ListView_GetItemText(hList, i, C_PTN, szPtn, MAX_BUF);
				nStat = GetFileAccTime(pFile, szBuf, atoi(szPtn), pExcludeFile);
				if (nStat == TRUE) {
					// リンク先の変化判定
					dAccTime = Date2Bin(szBuf);
					if (dStime < dAccTime) { // アクセス時刻変化の判定
						// 状態=変化あり へ更新
						SetListData(hList, i, C_STAT, "●", 0);
						SetWinIcon(TRUE);
					} else {
						if (!strcmp(szStat, "×")) { // 状態=アクセス不可
							// 状態=監視対象 へ更新
							SetListData(hList, i, C_STAT, "◎", 0);
						}

						// 開始時刻を現在時刻で更新
						SetListData(hList, i, C_STIME, szTime, 0);
					}
				} else {
					// 状態=アクセス不可 へ更新
					SetListData(hList, i, C_STAT, "×", 0);
					// 開始時刻を現在時刻で更新
					// ⇒ アクセス不可中の変化は判定できないので更新しない
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
		// WindowsのOSバージョン取得
		GetWinVer(&dwMajor, &dwMinor);
	}

	/* クライアント領域のサイズを取得 */
	GetClientRect(hWnd, &rc);
	nOfset = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top - nOfset;

	// Windows10の場合のみ補正
	if (dwMajor == 10) {
		height += 10;
	}

	// ListViewのウィンドウの大きさを変更する
	hList = GetDlgItem(hWnd, ListView);
	MoveWindow(hList, 0, 0, width, height, 0);

	// ListViewのカラム幅を調整する
	GetClientRect(hList, &rc);
	ListView_SetColumnWidth(hList, 0, 30);
	ListView_SetColumnWidth(hList, 1, (rc.right - rc.left) - 30);
	ListView_SetColumnWidth(hList, 2, 0);
	ListView_SetColumnWidth(hList, 3, 0);
	ListView_SetColumnWidth(hList, 4, 0);
	ListView_SetColumnWidth(hList, 5, 0);
	ListView_SetColumnWidth(hList, 6, 0);
	ListView_SetColumnWidth(hList, 7, 0);

	// ウインドウの更新
	SendMessage(hWnd, WM_ACTIVATE, 0, 0);

	// Statusバーの更新
	hStatus = GetDlgItem(hMainWnd, StatusBar);
	SendMessage(hStatus, WM_SIZE, 0, 0);

	return 0;
}

LRESULT MainWnd_Activate(HWND hWnd, WORD state, WORD minimized)
{
	HWND hList;

	// ウインドウの更新
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	// リストビューにフォーカス
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
			pExcStr = InputBox(hWnd, "フォルダ監視での除外ファイルを入力",
								"除外ファイル名", pExcludeFile);
			if (pExcStr) {
				if (pExcludeFile) free(pExcludeFile);
				pExcludeFile = strdup(pExcStr);
			}
			break;

		case IDM_APLI_FONT:
			if(ChooseFont(&cf) == TRUE) {
				// フォントを変更する
				if (hFont_MainWnd != NULL)	DeleteObject(hFont_MainWnd);
				hFont_MainWnd = CreateFontIndirect(cf.lpLogFont);

				SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);

				// 画面を更新する
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
			if (nStat) { // リスト選択あり
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
			if (nStat) { // リスト選択あり
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
			if (nStat) { // リスト選択あり
				for (i = nStt; i <= nEnd; ++i) {
					// パス名取得
					pFile = (char *)malloc(MAX_FULL_PATH + 1);
					ListView_GetItemText(hList, i, C_LINK, pFile, MAX_FULL_PATH);
					nLen = GetPathLen(pFile);
					if (nLen > 0) pFile[nLen] = '\0';

					// コマンドプロンプトの起動パラメータ設定
					pArg = (char *)malloc(MAX_FULL_PATH + 1);
					sprintf(pArg, "/Q /S /K \"pushd %s\"", pFile);

					// コマンド実行
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
			if (nStat) { // リスト選択あり
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_STAT , "◎" , 0);
					SetListData(hList, i, C_ACT  , "1"  , 0);
					GetTime(szBuf);
					SetListData(hList, i, C_STIME, szBuf, 0);
				}
			}
			DspListStat(hWnd);
			break;

		case IDM_LIST_OFF:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // リスト選択あり
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
			if (nStat) { // リスト選択あり
				// IDM値からListView設定文字を検索
				for (i = 0; IntvlInfo[i].pDspStr; ++i) {
					if (wID == IntvlInfo[i].nIdm) {
						nSelIdx = i;
						break;
					}
				}

				// ListView設定文字をリストデータに設定
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_INTVL,
					            IntvlInfo[nSelIdx].pSetStr, 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_PTN0:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // リスト選択あり
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_PTN, "0", 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_PTN1:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // リスト選択あり
				for (i = nStt; i <= nEnd; ++i) {
					SetListData(hList, i, C_PTN, "1", 0);
				}
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_ON, 0);
			}
			break;

		case IDM_LIST_DETAIL:
			if (nIdx >= 0) {
				// タイトルのインデント削除
				IndGrpTitle(hList, nIdx, FALSE);

				// 詳細設定ダイアログ
				hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
				DialogBox(hInstance, "IDD_DIALOG2",
						  hWnd, (DLGPROC)EventCall_RegWnd);

				// タイトルのインデント追加
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
			if (nStat) { // リスト選択あり
				for (i = nStt; i <= nEnd; ++i) {
					nStat = MovupListData(hList, i, TRUE);
					if (nStat == FALSE) break;
				}
				ListView_EnsureVisible(hList, nStt - 1, FALSE);
			}
			break;

		case IDM_LIST_DOWN:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // リスト選択あり
				for (i = nEnd; i >= nStt; --i) {
					nStat = MovupListData(hList, i, FALSE);
					if (nStat == FALSE) break;
				}
				ListView_EnsureVisible(hList, nEnd + 1, FALSE);
			}
			break;

		case IDM_LIST_SRCH:
			pSrchStr = InputBox(hWnd, "リスト検索する文字を入力",
								"リスト検索", NULL);
			if (pSrchStr) {
				SrchListData(hList, (int)pSrchStr, SRCH_MATCH);
				return 1;
			}
			break;

		case IDM_LIST_DELETE:
			nStat = GetSelList(hList, &nStt, &nEnd);
			if (nStat) { // リスト選択あり
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
			// パラメータファイル読込み
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
		if (nCnt > 1) { // 複数リスト選択
			EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_GRAYED);
			EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_ENABLED);

			// 監視有無
			for (i = 0; ActInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, ActInfo[0].nIdm, ActInfo[i - 1].nIdm,
							   ActInfo[i].nIdm, MF_BYCOMMAND);

			// 監視周期
			for (i = 0; IntvlInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
							   IntvlInfo[i].nIdm, MF_BYCOMMAND);

			// フォルダ監視パターン
			for (i = 0; PtnInfo[i].pDspStr; ++i) ;
			CheckMenuRadioItem(hMenu, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
							   PtnInfo[i].nIdm, MF_BYCOMMAND);
		} else {	// 単一リスト選択
			EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_ENABLED);
			EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_GRAYED);

			// 監視有無
			ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
			for (i = 0; ActInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, ActInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, ActInfo[0].nIdm, ActInfo[i - 1].nIdm,
							   ActInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		
			// 監視周期
			ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
			for (i = 0; IntvlInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, IntvlInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
							   IntvlInfo[nSelIdx].nIdm, MF_BYCOMMAND);

			// フォルダ監視パターン
			ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
			for (i = 0; PtnInfo[i].pDspStr; ++i) {
				if (!strcmp(szBuf, PtnInfo[i].pSetStr))	nSelIdx = i;
			}
			CheckMenuRadioItem(hMenu, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
							   PtnInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		}

		// グループ解除の設定
		nFnd = FALSE;
		GetSelList(hList, &nStt, &nEnd);
		for (i = nStt; i <= nEnd; ++i) {
			ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
			// GID設定あり
			if (*szBuf) {
				nFnd = TRUE;
				break;
			}
		}
		if (nFnd == FALSE) { // GID設定なし
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

	// リストのインデックス検索
	lvf.flags = LVFI_PARAM;
	lvf.lParam = lParam1;
	nIdx1 = ListView_FindItem(hList, -1, &lvf);

	lvf.lParam = lParam2;
	nIdx2 = ListView_FindItem(hList, -1, &lvf);
	
	if ((int)lCidx == C_TITLE) { // タイトル列のソート
		// GID取得
		ListView_GetItemText(hList, nIdx1, C_GID, szBuf1, MAX_BUF);
		ListView_GetItemText(hList, nIdx2, C_GID, szBuf2, MAX_BUF);
		// GID比較
		nStat = CompExcNull(szBuf1, szBuf2, nSortItem[(int)lCidx]);

		if (nStat != 0) return nStat;
	}

	// リストの文字列取得
	ListView_GetItemText(hList, nIdx1, (int)lCidx, szBuf1, MAX_BUF);
	ListView_GetItemText(hList, nIdx2, (int)lCidx, szBuf2, MAX_BUF);

	// リストの文字列比較
	return CompExcNull(szBuf1, szBuf2, nSortItem[(int)lCidx]);
}

// Nullが後ろに行くようなソートの比較
int CompExcNull(char *pStr1, char *pStr2, int nSort)
{
	if (nSort == SORT_ASCEND) {
		if ((*pStr1 == '\0') && *pStr2) {
			return 1; // 空白を後ろへ
		} else if (*pStr1 && (*pStr2 == '\0')) {
			return -1; // 空白を後ろへ
		} else {
			return(strcmp(pStr1, pStr2));
		}
	} else {
		return(strcmp(pStr2, pStr1));
	}
}

//----------------------------------------------------------------------
//■ListViewのメッセージ処理
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

	// ドロップされたファイル数を取得
	nMax = DragQueryFile(hDrop, -1, 0, 0);

	pFile = (char *)malloc(MAX_FULL_PATH + 1);
	if (pFile == NULL) return 0;
	for (i = 0; i < nMax; ++i) {
		// ドロップされたファイル名を取得
		nStat = DragQueryFile(hDrop, i, pFile, MAX_FULL_PATH);
		pFile[nStat] = '\0';

		// ネットワークドライブ名の展開
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

		// ファイル名を登録する
		Files.add(pFile);
	}
	free(pFile);
	DragFinish(hDrop);

	// ファイルを画面へ設定
	for (i = 0; i < Files.size(); ++i) {
		// ListViewへのデータ登録
		pFile = Files.get(i);
		RegListData(pFile, i);
	}

	// 追加データを選択して、ステータスバーへ処理内容表示
	SelListData(hWnd, -1, FALSE);
	for (i = 0; i < Files.size(); ++i) {
		SelListData(hWnd, i, TRUE);
	}
	//SendMessage(hWnd, WM_LBUTTONDOWN, NULL, NULL);
	DspListStat(hWnd);

	// Windowを前面に表示
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

	// Ctrlキー状態取得
	nCtrl = GetKeyState(VK_LCONTROL);
	nCtrl |= GetKeyState(VK_RCONTROL);
	// Shiftキー状態取得
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

				if(nCtrl & 0x80) {	// Ctrlキー押下
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
			if(nCtrl & 0x80) {	// Ctrlキー押下
				SendMessage(hWnd, WM_COMMAND, IDM_LIST_CMD, 0);
			} else if(nShift & 0x80) {	// Shiftキー押下
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
			if(nShift & 0x80) {	// Shiftキー押下
				ListView_Scroll(hWnd, 0, logfont.lfHeight * si.nPage);
			} else {
				ListView_Scroll(hWnd, 0, -1 * logfont.lfHeight * si.nPage);
			}

			return 0;

		case VK_F1:
			DebugLog(NULL);
			return 0;

		default:
			if(nCtrl & 0x80) {	// Ctrlキー押下
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
		// Ctrlキー状態取得
		nCtrl = GetKeyState(VK_LCONTROL);
		nCtrl |= GetKeyState(VK_RCONTROL);

		if(nCtrl & 0x80) {	// Ctrlキー押下
			// Ctrlキーを押下したリスト選択の抑止
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

	// 右クリックメニューの設定
	nPos = 0;
	InsMenu(hMenu, nPos++, "開く*\tEnter", IDM_LIST_OPEN, NULL);
	InsMenu(hMenu, nPos++, "フォルダを開く*\tShift+Enter", IDM_LIST_PATH, NULL);
	InsMenu(hMenu, nPos++, "コマンドプロンプト*\tCtrl+Enter", IDM_LIST_CMD, NULL);
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);

	// 「監視有無」の設定
	if (nCnt == 1) { // 単一リスト選択
		ListView_GetItemText(hList, nIdx, C_ACT, szBuf, MAX_BUF);
		for (i = 0; ActInfo[i].pDspStr; ++i) {
			if (!strcmp(szBuf, ActInfo[i].pSetStr))	nSelIdx = i;
		}
		sprintf(szMsg, "変化監視：%s", ActInfo[nSelIdx].pDspStr);
		InsMenu(hMenu, nPos++, szMsg, IDM_LIST_ACT, NULL);
		if (!strcmp(ActInfo[nSelIdx].pSetStr, "1")) {
			CheckMenuItem(hMenu, IDM_LIST_ACT, MF_BYCOMMAND|MF_CHECKED);
		} else {
			CheckMenuItem(hMenu, IDM_LIST_ACT, MF_BYCOMMAND|MF_UNCHECKED);
		}
	} else { // 複数リスト選択
		hAct = CreatePopupMenu();
		for (i = 0; ActInfo[i].pDspStr; ++i) {
			InsMenu(hAct, i, ActInfo[i].pDspStr, ActInfo[i].nIdm, NULL);
		}
		sprintf(szMsg, "変化監視");
		InsMenu(hMenu, nPos++, szMsg, NULL, hAct);
	}

	// 「監視周期」の設定
	ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
	for (i = 0; IntvlInfo[i].pDspStr; ++i) {
		InsMenu(hIntvl, i, IntvlInfo[i].pDspStr, IntvlInfo[i].nIdm, NULL);
		if (!strcmp(szBuf, IntvlInfo[i].pSetStr))	nSelIdx = i;
	}
	if (nCnt == 1) { // 単一リスト選択
		CheckMenuRadioItem(hIntvl, IntvlInfo[0].nIdm, IntvlInfo[i - 1].nIdm,
			               IntvlInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		sprintf(szMsg, "監視周期：%s", IntvlInfo[nSelIdx].pDspStr);
	} else { // 複数リスト選択
		sprintf(szMsg, "監視周期");
	}
	InsMenu(hMenu, nPos++, szMsg, NULL, hIntvl);

	// 「フォルダ監視パターン」の設定
	ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
	for (i = 0; PtnInfo[i].pDspStr; ++i) {
		sprintf(szCvt, "%s*", PtnInfo[i].pDspStr);
		InsMenu(hPtn, i, szCvt, PtnInfo[i].nIdm, NULL);
		if (!strcmp(szBuf, PtnInfo[i].pSetStr))	nSelIdx = i;
	}
	if (nCnt == 1) { // 単一リスト選択
		CheckMenuRadioItem(hPtn, PtnInfo[0].nIdm, PtnInfo[i - 1].nIdm,
			               PtnInfo[nSelIdx].nIdm, MF_BYCOMMAND);
		sprintf(szMsg, "監視パターン：%s", PtnInfo[nSelIdx].pDspStr);
	} else { // 複数リスト選択
		sprintf(szMsg, "監視パターン");
	}
	InsMenu(hMenu, nPos++, szMsg, NULL, hPtn);

	// 「詳細設定」の設定
	InsMenu(hMenu, nPos++, "詳細設定...", IDM_LIST_DETAIL, NULL);
	if (nCnt > 1) { // 複数リスト選択
		EnableMenuItem(hMenu, IDM_LIST_DETAIL, MF_GRAYED);
	}

	// 「上/下移動」の設定
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "上へ移動*\tCtrl+↑", IDM_LIST_UP, NULL);
	InsMenu(hMenu, nPos++, "下へ移動*\tCtrl+↓", IDM_LIST_DOWN, NULL);

	// 「リスト検索」の設定
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "リスト検索\tCtrl+F", IDM_LIST_SRCH, NULL);

	// 「グループ化」の設定
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "グループ設定*", IDM_LIST_GROUP, NULL);
	if (nCnt == 1) { // 単一リスト選択
		EnableMenuItem(hMenu, IDM_LIST_GROUP, MF_GRAYED);
	}

	InsMenu(hMenu, nPos++, "グループ解除*", IDM_LIST_UNGROUP, NULL);
	nFnd = FALSE;
	nStat = GetSelList(hList, &nStt, &nEnd);
	if (nStat) { // リスト選択あり
		for (i = nStt; i <= nEnd; ++i) {
			ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
			// GID設定あり
			if (*szBuf) {
				nFnd = TRUE;
				break;
			}
		}
	}
	if (nFnd == FALSE) { // GID設定なし
		EnableMenuItem(hMenu, IDM_LIST_UNGROUP, MF_GRAYED);
	}

	// 「削除」の設定
	InsMenu(hMenu, nPos++, NULL, NULL, NULL);
	InsMenu(hMenu, nPos++, "削除", IDM_LIST_DELETE, NULL);

	// 右クリックメニュー表示
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
//■メニュー項目の右クリック処理
//----------------------------------------------------------------------
// Args:
// 1	HMENU  	メニューハンドル
// 2	int		ハンドル!=0:メニュー項目の相対位置
// 				ハンドル=0:メニュー項目のID
// Return:		TRUE:解説あり/FALSE:解説なし
int Event_MenuRButton(HMENU hMenu, UINT uItem)
{
	int nStat, nDsp, nId;
	char *pMsg;
	MENUITEMINFO MenuInfo;

	if (hMenu) {
		// メニュー項目の相対位置からID取得
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
				 pMsg = "【アイコンの「変化あり」表示クリア】\nフォイルやフォルダに変化が有った際の\n「変化あり」アイコン(黄色)を\n「変化なし」アイコンへ戻す。";
				 break;
			case IDM_APLI_WATCH:
				 pMsg = "【新規リスト登録の監視有無】\nフォイルやフォルダを\nドラッグ＆ドロップした際、\n「監視あり」でリスト登録するか、\n「監視なし」でリスト登録するか\nを切替える。";
				 break;
			case IDM_APLI_NONE:
			case IDM_APLI_HEAD:
			case IDM_APLI_INC:
				 pMsg = "【1文字リスト検索】\n英数字キーを押下した際、\n一致するリストへカーソルを移動する。\n・リストの先頭1文字に一致\n・リスト内の1文字に一致\n・カーソル移動しない";
				 break;
			case IDM_APLI_OMIT:
				 pMsg = "【除外ファイル】\nフォルダ監視する際、変化を検知する必要の無い一時ファイル等を指定する。\nワイルドカード(*,?)を使用し、';'で区切って複数指定することが可能。";
				 break;
			case IDM_APLI_EXPL:
				 pMsg = "【解説】\nメニュー項目を右クリックすると\nメニュー項目の解説を表示する。";
				 break;
			case IDM_LIST_OPEN:
				 pMsg = "【開く】\nEnterキー押下/ダブルクリックにより、\nリスト登録したファイル/フォルダを開く。";
				 break;
			case IDM_LIST_PATH:
				 pMsg = "【フォルダを開く】\nShiftキーを押しながら、Enterキー押下/ダブルクリックにより、リスト登録したファイルのあるフォルダを開く。\nリスト登録がフォルダの場合は、そのフォルダを開く。";
				 break;
			case IDM_LIST_CMD:
				 pMsg = "【コマンドプロンプト】\nCtrlキーを押しながら、Enterキー押下/ダブルクリックにより、リスト登録したファイルのあるフォルダで、コマンドプロンプトを開く。\nリスト登録がフォルダの場合は、そのフォルダで開く。";
				 break;
			case IDM_LIST_PTN0:
			case IDM_LIST_PTN1:

				 pMsg = "【監視パターン】\nフォルダを監視する際の監視パターンを設定する。\n・ファイルの更新/追加を監視\n・ファイルの更新/追加/削除を監視\n\n※MS-Officeはファイルを開くだけで一時ファイルが生成され、ファイルの更新が無くてもフォルダの日時が更新されてしまう。不要な検知を防止するために2つの監視パターンを使い分ける。";
				 break;
			case IDM_LIST_UP:
			case IDM_LIST_DOWN:
				 pMsg = "【上/下への移動】\nリストを選択してから上または下に移動する。複数リストを選択しても移動可能。";
				 break;
			case IDM_LIST_GROUP:
				 pMsg = "【グループ設定】\nリストを複数選択してから設定する。\nグループ設定すると先頭が「親リスト」、\nその他が「子リスト」となり、「子リスト」はタイトルが字下げして表示される。\nグループ設定されたリストは、ソートした場合でも連続した状態で表示される。";
				 break;
			case IDM_LIST_UNGROUP:
				 pMsg = "【グループ解除】\nグループ設定したリストを選択してから解除する。\nグループ設定した一部のみの解除も可能。";
				 break;
			case IDM_HELP_OUTLINE:
				 pMsg = "【概略】\nファイルやフォルダの変化を監視する。\n・ドラッグ＆ドロップでリスト登録\n・監視有無/周期/パターンを設定\n・監視状態\n  ◎:監視対象\n  ●:変化あり(アイコンも変化)\n  ×:アクセス不可";
				 break;
			case IDM_HELP_METHOD:
				 pMsg = "【監視方法】\n更新時刻の変化により検知する。\n更新時刻はファイルの更新日時でなく、\nアクセス日時により判定する。\n(別フォルダからファイルコピーすると\n 更新日時は変化せず、アクセス日時が\n コピーした時刻となる為)\n\nファイルはそれ自体の変化で判定する。\nフォルダは以下2パターンで判定する。\n・フォルダ配下のファイルの変化\n  ⇒ 更新+追加を検知\n・フォルダ配下のファイルの変化と\n  フォルダの変化\n  ⇒ 更新+追加+削除を検知\n\n※フォルダ配下のファイルを検索では、\n検知不要の一時ファイル等を除外する。\n(MS-Officeファイルは開くだけで\n 一時ファイルが生成され、\n フォルダの更新時刻が更新される為)";
				 break;
			case IDM_HELP_NOMENU:
				 pMsg = "【メニューに無い操作】\n・英数字キー\n  ⇒ リストの1文字検索\n      先頭1文字に一致 or 1文字を含む\n      リストへカーソル移動\n・Space\n  ⇒ ページDOWN\n・Shift+Space\n  ⇒ ページUP\n・リストのタイトルをクリック\n  ⇒ クリックした列でソート\n      (クリック毎に昇順/降順切替え)";
				 break;
			default:
				 nDsp = FALSE;
				 break;
		}

		if (nDsp) {
			MsgBox(hMainWnd, pMsg, "解説", MB_OK);
		}
	} else {
		nDsp = FALSE;
	}

	return nDsp;
}

//----------------------------------------------------------------------
//■共通処理
//----------------------------------------------------------------------
// ステータス表示
void DspStatus(char *pStr)
{
	HWND hStatus;
	char szBuf[MAX_BUF+1];

	sprintf(szBuf, "%s ", pStr);
	hStatus = GetDlgItem(hMainWnd, StatusBar);
	SendMessage(hStatus, SB_SETTEXT, 255 | 0, (WPARAM)szBuf);
	SendMessage(hStatus, WM_SIZE, 0, 0);
}

// CHOOSEFONTの設定
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

// ListViewへのタイトル設定
// Args:
// 1	HWND	ListViewハンドル
// 2	int		列位置(>=0)
// 3	int		列幅
// 4	char *	設定文字列
// 5	int		LVCFMT_LEFT:左寄せ, LVCFMT_RIGHT:右寄せ, LVCFMT_CENTER:中央
// Return:		なし
void SetListTitle(HWND hList, int nCidx, int nCx, char *pText, int nFmt)
{
	LV_COLUMN lvc;

	ZeroMemory(&lvc, sizeof(lvc));	//lvc変数にNullを埋める

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = nFmt;

	lvc.cx = nCx;
	lvc.pszText = pText;
	lvc.iSubItem = nCidx;

	ListView_InsertColumn(hList, nCidx, &lvc);
}

// ListViewへのデータ設定
// Args:
// 1	HWND	ListViewハンドル
// 2	int		行位置(>=0)
// 3	int		列位置(>=0) 0:行挿入+設定/0>:既存行への設定
// 4	char *	設定文字列
// 5	int		0:行上書/1:行挿入
// 				×(文字列/1:文字列+イメージ)
// Return:		なし
void SetListData(HWND hList, int nRidx, int nCidx, char *pText, int bFlg)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(lvi));	//lviの変数にNullを埋める

	lvi.mask     = LVIF_TEXT;
	lvi.pszText  = pText;
	lvi.iItem    = nRidx;
	lvi.iSubItem = nCidx;
	lvi.lParam   = nRidx;	// ソート検索用に設定

	if (bFlg == 1) {
		lvi.mask = lvi.mask | LVIF_PARAM;
		ListView_InsertItem(hList, &lvi);
	} else {
		ListView_SetItem(hList, &lvi);
	}
}

// ListViewへのデータ登録
// Args:
// 1	char *	登録するファイルパス名
// 2	int		登録する行位置(>=0)
// Return:		なし
void RegListData(char *pFile, int nPos)
{
	HWND hList; 
	char *pFn, szBuf[MAX_BUF+1];
	Variable Edit;

	// ListViewのハンドル取得
	hList = GetDlgItem(hMainWnd, ListView);

	// ファイル名の取得
	Edit.StrSplit(pFile, "\\", FALSE, FALSE);
	pFn = Edit.get(-1);

	// リストを初期状態で登録
	GetTime(szBuf);
	if (g_nRegWithWatch == 0) {
		SetListData(hList, nPos, C_STAT , ""    , 1);
		SetListData(hList, nPos, C_ACT  , "0"   , 0);
	} else {
		SetListData(hList, nPos, C_STAT , "◎"  , 1);
		SetListData(hList, nPos, C_ACT  , "1"   , 0);
	}
	SetListData(hList, nPos, C_TITLE, pFn   , 0);
	SetListData(hList, nPos, C_LINK , pFile , 0);
	SetListData(hList, nPos, C_INTVL, "1:00", 0);
	SetListData(hList, nPos, C_STIME, szBuf , 0);
	SetListData(hList, nPos, C_GID  , ""    , 0);
	SetListData(hList, nPos, C_PTN  , "0"   , 0);
}

// ListViewのデータ移動
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	リストのインデックス
// 3	int		1:上への移動/0:下への移動
// Return:		1:移動実施/0:移動範囲外
int MovupListData(HWND hList, int nIdx, int bAct)
{
	int nMax, nStat = FALSE;
	int nOrgIdx, nDstIdx, nMask;
	char *pBuf;
	Variable Edit;

	nMax = ListView_GetItemCount(hList);

	if (bAct == TRUE) {
		// 上へ移動の入替え位置設定
		if (nIdx > 0) {
			nStat = TRUE;
			nOrgIdx = nIdx;
			nDstIdx = nIdx - 1;
		}
	} else {
		// 下へ移動の入替え位置設定
		if (nIdx < (nMax - 1)) {
			nStat = TRUE;
			nOrgIdx = nIdx;
			nDstIdx = nIdx + 1;
		}
	}

	if (nStat == TRUE) {
		// 移動先データの退避
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
	
		// 移動元から移動先へデータ移動
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

		// 移動元へ退避データ移動
		SetListData(hList, nOrgIdx, C_STAT , Edit.get(0), 0);
		SetListData(hList, nOrgIdx, C_TITLE, Edit.get(1), 0);
		SetListData(hList, nOrgIdx, C_LINK , Edit.get(2), 0);
		SetListData(hList, nOrgIdx, C_ACT  , Edit.get(3), 0);
		SetListData(hList, nOrgIdx, C_INTVL, Edit.get(4), 0);
		SetListData(hList, nOrgIdx, C_STIME, Edit.get(5), 0);
		SetListData(hList, nOrgIdx, C_GID  , Edit.get(6), 0);
		SetListData(hList, nOrgIdx, C_PTN  , Edit.get(7), 0);

		free(pBuf);

		// 選択位置の修正
		SelListData(hList, nOrgIdx, FALSE);
		SelListData(hList, nDstIdx, TRUE);
	}

	return nStat;
}

// ListViewのデータグループ化
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	1:グループ設定/0:グループ解除
// Return:		TRUE:成功/FALSE:失敗
int GrpListData(HWND hList, int bSet)
{
	int nIdx, nMax, nCnt, i, j, nSelCnt, nMjGid;
	int nStat, nRet = FALSE;
	char szGid[MAX_BUF+1], szTop[MAX_BUF+1];
	char *pDelGid;
	Variable Edit;

	// ListViewのデータ数/選択状態数取得
	nMax = ListView_GetItemCount(hList);
	nCnt = ListView_GetSelectedCount(hList);

	if (bSet) {	// グループ設定
		if (nCnt > 1) {
			nSelCnt = 0;
			for (i = 0; i < nMax; ++i) {
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if (nStat & LVIS_SELECTED) { // 選択状態
					if (nSelCnt == 0) {	// 1番目の選択データ
						ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
						if (*szGid == '\0') { // 未設定
							// 空いている主グループIDで生成
							nMjGid = GetMajorGid(hList);
							sprintf(szGid, "%02d", nMjGid);
						}
						// 1番目のグループID保存
						strcpy(szTop, szGid);
					} else {
						// 2番目以降のグループID生成
						sprintf(szGid, "%s%02d", szTop, nSelCnt);
					}

					// タイトルのインデント削除
					IndGrpTitle(hList, i, FALSE);
					// グループIDの再設定
					SetListData(hList, i, C_GID, szGid, 0);
					// タイトルのインデント追加
					IndGrpTitle(hList, i, TRUE);
					nRet = TRUE;

					++nSelCnt;
				}
			}
		}
	} else {	// グループ解除
		if (nCnt > 0) {
			nSelCnt = 0;
			for (i = 0; i < nMax; ++i) {
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if (nStat & LVIS_SELECTED) { // 選択状態
					// 選択された解除グループID保存
					ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
					Edit.add(szGid);

					// タイトルのインデント削除
					IndGrpTitle(hList, i, FALSE);
					// グループID解除
					SetListData(hList, i, C_GID, "", 0);
				}
			}
			for (i = 0; i < nMax; ++i) {
				ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
				nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
				if ((nStat & LVIS_SELECTED) == 0) { // 非選択状態
					// 削除したGIDと先頭桁が同一のGIDを解除
					ListView_GetItemText(hList, i, C_GID, szGid, MAX_BUF);
					for (j = 0; j < Edit.size(); ++j) {
						pDelGid = Edit.get(j);
						if (!strncmp(szGid, pDelGid, strlen(pDelGid))) {
							// タイトルのインデント削除
							IndGrpTitle(hList, i, FALSE);
							// グループID解除
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

// 空いている主グループID取得
int GetMajorGid(HWND hList)
{
	int nMax, i, nMjGid, nMaxId = 0;
	char szBuf[MAX_BUF+1], szUsed[300+1];

	*szUsed = '\0';
	nMax = ListView_GetItemCount(hList);
	for (i = 0; i < nMax; ++i) {
		ListView_GetItemText(hList, i, C_GID, szBuf, MAX_BUF);
		if (strlen(szBuf) == 2) {
			// 最大値の判定
			nMjGid = atoi(szBuf);
			if (nMaxId < nMjGid) nMaxId = nMjGid;

			// 使用ID保存
			strcat(szUsed, szBuf);
			strcat(szUsed, ",");
		}
	}

	if (nMaxId < 99) { // 最大値が99未満
		// 最大値+1を返す
		nMjGid = nMaxId + 1;
	} else {
		// 空いている主グループIDを探してを返す
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

// グループIDに応じたタイトルのインデント設定
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	リストのインデックス
// 3	int 	1:インデント追加/0:インデント削除
// Return:		TRUE:成功/FALSE:失敗
int IndGrpTitle(HWND hList, int nIdx, int bAdd)
{
	int i, nCnt, nStat = FALSE;
	char szBuf[MAX_BUF+1], szTitle[MAX_BUF+1];
	char *pOrg, *pDst;

	// グループIDの桁数取得
	ListView_GetItemText(hList, nIdx, C_GID, szBuf, MAX_BUF);
	nCnt = strlen(szBuf);
	if (nCnt > 2) { // 2桁より大きい
		ListView_GetItemText(hList, nIdx, C_TITLE, szTitle, MAX_BUF);
		pOrg = szTitle;
		pDst = szBuf;
		for (i = 0; i < (nCnt - 2); ++i) { // 副グループIDの桁数分
			if (bAdd) { // 行頭へ空白の追加
				*pDst = ' ';
				++pDst;
			} else { // 行頭から空白の削除
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

// ListViewのデータ削除
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	リストのインデックス
// Return:		TRUE:成功/FALSE:失敗
int DelListData(HWND hList, int nIdx)
{
	int nMax, nStat, nCnt, nRet = FALSE;
	char szBuf[MAX_BUF+1];
	char szMsg[MAX_BUF+1];

	nMax = ListView_GetItemCount(hList);

	if (nIdx >= 0) {
		// グループIDの桁数取得
		ListView_GetItemText(hList, nIdx, C_GID, szBuf, MAX_BUF);
		nCnt = strlen(szBuf);

		ListView_GetItemText(hList, nIdx, C_TITLE, szBuf, MAX_BUF);
		if (nCnt > 2) { // 2桁より大きい
			// 行頭から空白の削除
			strcpy(szBuf, szBuf + (nCnt - 2));
		}

		sprintf(szMsg, "「%s」を削除しますか？", szBuf);
		nStat = MsgBox(hMainWnd, szMsg, "確認", MB_YESNO | MB_DEFBUTTON2);
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

// ListViewのデータ選択
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	0>:選択するデータのインデックス/-1:全インデックス
// 3	int 	1:選択状態設定/0:選択状態解除
// Return:		なし
void SelListData(HWND hList, int nIdx, int bAct)
{
	int nMask, i, nStt, nEnd;

	if (nIdx == -1) { // 全インデックス指定
		nStt = 0;
		nEnd = ListView_GetItemCount(hList) - 1;
	} else {  // 個別インデックス指定
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

// ListViewのデータ検索
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int 	1文字コード(SRCH_HEAD/SRCH_INCLUDEの場合)
// 				文字列のポインタ(SRCH_MATCHの場合)
// 3	int 	SRCH_HEAD	:先頭1文字検索
// 				SRCH_INCLUDE:1文字を含む検索
// 				SRCH_MATCH	:文字列を含む検索
// Return:		なし
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

			if (nSrchPtn == SRCH_HEAD) { // 先頭1文字に一致
				// 行頭の空白削除
				Edit.set(szBuf, 0);
				Edit.gsub(" +", "");
				pStr = Edit.get(0);
				// 先頭1文字の一致を判定
				cCode = (char)SrchParam;
				cChar = *pStr;
				if ('A' <= cCode && cCode <= 'Z') {
					if (cCode == cChar || (cCode + 0x20) == cChar) nFnd = TRUE;
				} else {
					if (cCode == cChar) nFnd = TRUE;
				}
			} else if (nSrchPtn == SRCH_INCLUDE) { // 1文字を含む
				cCode = (char)SrchParam;
				szSrch[0] = cCode;
				szSrch[1] = '\0';
				pPos = StrChr(szBuf, szSrch);
				if (pPos) {
					nFnd = TRUE;
				} else if ('A' <= cCode && cCode <= 'Z') {
					// 小文字で再検索
					szSrch[0] += 0x20;
					pPos = StrChr(szBuf, szSrch);
					if (pPos) nFnd = TRUE;
				}
			} else if (nSrchPtn == SRCH_MATCH) { // 文字列検索
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

// ListViewデータの監視状態戻し
void ResetListStat(HWND hList, int nIdx)
{
	char szBuf[MAX_BUF+1];

	if (nIdx >= 0) {
		ListView_GetItemText(hList, nIdx, C_STAT, szBuf, MAX_BUF);
		if (!strcmp(szBuf, "●")) {
			SetListData(hList, nIdx, C_STAT , "◎" , 0);
			GetTime(szBuf);
			SetListData(hList, nIdx, C_STIME, szBuf, 0);
			SetWinIcon(FALSE);
		}
	}
}

// 選択ListViewデータの状態表示
void DspListStat(HWND hList)
{
	int nIdx;
	char szBuf[MAX_BUF+1], szMsg[MAX_BUF+1];

	*szMsg = '\0';
	nIdx = ListView_GetSelectionMark(hList);
	if (nIdx >= 0) {
		ListView_GetItemText(hList, nIdx, C_STAT, szBuf, MAX_BUF);
		if (!strcmp(szBuf, "◎")) {
			strcat(szMsg, "監視中");
			strcat(szMsg, "(");
			ListView_GetItemText(hList, nIdx, C_INTVL, szBuf, MAX_BUF);
			strcat(szMsg, szBuf);
			ListView_GetItemText(hList, nIdx, C_PTN, szBuf, MAX_BUF);
			if (!strcmp(szBuf, "1")) {
				strcat(szMsg, ", 更新+追加+削除");
			} else {
				strcat(szMsg, ", 更新+追加");
			}
			strcat(szMsg, ")");
		} else if (!strcmp(szBuf, "●")) {
			strcat(szMsg, "変化あり");
			strcat(szMsg, "(");
			ListView_GetItemText(hList, nIdx, C_STIME, szBuf, MAX_BUF);
			strcat(szMsg, szBuf);
			strcat(szMsg, "～)");
		} else if (!strcmp(szBuf, "×")) {
			strcat(szMsg, "アクセス不可");
		} else {
			strcat(szMsg, "");
		}
	}
	DspStatus(szMsg);
}

// 選択リストの最小/最大インデックス取得(連続した選択のみの前提)
// Args:
// 1	HWND 	リストビューのハンドル
// 2	int * 	選択されたリストの最小インデックス(結果書込み)
// 3	int * 	選択されたリストの最大インデックス(結果書込み)
// Return:		TRUE:選択されたリストあり、FALSE:選択されたリストなし
int GetSelList(HWND hList, int *lpStart, int *lpEnd)
{
	int i, nMax, nCnt, nStt, nEnd, nStat, nRet;

	// ListViewのデータ数/選択状態数取得
	nMax = ListView_GetItemCount(hList);
	nCnt = ListView_GetSelectedCount(hList);

	if (nCnt >= 0) {
		nStt = -1;
		nEnd = -1;
		for (i = 0; i < nMax; ++i) {
			nStat = ListView_GetItemState(hList, i, LVIS_SELECTED);
			if (nStat & LVIS_SELECTED) { // 選択状態
				if (nStt == -1) {
					// 最小/最大インデックスの設定
					nStt = i;
					nEnd = i;
				} else {
					// 最大インデックスの設定
					nEnd = i;
				}
			} else if (nStt != -1) { // インデックス設定済み
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

// メニュー項目の追加
void InsMenu(HMENU hMenu, int nPos, char *pTypeData, LONG wID, HMENU hSubMenu)
{
	MENUITEMINFO MenuInfo;

	MenuInfo.cbSize = sizeof(MENUITEMINFO);

	if (hSubMenu == NULL) {
		// サブメニュー以外
		if (pTypeData == NULL) {
			MenuInfo.fMask    = MIIM_TYPE | MIIM_STATE;
		} else {
			MenuInfo.fMask    = MIIM_TYPE | MIIM_STATE | MIIM_ID;
			MenuInfo.wID      = wID;
		}
		MenuInfo.fState   = MFS_ENABLED;
	} else {
		// サブメニューの設定
		MenuInfo.fMask    = MIIM_TYPE | MIIM_SUBMENU;
		MenuInfo.hSubMenu = hSubMenu;
	}

	if (pTypeData == NULL) {
		// セパレータの設定
		MenuInfo.fType      = MFT_SEPARATOR;;
	} else {
		// メニュー項目の設定
		MenuInfo.fType      = MFT_STRING;;
		MenuInfo.dwTypeData = pTypeData;
		MenuInfo.cch        = strlen(MenuInfo.dwTypeData);
	}
	InsertMenuItem(hMenu, nPos, TRUE, &MenuInfo);
}

// 監視周期時間の相互変換(h:mm ⇔ h時間/mm分)
void CvtDspTime(char *pOrgStr, char *pDstStr)
{
	Variable Edit;
	int nHour, nMin;

	Edit.split(pOrgStr, ":");
	if (Edit.size() == 2) {
		// h:mm ⇒ h時間/mm分
		nHour = atoi(Edit.get(0));
		nMin  = atoi(Edit.get(1));

		if (nHour == 0) {
			sprintf(pDstStr, "%d分", nMin);
		} else {
			if (nMin == 0) {
				sprintf(pDstStr, "%d時間", nHour);
			} else {
				sprintf(pDstStr, "%d時間%d分", nHour, nMin);
			}
		}
	} else {
		// h時間/mm分 ⇒ h:mm
		Edit.split(pOrgStr, "時間");
		if (Edit.size() == 2) {
			nHour = atoi(Edit.get(0));
			Edit.gsub("分", "");
			nMin  = atoi(Edit.get(1));
		} else {
			nHour = 0;
			nMin  = atoi(Edit.get(0));
		}
		sprintf(pDstStr, "%d:%02d", nHour, nMin);
	}
}

// ファイルの実行
int ExecFile(char *pFile)
{
	HWND hEdit;
	int nStat, nLen, nLidx, nMax;
	char exec[MAX_PATH+1], *pParam;

	// ファイル名をダブルクォートで括る
	nLen = strlen(pFile);
	pParam = (char *)malloc(nLen + 3);
	sprintf(pParam, "\"%s\"", pFile);

	/* ファイル属性取得 */
	nStat = GetFileAttributes(pFile);
	if (nStat != -1 && (nStat & FILE_ATTRIBUTE_DIRECTORY)) {
		/* ディレクトリの場合、Explorer起動 */
		nStat = (int)ShellExecute(NULL, "explore", pParam,
		                          NULL, NULL, SW_SHOWNORMAL);
	} else {
		/* 関連付けられた実行ファイルがあれば起動 */
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

// パラメータファイルの取込み
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
	
	// パラメータファイル名の取得
	Fn.add(pExcFile);
	Fn.gsub("^\"|\"$", "");
	pFn = Fn.get(0);

	Edit.split(pFn, "\\\\");
	Edit.set(PARAM_FILE, -1);
	pFn = Edit.join("\\");
	strcpy(ParamFile, pFn);
	free(pFn);

	hList = GetDlgItem(hMainWnd, ListView);

	// パラメータファイル読込み
	fp = fopen(ParamFile, "r");
	if (fp == NULL)	return;
	while (fgets(szBuf, MAX_FULL_PATH, fp) != NULL) {
		// 改行削除
		SubStr(szBuf, "\n", "", 0);
		if (nPid == ID_NONE) {
			// カテゴリの判定
			if (!strcmp(szBuf, "[Window]")) {
				nPid = ID_WINDOW;
				nPosFlg = 1;
			} else if (!strcmp(szBuf, "[Option]")) {
				nPid = ID_OPTION;
			} else if (!strcmp(szBuf, "[ListData]")) {
				nPid = ID_LISTDATA;
			}
		} else if (nPid == ID_WINDOW) {
			// ウィンドウ位置/サイズ取得
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
			// オプション取得
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

				// フォントを変更する
				if (hFont_MainWnd != NULL)	DeleteObject(hFont_MainWnd);
				hFont_MainWnd = CreateFontIndirect(cf.lpLogFont);
				SendMessage(hList, WM_SETFONT, (WPARAM)hFont_MainWnd, 0);
			}
		} else if (nPid == ID_LISTDATA) {
			// リストデータ取得
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

	// ウィンドウ位置/サイズの変更
	if (nPosFlg == 1) {
		SetWindowPos(hMainWnd, HWND_TOP, sx, sy, cx, cy, 0);
	}

	return;
}

// パラメータ文字の判定
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

// パラメータファイルへの保存
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

// ファイル選択による新規リスト登録
void SelectFile()
{
	OPENFILENAME ofn;
	char *pFiles, *pFd, *pFn, *pBuf;
	int nStat, nOfset;

	pFiles = (char *)malloc(MAX_FULL_PATH + 1);
	pFd    = (char *)malloc(MAX_FULL_PATH + 1);

	// ファイル名を取得 
	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(pFiles, MAX_FULL_PATH + 1);
	ofn.lStructSize    = sizeof(ofn);
	ofn.hwndOwner      = hMainWnd;
	ofn.lpstrFilter    = "すべてのファイル(*.*)\0*\0\0";
	ofn.nFilterIndex   = 1;
	ofn.nMaxFile       = MAX_FULL_PATH;
	ofn.lpstrFile      = pFiles;
	//ofn.Flags          = OFN_ALLOWMULTISELECT + OFN_FILEMUSTEXIST
	//                     + OFN_LONGNAMES + OFN_EXPLORER;
	ofn.Flags          = OFN_FILEMUSTEXIST + OFN_LONGNAMES + OFN_EXPLORER;
	nStat = GetOpenFileName(&ofn);
	if (nStat == 0) return;

	// ファイル名をリストデータ登録
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

// フォルダ選択による新規リスト登録
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

// ダイアログによるフォルダ名選択
int GetDir(HWND hWnd, char *pDefDir, char *pPath)
{
	BROWSEINFO bInfo;
	LPITEMIDLIST pIDList;

	memset(&bInfo, 0, sizeof(bInfo));
	bInfo.hwndOwner = hWnd;				// ダイアログの親ウインドウのハンドル
	bInfo.pidlRoot  = NULL;				// ルートフォルダをデスクトップフォルダ
	bInfo.pszDisplayName = pPath;		// 選択されたフォルダ名が格納
	bInfo.lpszTitle = "フォルダの選択"; // ツリービューのタイトル文字列
	bInfo.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE;
	bInfo.lpfn      = EventCall_GetDir;	// コールバック関数のアドレス 
	bInfo.lParam    = (LPARAM)pDefDir;	// コールバック関数に渡す引数

	pIDList = SHBrowseForFolder(&bInfo);
	if (pIDList == NULL){
		*pPath = '\0';

		return FALSE;	// 何も選択されなかった場合 
	} else {
		if (!SHGetPathFromIDList(pIDList, pPath))
			return FALSE;	// 変換に失敗 

		CoTaskMemFree(pIDList);	// pIDListのメモリを開放 

		return TRUE;
	}
}

int __stdcall EventCall_GetDir(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	char szDir[MAX_FULL_PATH + 1];
	ITEMIDLIST *lpid;
	HWND hEdit;

	switch (uMsg) {
		case BFFM_INITIALIZED:	// ダイアログボックス初期化時
			// コモンダイアログの初期ディレクトリ
			SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
			break;

		case BFFM_VALIDATEFAILED:	// 無効なフォルダー名が入力された
			MessageBox(hWnd, (char *)lParam,
						"無効なフォルダー名が入力されました", MB_OK);
			// エディットボックスのハンドルを取得する
			hEdit = FindWindowEx(hWnd, NULL, "EDIT", NULL);
			SetWindowText(hEdit, "");
			return 1;	//  ダイアログボックスを閉じない

		case BFFM_SELCHANGED:	// 選択フォルダーが変化した場合
			lpid = (ITEMIDLIST *)lParam;
			SHGetPathFromIDList(lpid, szDir);
			// エディットボックスのハンドルを取得する
			hEdit = FindWindowEx(hWnd, NULL, "EDIT", NULL);    
			SetWindowText(hEdit, szDir);
			break;
	}

	return 0;
}

// WindowsのOSバージョン取得
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

// 現在時刻取得
// Args:
// 1	char *	現在時刻文字列(YYYY/M/D h:mm:ss)を受取るポインタ
// Return:		なし
void GetTime(char *pCurTime)
{
	time_t now = time(NULL);
	struct tm *pNow = localtime(&now);

	sprintf(pCurTime, "%d/%d/%d %d:%02d:%02d", pNow->tm_year + 1900, pNow->tm_mon + 1, pNow->tm_mday, pNow->tm_hour, pNow->tm_min, pNow->tm_sec);
}

// ファイル/フォルダのアクセス時刻取得
// Args:
// 1	char *	ファイル名/フォルダ名
// 2	char *	アクセス時刻文字列(YYYY/M/D h:mm:ss)を受取るポインタ
// 3	int 	フォルダ時刻取得有無(0:配下のファイルのみ、1:フォルダを含む)
// 4	char *	アクセス時刻取得を除外するファイル名(file1;file2;...形式)
// Return:		TRUE:取得成功、FALSE:取得失敗
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
	if (nAtr == -1) { // ファイルなし
		return FALSE;
	} else if(nAtr & FILE_ATTRIBUTE_ARCHIVE) { // ファイル
		hFile = FindFirstFile(pFile, &wfd);
		if (hFile == INVALID_HANDLE_VALUE) { // Openエラー
			return FALSE;
		}

		AccessTime = wfd.ftLastAccessTime;
		GetFileTime(hFile,  NULL, &AccessTime, NULL);
		//CloseHandle(hFile);
		FindClose(hFile);

		FileTimeToLocalFileTime(&AccessTime , &LocalTime);
	} else if(nAtr & FILE_ATTRIBUTE_DIRECTORY) { // フォルダ
		if (nPtn == 1) {
			// フォルダのアクセス日時取得
			hFile = FindFirstFile(pFile, &wfd);
			FindClose(hFile);
			LastTime = wfd.ftLastAccessTime;
		} else {
			ZeroMemory(&LastTime, sizeof(FILETIME));
		}

		// アクセス日時取得の除外ファイル名の正規表現作成
		if (pExclude) {
			Edit.clear();
			Edit.add(pExclude);
			// ";"区切りのワイルドカード形式から正規表現へ変換
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

		// フォルダ配下のファイルで最新アクセス日時取得
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
					// 除外ファイル名の一致判定
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

// FILETIMEの比較
int CmpFileTime(FILETIME Time1, FILETIME Time2)
{
	int nStat;

	nStat = Time1.dwHighDateTime -  Time2.dwHighDateTime;
	if (nStat == 0) {
		nStat = Time1.dwLowDateTime -  Time2.dwLowDateTime;
	}

	return nStat;
}

// フルパス内におけるフォルダ名の長さ取得
// Args:
// 1	char *	ファイル名(フルパス)
// Return:		0:失敗、0>:フォルダ名の長さ
int GetPathLen(char *pFile)
{
	int nAtr, nLen = 0;
	Variable Edit;

	nAtr = GetFileAttributes(pFile);
	if (nAtr == -1) { // ファイルなし
		;
	} else if(nAtr & FILE_ATTRIBUTE_ARCHIVE) { // ファイル
		Edit.StrSplit(pFile, "\\", FALSE, FALSE);
		nLen = strlen(pFile) - strlen(Edit.get(-1)) - 1;
	} else if(nAtr & FILE_ATTRIBUTE_DIRECTORY) { // フォルダ
		nLen = strlen(pFile);
	}

	return nLen;
}

// アイコンの設定
// Args:
// 1	int 	TRUE:変化あり、FALSE:標準
// Return:      なし
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
	
// デバッグ用ログの保存/表示
// Args:
// 1	char *	ログ文字列:保存、NULL:表示
// Return:      なし
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
		MsgBox(hMainWnd, pMsg, "ログ", MB_OK);
		free(pMsg);
	}
}
