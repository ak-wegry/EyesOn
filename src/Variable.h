/*
 * Variableクラスの定義
 */

#ifndef VARIABLE_INCLUDED
#define VARIABLE_INCLUDED

#define CONV_UPPER			1	// 大文字変換
#define CONV_LOWER			2	// 小文字変換
#define CONV_WIDE_ASCII		4	// 英数字の全角変換
#define CONV_WIDE_KANA		8	// カタカナの全角変換
#define CONV_WIDE			(CONV_WIDE_ASCII+CONV_WIDE_KANA)
#define CONV_NARROW_ASCII	16	// 英数字の半角変換
#define CONV_NARROW_KANA	32	// カタカナの半角変換
#define CONV_NARROWI		(CONV_NARROW_ASCII+CONV_NARROW_KANA)

// -----------------------------------------------------------------------------
// Variableクラスの定義
class Variable {
	char **m_pList;	// データリストの先頭ポインタ
	int m_nMaxSize;	// 登録したデータの最大数
	int m_nMaxArea;	// 確保したデータ領域の最大数

public:
	Variable();
	~Variable();
	char **GetList(int nIdx, int bExpand);
	int AdjIdx(nIdx);
	int IsKanji(unsigned char c);
	void clear();
	int size();
	set(char *pStr, int nIdx);
	add(char *pStr);
	add(char *pStr, int nLen);
	move(int nOrgIdx, int nDstIdx);
	del(int nIdx);
	ins(char *pStr, int nIdx);
	char *get(int nIdx);
	int index(char *pStr);
	char *join(char *pSep);
	int split(char *pStr, char *pDlmt, int nCnt);
	int split(char *pStr, char *pDlmt);
	match(char *pStr, char *pRegex);
	scan(char *pStr, char *pRegex);
	RegexMatch(char *pStr, char *pRegex, int nCnt, int bRvs);
	char *gsub(char *pOrgStr, char *pRegex, char *pReplace);
	gsub(char *pRegex, char *pReplace);
	int uniq();
	QuickSort(char **pData, int nTop, int nBottom, int bAsc, int bLogical);
	sort(int bAsc, int bLogical);
	sort(int bAsc);
	sort();
	StrCmpLogical(char *pStr1, char *pStr2);
	void MakCmpDat(char *pStr, Variable *pDat);
	int GetRngPos(char *pOrg, char *pRng);
	int GetRngChar(char *pRng, int nIdx);
	char *tr(char *pStr, char *pFrom, char *pTo);
	tr(char *pFrom, char *pTo);
	char *CsvTr(char *pStr, char *pFrom, char *pTo);
	CsvTr(char *pFrom, char *pTo);
	char *StrConv(char *pOrgStr, int nConv);
	StrConv(int nConv);
	char *MetaConv(char *pOrgStr);
	MetaConv();
	int StrSplit(char *pStr, char *pDlmt, int bDlmtRept, int bWQuot);
	int WildSrch(char *pOrgStr, char *pWldStr, int *lpStt, int *lpLen);
	char *wsub(char *pOrgStr, char *pSrch, char *pReplace);
	wsub(char *pSrch, char *pReplace);
	void DspList();
};

#endif
