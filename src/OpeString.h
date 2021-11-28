/*
 * •¶š—ñ‘€ì‚ÉŠÖ‚·‚éˆ—‚Ì’è‹`
 */

#ifndef OPESTRING_INCLUDED
#define OPESTRING_INCLUDED

int IsKanji(unsigned char c);
char *StrChr(char *s, char *c);
int GetElement(char *pOrg, int nIdx, char **pArg);
int CountChar(char *p, char c);
int SrchCharPos(char *p, char c, int idx);
int SubStr(char *pStr, char *pSrch, char *pRep, int nCnt);
char *TailStr(char *pStr, int nCnt);
int CmpStr(unsigned char *s1, unsigned char *s2);

#endif
