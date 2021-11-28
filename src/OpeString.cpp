/*
 * 文字列操作に関する処理
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OpeString.h"

/* SJISの漢字判定 */
int IsKanji(unsigned char c)
{
	if((c >= 0x80 && c < 0xA0) || c >= 0xE0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/* 文字の検索 */
char *StrChr(char *s, char *c)
{
	char *ret = NULL;

	while (*s) {
		if (IsKanji(*s)) {
			if (!memcmp(s, c, 2)) {
				ret = s;
				break;
			} else { 
				s += 2;
			}
		} else {
			if (*s == *c) {
				ret = s;
				break;
			} else {
				++s;
			}
		}
	}

	return ret;
}

/* CommandLineファイル名の分解 */
int GetElement(char *pOrg, int nIdx, char **pArg)
{
	static char *pSave = NULL;
	static int nPos[1024];
	static int nMax = 0;
	int i, stt_f;
	char cSrch;

	if (pOrg != NULL) {
		if (pSave != NULL) {
			delete pSave;
			nMax = 0;
		}
		pSave = strdup(pOrg);

		i = 0;
		stt_f = 0;
		while (pSave[i]) {
			if (stt_f == 0) {
				if(pSave[i] == '"') {
					++i;
					nPos[++nMax] = i;
					cSrch = '"';
					stt_f = 1;
				} else if(pSave[i] == ' ') {
					++i;
				} else {
					nPos[++nMax] = i;
					cSrch = ' ';
					stt_f = 1;
				}
			} else {
				if (pSave[i] == cSrch) {
					pSave[i] = '\0';
					++i;
					stt_f = 0;
				} else {
					if (IsKanji(pSave[i])) {
						i += 2;
					} else {
						++i;
					}
				}
			}
		}
		return nMax;
	} else {
		if (nIdx <= nMax) {
			*pArg = pSave + nPos[nIdx];
			return 1;
		} else {
			*pArg = NULL;
			return 0;
		}
	}
}

/* 特定文字のカウント */
int CountChar(char *p, char c)
{
	int n = 0;

	while (*p) {
		if (*p == c) ++n;
		if (IsKanji(*p)) {
			p += 2;
		} else {
			++p;
		}
	}

	return n;
}

/* n番目の特定文字位置の検索 */
int SrchCharPos(char *p, char c, int idx)
{
	int i = 0, n = 0, find_f = 0;

	while (p[i] && find_f == 0) {
		if (p[i] == c) {
			++n;
			if (idx <= n) {
				find_f = 1;
				break;
			}
		}

		if (IsKanji(p[i])) {
			i += 2;
		} else {
			++i;
		}
	}

	if (find_f) {
		return i;
	} else {
		return -1;
	}
}

/* 文字列の置き換え */
int SubStr(char *pStr, char *pSrch, char *pRep, int nCnt)
{
	int	nIdx = 0, nDone = 0;
	int nSrchLen, nRepLen;
	char buf[1024];

	nSrchLen = strlen(pSrch);
	nRepLen = strlen(pRep);
	while (pStr[nIdx]) {
		/* 置き換え文字の検索 */
		if(!strncmp(pStr + nIdx, pSrch, nSrchLen)) {
			/* 文字の置換え */
			strcpy(buf, pStr + nIdx + nSrchLen);
			strcpy(pStr + nIdx, pRep);
			strcpy(pStr + nIdx + nRepLen, buf);
			++nDone;
			/* 置換え回数のチェック */
			if (nDone == nCnt)	break;
			/* 検索位置の更新 */
			nIdx += (nRepLen + 1);
		} else {
			/* 検索位置の更新 */
			if (IsKanji(pStr[nIdx])) {
				nIdx += 2;
			} else {
				++nIdx;
			}
		}
	}
	return(nDone);

}

/* 後ろからN文字目のポインタ */
char *TailStr(char *pStr, int nCnt)
{
	int nLen;
	char *pTail;

	nLen = strlen(pStr);
	if (nCnt <= nLen) {
		pTail = pStr + nLen - nCnt;
	} else {
		pTail = pStr;
	}

	return pTail;
}

/* 文字列比較(大/小文字区別なし) */
int CmpStr(unsigned char *s1, unsigned char *s2)
{
	int nStat;
	unsigned char bLow1, bLow2;
	char bKanji = 0;

	while (*s1 && *s2) {
		if (isalpha(*s1) && isalpha(*s2) && bKanji == 0) {
			bLow1 = *s1 & 0x20;
			bLow2 = *s2 & 0x20;
			nStat = (int)(bLow1 - bLow2);
		} else {
			nStat = (int)(*s1 - *s2);
		}
		if (nStat != 0) break;

		if (IsKanji(*s1) && IsKanji(*s2)) {
			bKanji = 1;
		} else {
			bKanji = 0;
		}
		++s1;
		++s2;
	}

	return nStat;
}
