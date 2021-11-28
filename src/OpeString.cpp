/*
 * �����񑀍�Ɋւ��鏈��
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OpeString.h"

/* SJIS�̊������� */
int IsKanji(unsigned char c)
{
	if((c >= 0x80 && c < 0xA0) || c >= 0xE0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/* �����̌��� */
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

/* CommandLine�t�@�C�����̕��� */
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

/* ���蕶���̃J�E���g */
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

/* n�Ԗڂ̓��蕶���ʒu�̌��� */
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

/* ������̒u������ */
int SubStr(char *pStr, char *pSrch, char *pRep, int nCnt)
{
	int	nIdx = 0, nDone = 0;
	int nSrchLen, nRepLen;
	char buf[1024];

	nSrchLen = strlen(pSrch);
	nRepLen = strlen(pRep);
	while (pStr[nIdx]) {
		/* �u�����������̌��� */
		if(!strncmp(pStr + nIdx, pSrch, nSrchLen)) {
			/* �����̒u���� */
			strcpy(buf, pStr + nIdx + nSrchLen);
			strcpy(pStr + nIdx, pRep);
			strcpy(pStr + nIdx + nRepLen, buf);
			++nDone;
			/* �u�����񐔂̃`�F�b�N */
			if (nDone == nCnt)	break;
			/* �����ʒu�̍X�V */
			nIdx += (nRepLen + 1);
		} else {
			/* �����ʒu�̍X�V */
			if (IsKanji(pStr[nIdx])) {
				nIdx += 2;
			} else {
				++nIdx;
			}
		}
	}
	return(nDone);

}

/* ��납��N�����ڂ̃|�C���^ */
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

/* �������r(��/��������ʂȂ�) */
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
