/*
 * ファイル変化監視ツール
 */

// 固定値 
#define VERSION		"1.04"
#define DATE		"2021/2/2"

#define MAX_FULL_PATH	32767 

#define C_STAT		0	// 状態(◎:監視中、●:変化あり、×:リンク先アクセス不可)
#define C_TITLE		1	// タイトル
#define C_LINK		2	// リンク先
#define C_ACT		3	// 監視有無(0:なし、1:あり)
#define C_INTVL		4	// 監視周期(h:mm)
#define C_STIME		5	// 監視開始時刻(YYYY/MM/DD h:mm:ss)
#define C_GID		6	// グループID
#define C_PTN		7	// フォルダ監視パターン
#define NUM_ITEM	8	// 項目の数

// ローカル構造体定義
typedef struct {
	char *pDspStr;	// メニュー表示文字
	char *pSetStr;	// ListView設定文字
	int nIdm;		// IDM値
} POPMENU_INFO;

// グローバル変数定義
extern HWND hMainWnd; 
extern POPMENU_INFO ActInfo[];
extern POPMENU_INFO IntvlInfo[];
extern POPMENU_INFO PtnInfo[];

// 関数定義
LRESULT CALLBACK EventCall_MainWnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT MainWnd_Create(HWND hWnd, LPCREATESTRUCT cs);
LRESULT MainWnd_Timer(HWND hWnd, LONG id);
LRESULT MainWnd_Resize(HWND hWnd, LONG SizeType, WORD cx, WORD cy);
LRESULT MainWnd_Activate(HWND hWnd, WORD state, WORD minimized);
LRESULT MainWnd_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
LRESULT MainWnd_MenuSelect(HWND hWnd);
int CALLBACK SortComp(LPARAM lp1, LPARAM lp2, LPARAM lp3);
int CompExcNull(char *pStr1, char *pStr2, int nSort);
LRESULT CALLBACK EventCall_ListView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT ListView_Command(HWND hWnd, WORD wNotifyCode, WORD wID, HWND hWndCtl);
LRESULT ListView_DropFiles(HWND hWnd, HDROP hDrop);
LRESULT ListView_KeyDown(HWND hWnd, DWORD KeyCode);
LRESULT ListView_LButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT ListView_LButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT ListView_DoubleClick(HWND hWnd);;
LRESULT ListView_RButtonDown(HWND hWnd, LPARAM lParam);

int Event_MenuRButton(HMENU hMenu, UINT uItem);
void DspStatus(char *pStr);
int setcf(HWND hWnd, CHOOSEFONT *cf);
void SetListTitle(HWND hList, int nCidx, int nCx, char *pText, int nFmt);
void SetListData(HWND hList, int nRidx, int nCidx, char *pText, int bFlg);
void RegListData(char *pFile, int nPos);
int MovupListData(HWND hList, int nIdx, int bAct);
int GrpListData(HWND hList, int bSet);
int GetMajorGid(HWND hList);
int IndGrpTitle(HWND hList, int nIdx, int bAdd);
int DelListData(HWND hList, int nIdx);
void SelListData(HWND hList, int nIdx, int bAct);
void SrchListData(HWND hList, int SrchParam, int nSrchPtn);
void ResetListStat(HWND hList, int nIdx);
void DspListStat(HWND hList);
int GetSelList(HWND hList, int *lpStart, int *lpEnd);
void InsMenu(HMENU hMenu, int nPos, char *pTypeData, LONG wID, HMENU hSubMenu);
void CvtDspTime(char *pOrgStr, char *pDstStr);
int ExecFile(char *pFile);
void LoadParamFile(char *pExcFile);
int ChkParam(char *pLine, char *pParam);
void SaveParamFile();
void SelectFile();
void SelectFolder();
int GetDir(HWND hWnd, char *pDefDir, char *pPath);
int __stdcall EventCall_GetDir(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
int GetWinVer(DWORD *lpMajor, DWORD *lpMinor);
void GetTime(char *pCurTime);
int GetFileAccTime(char *pFile, char *pAccTime, int nPtn, char *pExclude);
int CmpFileTime(FILETIME Time1, FILETIME Time2);
int GetPathLen(char *pFile);
void SetWinIcon(int bFnd);
