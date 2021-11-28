/*
 * �t�@�C���ω��Ď��c�[��
 */

// �Œ�l 
#define VERSION		"1.04"
#define DATE		"2021/2/2"

#define MAX_FULL_PATH	32767 

#define C_STAT		0	// ���(��:�Ď����A��:�ω�����A�~:�����N��A�N�Z�X�s��)
#define C_TITLE		1	// �^�C�g��
#define C_LINK		2	// �����N��
#define C_ACT		3	// �Ď��L��(0:�Ȃ��A1:����)
#define C_INTVL		4	// �Ď�����(h:mm)
#define C_STIME		5	// �Ď��J�n����(YYYY/MM/DD h:mm:ss)
#define C_GID		6	// �O���[�vID
#define C_PTN		7	// �t�H���_�Ď��p�^�[��
#define NUM_ITEM	8	// ���ڂ̐�

// ���[�J���\���̒�`
typedef struct {
	char *pDspStr;	// ���j���[�\������
	char *pSetStr;	// ListView�ݒ蕶��
	int nIdm;		// IDM�l
} POPMENU_INFO;

// �O���[�o���ϐ���`
extern HWND hMainWnd; 
extern POPMENU_INFO ActInfo[];
extern POPMENU_INFO IntvlInfo[];
extern POPMENU_INFO PtnInfo[];

// �֐���`
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
