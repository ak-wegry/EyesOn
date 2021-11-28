/*
 * Variableクラスの定義
 *======================================================================
 *[変更履歴]
 * Ver1.00  2019/05/04 新規作成
 * Ver1.01  2020/05/10 空のリストへins()出来ない不具合修正
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <pcreposi.h>
#include "Variable.h"

#define MAX_BUF	256

#define UCHAR	unsigned char
#define CODE_VAL(p)	(IsKanji(*p) ? (*(UCHAR *)(p) * 256 + *(UCHAR *)(p+1)) : *(UCHAR *)p)
#define CHAR_LEN(p)	(IsKanji(*p) ? 2 : 1)

// コンストラクタ
Variable::Variable()
{
	m_pList = NULL;
	m_nMaxSize = 0;
	m_nMaxArea = 0;
}

// デストラクタ
Variable::~Variable()
{
	clear();
	if(m_pList != NULL) {
		free((char *)m_pList);
		m_pList = NULL;
	}
	m_nMaxArea = 0;
}

// テーブルのポインタ取得
char **Variable::GetList(int nIdx, int bExpand)
{
	char **pList, *pNew;
	int nBlkCnt;	// メモリを確保する1ブロック数
	int nSize;		// 1リストのメモリサイズ
	int nCnv;		// nIdxがマイナス時の補正値

	nBlkCnt = 10;
	nSize = sizeof(char *);
	if(nIdx < m_nMaxSize) {
		// 登録データのポインタ設定
		if (nIdx < 0) {
			nCnv = m_nMaxSize + nIdx;
			if (nCnv >= 0) {
				pList = m_pList + nCnv;
			} else {
				pList = NULL;
			}
		} else {
			pList = m_pList + nIdx;
		}
	} else {
		if(bExpand == FALSE) {
			// 領域拡張なしの場合、NULLポインタ設定
			pList = NULL;
		} else {
			if(m_nMaxArea <= nIdx) {
				// 新規データ領域の確保
				if(m_nMaxSize == 0) {
					m_pList = (char **)new char[nBlkCnt * nSize];
				} else {
					pNew = new char[(m_nMaxArea + nBlkCnt) * nSize];
					memcpy(pNew, m_pList, m_nMaxSize * nSize);
					delete m_pList;
					m_pList = (char **)pNew;
				}
				// 拡張データ領域のクリア
				memset(m_pList + m_nMaxArea, 0, nBlkCnt * nSize);

				// 確保データ領域数の更新
				m_nMaxArea += nBlkCnt;
			}

			// 登録データのポインタ設定
			pList = m_pList + nIdx;

			// 登録データ数の更新
			m_nMaxSize = nIdx + 1;
		}
	}

	return pList;
}

// リストのインデックス値補正
int Variable::AdjIdx(int nIdx)
{
	int nCnv, nRet = -1;

	// マイナスのインデックス値補正
	if (nIdx < 0) {
		nCnv = m_nMaxSize + nIdx;
	} else {
		nCnv = nIdx;
	}

	// インデックス値の正常判定
	if (0 <= nCnv && nCnv < m_nMaxSize) {
		nRet = nCnv;
	}

	return nRet;
}

// SJISの漢字判定
int Variable::IsKanji(unsigned char c)
{
	if((c >= 0x80 && c < 0xA0) || c >= 0xE0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

// データのクリア
void Variable::clear()
{
	char **p;

	if(m_pList != NULL) {
		for (int i = 0; i < m_nMaxSize; ++i) {
			p = m_pList + i;
			if (*p) {
				free(*p);
				*p = NULL;
			}
		}
	}
	m_nMaxSize = 0;
}

// データ数の取得
int Variable::size()
{
	return m_nMaxSize;
}

// データの登録
Variable::set(char *pStr, int nIdx = 0)
{
	char **pList;
	int nRet = FALSE;
	int nExpand = TRUE;

	pList = GetList(nIdx, nExpand);
	if (pList) {
		if (*pList) free(*pList);
		*pList = strdup(pStr);
		nRet = TRUE;
	}

	return nRet;
}

// データの追加
Variable::add(char *pStr)
{
	return set(pStr, m_nMaxSize);
}

// データの追加
Variable::add(char *pStr, int nLen)
{
	char *pReg;
	int nRet;

	pReg = (char *)malloc(nLen + 1);
	memcpy(pReg, pStr, nLen);
	pReg[nLen] = '\0';
	nRet = set(pReg, m_nMaxSize);
	free(pReg);

	return nRet;
}

// データの挿入
Variable::ins(char *pStr, int nIdx)
{
	int nRet, nIns;

	nIns = AdjIdx(nIdx);
	if(nIns < 0) {
		if (m_nMaxSize == 0 && nIdx == 0) {
			// リストが空の場合にindex=0への挿入
			nRet = set(pStr, 0);
		} else {
			// Idx値の異常
			nRet = FALSE;
		}
	} else {
		nRet = add(pStr);
		if (nRet) {
			nRet = move(-1, nIns);
			if (!nRet) {
				del(-1);
			}
		}
	}

	return nRet;
}

// データの取得
char *Variable::get(int nIdx)
{
	char **pList, *pStr;

	pList = GetList(nIdx, FALSE);
	if (pList) {
		pStr = *pList;
	} else {
		pStr = NULL;
	}

	return pStr;
}

// データの移動
Variable::move(int nOrgIdx, int nDstIdx)
{
	char *pSave, *pBuf;
	char **pList, **pOrg, **pDst;
	int nOrg, nDst;
	int	nSize;

	// Idx値の正常性チェック
	nOrg = AdjIdx(nOrgIdx);
	nDst = AdjIdx(nDstIdx);
	if(nOrg < 0 || nDst < 0) return FALSE;

	// 移動元データ退避
	pList = GetList(nOrg, FALSE);
	pSave = *pList;

	/* データの移動 */
	if(nDst < nOrg) {
		pOrg = GetList(nDst, FALSE);
		pDst = GetList(nDst + 1, FALSE);
		nSize = sizeof(char *) * (nOrg - nDst);
		pBuf = new char[nSize];
		memcpy(pBuf, (char *)pOrg, nSize);
		memcpy((char *)pDst, pBuf, nSize);
		delete pBuf;

		*pOrg = pSave;
	} else if (nOrg < nDst) {
		pOrg = GetList(nOrg + 1, FALSE);
		pDst = GetList(nOrg, FALSE);
		nSize = sizeof(char *) * (nDst - nOrg);
		pBuf = new char[nSize];
		memcpy(pBuf, (char *)pOrg, nSize);
		memcpy((char *)pDst, pBuf, nSize);
		delete pBuf;

		pList = GetList(nDst, FALSE);
		*pList = pSave;
	}

	return TRUE;
}

// データの削除
Variable::del(int nIdx)
{
	int nDel;
	char **pList;

	// Idx値の正常性チェック
	nDel = AdjIdx(nIdx);
	if(nDel < 0) return FALSE;

	move(nDel, -1);
	pList = GetList(-1, FALSE);
	if (*pList) {
		free(*pList);
		*pList = NULL;
		--m_nMaxSize;
	}

	return TRUE;
}

// データの検索
int Variable::index(char *pStr)
{
	char *pData;
	int nRet = -1;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pData = get(i);
		if (!strcmp(pStr, pData)) {
			nRet = i;
			break;
		}
	}

	return nRet;
}

// データの結合
char *Variable::join(char *pSep)
{
	char *pTxtBuf, *pData;
	int i, nIdx, nSize, nLen, nSepLen;

	nSepLen = strlen(pSep);
	nSize = 0;
	for (i = 0; i < m_nMaxSize; ++i) {
		pData = get(i);
		nLen = strlen(pData);
		nSize += (nLen + nSepLen);
	}

	pTxtBuf = new char[nSize + 1];
	pTxtBuf[0] = NULL;
	for (i = 0; i < m_nMaxSize; ++i) {
		if (i > 0) strcat(pTxtBuf, pSep);

		pData = get(i);
		strcat(pTxtBuf, pData);
	}

	return pTxtBuf;
}

// 文字列の分解
int Variable::split(char *pStr, char *pDlmt, int nCnt)
{
	return RegexMatch(pStr, pDlmt, nCnt, TRUE);
}

int Variable::split(char *pStr, char *pDlmt)
{
	return RegexMatch(pStr, pDlmt, 0, TRUE);
}

// 文字列のパターンマッチ
Variable::match(char *pStr, char *pRegex)
{
	return RegexMatch(pStr, pRegex, 1, FALSE);
}

// 文字列のスキャン
Variable::scan(char *pStr, char *pRegex)
{
	return RegexMatch(pStr, pRegex, 0, FALSE);
}

// 文字列の正規表現マッチ
// [引数]
//  pStr   - 検索される文字列
//  pRegex - 検索する正規表現
//  nCnt   - 結果を返却するデータ数(0:データ数指定なし)
//  vRvs   - TRUE:マッチしなかったデータを返却 , FALSE:マッチしたデータを返却
// [戻り値]
//  正規表現の括弧によるキャプチャ数
Variable::RegexMatch(char *pStr, char *pRegex, int nCnt, int bRvs)
{
#define REGEX_PTN_MAX	30
    regex_t PtnReg; // 正規表現オブジェクト
    regmatch_t PtnMatch[REGEX_PTN_MAX]; // 結果を格納する
	int nLen, nAddCnt, nMchCnt, nUnMchTop, nPreUnMch, nLastNext;
	char *pSrchTop, *pMchTop;

	// データのクリア
	clear();

	// 正規表現のコンパイル
	//if (regcomp(&PtnReg, pRegex, REG_EXTENDED|REG_NEWLINE) != 0)
	if (regcomp(&PtnReg, pRegex, REG_NEWLINE) != 0) {
		return -1;	// 正規表現のコンパイルに失敗
	}

	// 正規表現による検索
	nMchCnt = 0;
	pSrchTop = pStr;
	while (*pSrchTop) {
		nUnMchTop = 0;
		nPreUnMch = 0;
		if (regexec(&PtnReg, pSrchTop, REGEX_PTN_MAX, PtnMatch, 0) == 0) {
			nAddCnt = 0;
			for (int i = 0; i < REGEX_PTN_MAX; ++i) {
				if (PtnMatch[i].rm_so >= 0 && PtnMatch[i].rm_eo >= 0) {
					if (bRvs == FALSE) {
						// マッチした文字列のデータ追加
						pMchTop = pSrchTop + (int)PtnMatch[i].rm_so;
						nLen = (int)PtnMatch[i].rm_eo - (int)PtnMatch[i].rm_so;
						add(pMchTop, nLen);
						++nAddCnt;
					} else {
						// マッチしていない文字列のデータ追加
						nLen = (int)PtnMatch[i].rm_so - nUnMchTop;
						if (nLen >= 0) {
							add(pSrchTop + nUnMchTop, nLen);
							nPreUnMch = nUnMchTop;
							nUnMchTop = (int)PtnMatch[i].rm_eo;
							++nAddCnt;
						}
					}
					nLastNext = (int)PtnMatch[i].rm_eo;
				}
			}
			++nMchCnt;
		} else {
			// マッチしない場合は終了
			break;
		}

		if (nMchCnt == nCnt) {
			// 指定データ数のマッチで終了
			break;
		} else {
			// 最終マッチした次の文字列から再検索
			pSrchTop += nLastNext;
		}
	}

	if (bRvs == TRUE) {
		if (nMchCnt == 0) {
			// 1つもマッチしない場合
			add(pSrchTop);
		} else if (nMchCnt == nCnt) {
			// 指定データ数で終了した場合
			set(pSrchTop + nPreUnMch, -1);
		} else {
			//add(pSrchTop + nUnMchTop);
			add(pSrchTop);
		}
	}

	// オブジェクトのメモリ開放
	regfree(&PtnReg);

	if (nMchCnt > 1 && nAddCnt > 1 && bRvs == FALSE) {
		for (int i = (nAddCnt * (nMchCnt - 1)); i >= 0; i -= nAddCnt) {
			del(i);
		}
		--nAddCnt;
	}

	return nAddCnt;
}

// 文字列の置換
char *Variable::gsub(char *pOrgStr, char *pRegex, char *pReplace)
{
#define REGEX_PTN_MAX	30
    regex_t PtnReg; // 正規表現オブジェクト
    regmatch_t PtnMatch[REGEX_PTN_MAX]; // 結果を格納する
	int nLen, nMchCnt, nLastNext;
	char *pSrchTop, *pMchTop, *pRegRep;
	char szKey[MAX_BUF];
	Variable OutBuf, RepBuf, MchBuf;

	// 正規表現のコンパイル
	if (regcomp(&PtnReg, pRegex, REG_NEWLINE) != 0) {
		return NULL;	// 正規表現のコンパイルに失敗
	}

	// 正規表現による検索
	pSrchTop = pOrgStr;
	while (*pSrchTop) {
		if (regexec(&PtnReg, pSrchTop, REGEX_PTN_MAX, PtnMatch, 0) == 0) {
			for (nMchCnt = 0; nMchCnt < REGEX_PTN_MAX; ++nMchCnt) {
				if (PtnMatch[nMchCnt].rm_so < 0 ||
					PtnMatch[nMchCnt].rm_eo < 0) break;

				nLastNext = (int)PtnMatch[nMchCnt].rm_eo;
			}

			// マッチした文字列の前までデータ追加
			OutBuf.add(pSrchTop, (int)PtnMatch[0].rm_so);

			if (nMchCnt == 1) {
				// 置換え文字列データ追加
				OutBuf.add(pReplace);
			} else {
				// マッチした文字列を置換え文字列の中に埋め込む
				RepBuf.clear();
				RepBuf.add(pReplace);
				for (int i = nMchCnt - 1; i > 0; --i) {
					pMchTop = pSrchTop + (int)PtnMatch[i].rm_so;
					nLen = (int)PtnMatch[i].rm_eo - (int)PtnMatch[i].rm_so;
					MchBuf.clear();
					MchBuf.add(pMchTop, nLen);

					sprintf(szKey, "\\\\%d", i);
					RepBuf.gsub(szKey, MchBuf.get(0));
				}
				// 置換え文字列データ追加
				OutBuf.add(RepBuf.get(0));
			}
		} else {
			// マッチしない残りのデータ追加して終了
			OutBuf.add(pSrchTop);
			break;
		}

		// 最終マッチした次の文字列から再検索
		pSrchTop += nLastNext;
	}

	// オブジェクトのメモリ開放
	regfree(&PtnReg);

	return OutBuf.join("");
}

// データの文字列置換
Variable::gsub(char *pRegex, char *pReplace)
{
	char *pData, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pData = get(i);
		pRet = gsub(pData, pRegex, pReplace);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

// データの重複削除
int Variable::uniq()
{
	int i, nIdx;
	char *pData;
	Variable Save;

	for (int i = 0; i < m_nMaxSize; ++i) {
		pData = get(i);
		nIdx = Save.index(pData);
		if (nIdx == -1) {
			Save.add(pData);
		} else {
			del(i);
			--i;
		}
	}

	return m_nMaxSize;
}


// クイックソート
Variable::QuickSort(char **pData, int nTop, int nBottom, int bAsc, int bLogical)
{
	int nIdx, i, nStat;
	char *pBuf;
	wchar_t *pIdx, *pTop;

	if(nTop >= nBottom) return TRUE;

	nIdx = nTop;
	for(i = nTop + 1; i <= nBottom; ++i) {
		if (bLogical == TRUE) {
			nStat = StrCmpLogical(*(pData + i), *(pData + nTop));
		} else {
			nStat = strcmp(*(pData + i), *(pData + nTop));
		}
		if (bAsc == FALSE) nStat *= -1;
		if(nStat < 0) {
			nIdx = nIdx + 1;

			pBuf = *(pData + nIdx);
			*(pData + nIdx) = *(pData +  i);
			*(pData + i) = pBuf;
		}
	}

	pBuf = *(pData + nIdx);
	*(pData + nIdx) = *(pData + nTop);
	*(pData + nTop) = pBuf;

	QuickSort(pData, nTop, nIdx - 1, bAsc, bLogical);
	QuickSort(pData, nIdx + 1, nBottom, bAsc, bLogical);

	return TRUE;
}

// データのソート
Variable::sort(int bAsc, int bLogical)
{
	return QuickSort(m_pList, 0, m_nMaxSize - 1, bAsc, bLogical);
}

Variable::sort(int bAsc)
{
	return QuickSort(m_pList, 0, m_nMaxSize - 1, bAsc, FALSE);
}

Variable::sort()
{
	return QuickSort(m_pList, 0, m_nMaxSize - 1, TRUE, FALSE);
}

// 自然順ソート用比較判定
Variable::StrCmpLogical(char *pStr1, char *pStr2)
{
	Variable CmpDat1, CmpDat2;
	int nSize1, nSize2, nSize, nIdx, nStat = 0;
	char *pArg1, *pArg2;

	// 自然順ソート用比較データ作成
	MakCmpDat(pStr1, &CmpDat1);
	MakCmpDat(pStr2, &CmpDat2);

	// 配列数の少ない方を取得
	nSize1 = CmpDat1.size();
	nSize2 = CmpDat2.size();
	nSize = (nSize1 < nSize2) ? nSize1 : nSize2;

	for (nIdx = 0; nIdx < nSize; ++nIdx) {
		pArg1 = CmpDat1.get(nIdx);
		pArg2 = CmpDat2.get(nIdx);

		if (isdigit(*pArg1)) {
			if (isdigit(*pArg2)) {
				// 両方数字の場合は数値に変換した差分を求める
				nStat = atoi(pArg1) - atoi(pArg2);
			} else {
				// 数字と文字列の場合は文字列が大きいと判断する
				nStat = -1;
			}
		} else {
			if (isdigit(*pArg2)) {
				// 数字と文字列の場合は文字列が大きいと判断する
				nStat = 1;
			} else {
				// 両方文字列の場合は文字列比較する
				nStat = strcmp(pArg1, pArg2);
			}
		}

		// 差分がある場合ループを抜ける
		if (nStat != 0) break;
	}

	if (nStat == 0) {
		// 差分がない場合は配列数で判定
		nStat = nSize1 - nSize2;
	}

	return nStat;
}

// 自然順ソート用比較データ作成
void Variable::MakCmpDat(char *pStr, Variable *pDat)
{
	int nIdx, nWrt, nStat, nPos, nCode;
	char *pEdit;
	Variable Edit;

	Edit.set(pStr);

	// 全角英数字であれば半角/大文字に変換して設定
	Edit.StrConv(CONV_UPPER + CONV_NARROW_ASCII);

	// コード順不一致文字であれば入替て設定
	Edit.tr("上下前後", "下上後前");

	// 連続する数字とそれ以外に分解
	pEdit = Edit.get(0);
	pDat->clear();
	pDat->scan(pEdit, "([^\\d]+|\\d+|.*)");
}

// 文字の範囲内位置取得
// [引数]
//  pOrg - チェックする文字
//  pRng - 判定する文字範囲
//         > リスト/範囲指定の1文字に対応(ex. \-\\abcd-zA-Z0-9)
//         > チェックする文字と全角/半角は一致すること
// [戻り値]
//  範囲内:先頭からの位置(0～)
//  範囲外:-1
int Variable::GetRngPos(char *pOrg, char *pRng)
{
	int nOrgVal, nSttVal, nEndVal, nRet = -1;
	int nOrgLen, nCurLen, nStat, nCurIdx = 0;
	char *pCurPos;

	nOrgVal = CODE_VAL(pOrg);
	nOrgLen = CHAR_LEN(pOrg);
	pCurPos = pRng;
	while (*pCurPos) {
		if (*pCurPos == '\\') ++pCurPos;

		nSttVal = CODE_VAL(pCurPos);
		nCurLen = CHAR_LEN(pCurPos);
		nStat = -1;
		if (nOrgLen == nCurLen) {	// 比較する文字の全角/半角が同一
			nStat = nOrgVal - nSttVal;
		}

		if (nStat == 0) {
			// 一致
			nRet = nCurIdx;
			break;
		} else if (nStat > 0) {
			// 範囲内の可能性あり
			pCurPos += nCurLen;
			if (*pCurPos == '-') {
				++pCurPos;
				nEndVal = CODE_VAL(pCurPos);
				if (nOrgVal <= nEndVal) {
					// 範囲内
					nRet = nCurIdx + (nOrgVal - nSttVal);
					break;
				} else {
					// 範囲外
					pCurPos += CHAR_LEN(pCurPos);
					nCurIdx += (nEndVal - nSttVal);
				}
			}
			++nCurIdx;
		} else {
			// 次
			pCurPos += nCurLen;
			++nCurIdx;
			// 範囲指定チェック
			if (*pCurPos == '-') {
				++pCurPos;
				nEndVal = CODE_VAL(pCurPos);
				pCurPos += CHAR_LEN(pCurPos);
				nCurIdx += (nEndVal - nSttVal);
			}
		}
	}

	return nRet;
}

// 範囲内の1文字取得
// [引数]
//  pRng - 文字範囲
//         > リスト/範囲指定(ex. \-\\abcd-zA-Z0-9)
//  nIdx - 文字範囲内の位置(0～)
// [戻り値]
//  範囲内:1文字のコード(ASCII or SJIS)
//  範囲外:0
int Variable::GetRngChar(char *pRng, int nIdx)
{
	int nSttVal, nEndVal, nRet = 0;
	int nStat, nCurIdx = 0;
	char *pCurPos;

	pCurPos = pRng;
	while (*pCurPos) {
		if (*pCurPos == '\\') ++pCurPos;

		nSttVal = CODE_VAL(pCurPos);
		pCurPos += CHAR_LEN(pCurPos);
		if (nCurIdx == nIdx) {
			// 一致
			nRet = nSttVal;
			break;
		} else if (*pCurPos == '-') {
			++pCurPos;
			nEndVal =  CODE_VAL(pCurPos);
			if (nCurIdx <= nIdx && nIdx <= (nCurIdx + nEndVal - nSttVal)) {
				// 範囲内
				nRet = nSttVal + (nIdx - nCurIdx);
				break;
			} else {
				// 範囲外
				pCurPos += CHAR_LEN(pCurPos);
				nCurIdx += (nEndVal - nSttVal);
			}
		}
		++nCurIdx;
	}

	return nRet;
}

// 文字リストによる置換
// [引数]
//  pStr  - 置換前文字列
//  pFrom - 置換前リスト(ex. \-\\abcd-zA-Z0-9)
//  pTo   - 置換後リスト(ex. \-\\abcd-zA-Z0-9)
// [戻り値]
//  置換後文字列
char *Variable::tr(char *pStr, char *pFrom, char *pTo)
{
	char *pOrg, szChar[MAX_BUF];
	int nPos, nCode, nLen;
	Variable Save;

	pOrg = pStr;
	while (*pOrg) {
		nLen = CHAR_LEN(pOrg);

		// 置換前リストの検索
		nPos = GetRngPos(pOrg, pFrom);
		if (nPos >= 0) {
			// 置換後リストから文字取得
			nCode = GetRngChar(pTo, nPos);
			if (nCode <= 0xff) {
				szChar[0] = nCode;
				szChar[1] = '\0';
			} else {
				szChar[0] = nCode >> 8;
				szChar[1] = nCode & 0xff;
				szChar[2] = '\0';
			}
			Save.add(szChar);
		} else {
			Save.add(pOrg, nLen);
		}
		pOrg += nLen;
	}

	return Save.join("");
}

// データの文字リストによる置換
Variable::tr(char *pFrom, char *pTo)
{
	char *pStr, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pStr = get(i);
		pRet = tr(pStr, pFrom, pTo);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

// CSVリストによる置換
// [引数]
//  pStr  - 置換前文字列
//  pFrom - 置換前リスト(ex. aa,bb,cc)
//  pTo   - 置換後リスト(ex. AA,BB,CC)
// [戻り値]
//  置換後文字列
char *Variable::CsvTr(char *pStr, char *pFrom, char *pTo)
{
	char *pOrg, *pCmp, *pCnv;
	int i, nIdx, nLen;
	Variable From, To, Save;

	// From,Toリストを分解
	From.split(pFrom, ",");
	To.split(pTo, ",");

	pOrg = pStr;
	while (*pOrg) {
		// Fromリストの検索
		nIdx = -1;
		for (i = 0; i < From.size(); ++i) {
			pCmp = From.get(i);
			nLen = strlen(pCmp);
			if (!strncmp(pOrg, pCmp, nLen)) {
				nIdx = i;
				break;
			}
		}

		// FromリストにあればToリストの文字を取得
		pCnv = NULL;
		if (nIdx != -1) {
			pCnv = To.get(nIdx);
		}

		if (pCnv) {
			// Toリストの文字を保存
			Save.add(pCnv);
		} else {
			// 元の1文字を保存
			nLen = CHAR_LEN(pOrg);
			Save.add(pOrg, nLen);
		}
		pOrg += nLen;
	}

	return Save.join("");
}

// データのCSVリストによる置換
Variable::CsvTr(char *pFrom, char *pTo)
{
	char *pStr, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pStr = get(i);
		pRet = CsvTr(pStr, pFrom, pTo);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

#define LOWER_ALPHA	"a-zａ-ｚ"
#define UPPER_ALPHA	"A-ZＡ-Ｚ"
#define HAN_ASCII	"a-zA-Z0-9@ !\"\"#$%&'()*+,\\-./:;<=>?[\\\\]^_`{|}~"
#define ZEN_ASCII	"ａ-ｚＡ-Ｚ０-９＠　！”“＃＄％＆’（）＊＋，－．／：；＜＝＞？［￥］＾＿‘｛｜｝～"
#define ZEN_KANA1	"ガ,ギ,グ,ゲ,ゴ,ザ,ジ,ズ,ゼ,ゾ,ダ,ヂ,ヅ,デ,ド,バ,ビ,ブ,ベ,ボ,パ,ピ,プ,ペ,ポ,ヴ"
#define HAN_KANA2	"｡｢｣､･ｦｧｨｩｪｫｬｭｮｯｰｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝﾞﾟ"
#define HAN_KANA1	"ｶﾞ,ｷﾞ,ｸﾞ,ｹﾞ,ｺﾞ,ｻﾞ,ｼﾞ,ｽﾞ,ｾﾞ,ｿﾞ,ﾀﾞ,ﾁﾞ,ﾂﾞ,ﾃﾞ,ﾄﾞ,ﾊﾞ,ﾋﾞ,ﾌﾞ,ﾍﾞ,ﾎﾞ,ﾊﾟ,ﾋﾟ,ﾌﾟ,ﾍﾟ,ﾎﾟ,ｳﾞ"
#define ZEN_KANA2	"。「」、・ヲァィゥェォャュョッーアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワン゛゜"

// 文字種変換 
char *Variable::StrConv(char *pOrgStr, int nConv)
{
	Variable Edit;

	Edit.set(pOrgStr);

	if ((nConv & 0x3) == CONV_UPPER) {
		// 大文字変換
		Edit.tr(LOWER_ALPHA, UPPER_ALPHA);
	} else if ((nConv & 0x3) == CONV_LOWER) {
		// 小文字変換
		Edit.tr(UPPER_ALPHA, LOWER_ALPHA);
	}

	if ((nConv & 0xc) == CONV_WIDE_ASCII) {
		// 英数字の全角変換
		Edit.tr(HAN_ASCII, ZEN_ASCII);
	}
	if ((nConv & 0xc) == CONV_WIDE_KANA) {
		// カタカナの全角変換
		Edit.CsvTr(HAN_KANA1, ZEN_KANA1);
		Edit.tr(HAN_KANA2, ZEN_KANA2);
	}

	if ((nConv & 0x30) == CONV_NARROW_ASCII) {
		// 英数字の半角変換
		Edit.tr(ZEN_ASCII, HAN_ASCII);
	}
	if ((nConv & 0x30) == CONV_NARROW_KANA) {
		// カタカナの半角変換
		Edit.CsvTr(ZEN_KANA1, HAN_KANA1);
		Edit.tr(ZEN_KANA2, HAN_KANA2);
	}

	return Edit.join("");
}

// データの文字種変換
Variable::StrConv(int nConv)
{
	char *pStr, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pStr = get(i);
		pRet = StrConv(pStr, nConv);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

// メタ文字→制御コード種変換
char *Variable::MetaConv(char *pOrgStr)
{
	Variable Edit;
	char *pBuf, *pRef;
	int nPos1, nPos2, nVal1, nVal2;

	// タブ、改行、改ページを変換
	pBuf = Edit.CsvTr(pOrgStr, "\\t,\\n,\\r,\\f", "\t,\r\n,\r,\f");

	pRef = pBuf;
	while (*pRef) {
		if (*pRef == '\\') {
			if (*(pRef + 1) == 'x') {
				// 16進数表記の変換
				nPos1 = GetRngPos(pRef + 2, "0-9a-fA-F");
				nPos2 = GetRngPos(pRef + 3, "0-9a-fA-F");
				if (nPos1 >= 0 && nPos2 >= 0) {
					nVal1 = nPos1 < 16 ? nPos1 : nPos1 - 6;
					nVal2 = nPos2 < 16 ? nPos2 : nPos2 - 6;
					*pRef = 16 * nVal1 + nVal2;
					strcpy(pRef + 1, pRef + 4);
				}
			} else if (*pRef == 'c') {
				// Ctrl表記の変換
				nPos1 = GetRngPos(pRef + 2, "A-Z");
				if (nPos1 >= 0) {
					*pRef = nPos1 + 1;
					strcpy(pRef + 1, pRef + 3);
				}
			}
		}

		pRef += CHAR_LEN(pRef);
	}

	return pBuf;
}

// データのメタ文字→制御コード種変換
Variable::MetaConv()
{
	char *pStr, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pStr = get(i);
		pRet = MetaConv(pStr);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

// 文字列の分解
// [引数]
//  pStr      - 分解対象文字列
//  pDlmt     - デリミタ[1Byte文字のみ指定可]
//  bDlmtRept - 指定デリミタ内の1文字が連続する場合を1つの区切りとする
//  bWQuot    - ダブルクォートで囲まれた値を塊とみなす
// [戻り値]
//  分解した数
int Variable::StrSplit(char *pStr, char *pDlmt, int bDlmtRept = FALSE, int bWQuot = FALSE)
{
	int nIdx = 0,				// 分解する文字列の参照位置
		nTop,					// 分解する各カラムの先頭位置
		nLen,					// カラム文字列の長さ
		nDlmtLen;				// デリミタの長さ
#define PID_SRCH_DLMT	0		// デリミタ検索中
#define PID_SRCH_CHAR	1		// デリミタ以外検索中
#define PID_SRCH_QUOTE	2		// ダブルクォート検索中
	int nPid = PID_SRCH_DLMT,	// 処理種別
		nRetPid,				// ダブルクォート発見時の処理種別
		nAdjUpd = 0;			// 参照位置更新の補正バイト数
	int nAddCnt = 0,			// リストへの出力データ数
		bEndFlg = FALSE;		// 分解処理の終了表示
	char *pRet;

	// データのクリア
	clear();

	// 検索するデリミタの長さ設定
	if (bDlmtRept == FALSE) {
		nDlmtLen = strlen(pDlmt);	// 1デリミタで分解
	} else {
		nDlmtLen = 1;	// 連続するデリミタ内の文字で分解
	}

	nTop = nIdx;
	while (bEndFlg == FALSE) {
		// 文字列内のデリミタを判定
		if (bDlmtRept == FALSE) {
			pRet = NULL;
			if (pStr[nIdx] == '\0' ||
			    strncmp(pStr + nIdx, pDlmt, nDlmtLen) == 0) pRet = pDlmt;
		} else {
			pRet = strchr(pDlmt, pStr[nIdx]);
		}

		if (nPid == PID_SRCH_DLMT) {
			// デリミタ検索中の場合
			if (pStr[nIdx] == '\"' && bWQuot == TRUE) {
				// ダブルクォート処理対象の場合
				// 処理種別を保存し、ダブルクォートを検索する
				nRetPid = nPid;
				nPid = PID_SRCH_QUOTE;
			} else if (pRet) {
				// デリミタを発見した場合
				nAddCnt = 1;
				if (pStr[nIdx]) nAdjUpd = nDlmtLen - 1;

				// デリミタ以外を検索する
				nPid = PID_SRCH_CHAR;
			}
		} else if (nPid == PID_SRCH_CHAR) {
			// デリミタ以外検索中の場合
			if (pRet == NULL) { // デリミタ以外か？
				nTop = nIdx;

				// デリミタを検索する
				nPid = PID_SRCH_DLMT;

				if (pStr[nIdx] == '\"' && bWQuot == TRUE) {
					// ダブルクォートの処理対象の場合
					// 処理種別を保存し、ダブルクォートを検索する
					nRetPid = nPid;
					nPid = PID_SRCH_QUOTE;
				}
			} else {
				if (bDlmtRept == FALSE) { // 1デリミタで分解
					nTop = nIdx;
					nAddCnt = 1;

					nAdjUpd = nDlmtLen - 1;
				}
			}
		} else if (nPid == PID_SRCH_QUOTE) {
			// ダブルクォート検索中の場合
			if (pStr[nIdx] == '\"') {
				// ダブルクォートの場合
				if (pStr[nIdx+1] == '\"') {
					// ダブルクォートが連続する場合
					// 参照位置を1バイト余分に更新
					nAdjUpd = 1;
				} else {
					// 処理種別を元に戻す
					nPid = nRetPid;
				}
			}
		}

		// 行単位の文字列を登録
		if (nAddCnt) {
			nLen = nIdx - nTop;
			add(pStr + nTop, nLen);

			if (bWQuot == TRUE) {
				// ダブルクォート処理の場合
				// 前後のダブルクォート削除
				gsub("^\"|\"$", "");
				// ダブルクォート2つ→1つへ変換
				gsub("\"\"", "\"");
			}

			nTop = nIdx;
			nAddCnt = 0;
		}

		if (pStr[nIdx]) {
			// 参照位置更新
			if (IsKanji(pStr[nIdx])) {
				nIdx += 2;
			} else {
				nIdx += (1 + nAdjUpd);
				nAdjUpd = 0;
			}
		} else {
			bEndFlg = TRUE;
		}
	}

	return m_nMaxSize;
}

// ワイルドカード文字列判定
int Variable::WildSrch(char *pOrgStr, char *pWldStr, int *lpStt, int *lpLen)
{
#define NOMATCH (-1)
#define SEARCH	0
#define BEGIN	1
#define MATCH	2
	int		nStat,			// 状態表示 (0:検査中 1:一致中 2:一致 -1:不一致)
			nMchPos = 0,	// 一致開始位置
			bMchFlg,		// 一致状態
			bSpcFlg,		// 特殊文字表示
			nSubTop,		// 再帰の時の開始位置
			nSubLen;		// 再帰の時の文字列の長さ
	char	*pOrgTop,		// 検査文字列の先頭位置
			*pWldTop;		// ワイルドカード先頭位置

	// 初期状態設定
	if (*pOrgStr) {
		nStat = SEARCH;
	} else {
		nStat = NOMATCH;
	}
	// 検査文字列，ワイルドカードの先頭位置保存
	pOrgTop = pOrgStr;
	pWldTop = pWldStr;

	while (nStat == SEARCH || nStat == BEGIN) { // 判定終了まで
		// 特殊文字表示クリア
		bSpcFlg = 0;
		switch (*pWldStr) { // ワイルドカード文字
			case '*':
				// ワイルドカード文字を１つ進める
				++pWldStr;
				if (*pWldStr == '\0') { // '*'は最終文字か？
					// 検査文字列の一致開始位置を設定
					*lpStt = nMchPos;
					*lpLen = strlen(pOrgTop + nMchPos);
					// 状態＝一致
					nStat = MATCH;
				} else {
					if (WildSrch(pOrgStr, pWldStr, &nSubTop, &nSubLen)==TRUE) {
						// 検査文字列の一致開始位置を設定
						*lpStt = nMchPos;
						*lpLen = (pOrgStr - pOrgTop) - nMchPos 
						             + nSubTop + nSubLen;
						// 状態＝一致
						nStat = MATCH;
					}
				}
				break;

			case '?':
				if (*pOrgStr == '\0') { // 検査文字列は終了か？
					// 状態表示＝不一致
					nStat = NOMATCH;
				} else {
					// 状態表示＝一致中
					nStat = BEGIN;
				}
				// 検査文字とワイルドカード文字を１つ進める
				pOrgStr += CHAR_LEN(pOrgStr);
				++pWldStr;
				break;

			case '\0': // Null
				if (nStat == SEARCH) { // 状態は検査中か？
					// 状態＝不一致
					nStat = NOMATCH;
				} else {
					// 検査文字列の一致開始位置を設定
					*lpStt = nMchPos;
					*lpLen = (pOrgStr - pOrgTop) - nMchPos;
					// 状態＝一致
					nStat = MATCH;
				}
				break;

			case '\\':
				// 特殊文字表示を設定する
				bSpcFlg = 1;
				; // No BREAK

			default: // 通常文字
				// 文字一致判定
				bMchFlg = FALSE;
				if (IsKanji(*pOrgStr)) { // 漢字か？
					if (*pOrgStr == *(pWldStr + bSpcFlg) &&
						*(pOrgStr + 1) == *(pWldStr + bSpcFlg + 1)) {
						bMchFlg = TRUE;
					}
				} else {
					if (*pOrgStr == *(pWldStr + bSpcFlg)) bMchFlg = TRUE;
				}

				if (bMchFlg) { // 文字一致か？
					// 状態が検査中であれば、状態＝一致中
					if (nStat == SEARCH) nStat = BEGIN;
					// 検査文字とワイルドカード文字を１つ進める
					pOrgStr += CHAR_LEN(pOrgStr);
					pWldStr += CHAR_LEN(pWldStr);
					pWldStr += bSpcFlg;
				} else {
					if (nStat == BEGIN) { // 状態は一致中か？
						// 状態＝検査中
						nStat = SEARCH;
						// 検査文字を一致開始位置に戻す
						pOrgStr = pOrgTop + nMchPos;
						// ワイルドカード文字を先頭に戻す
						pWldStr = pWldTop;
					}
					// 検査文字を１つ進める
					pOrgStr += CHAR_LEN(pOrgStr);
				}
				break;
		}

		if (nStat == SEARCH) { // 状態は検査中か？
			// 一致開始位置を検査位置にする
			nMchPos = pOrgStr - pOrgTop;
			if (*pOrgStr == '\0') { // 検査文字終了か？
				// 状態＝不一致
				nStat = NOMATCH;
			}
		}
	}

	if (nStat == MATCH) { // 一致か？
		return TRUE; // 一致
	} else {
		return FALSE; // 不一致
	}
}

// ワイルドカード文字による文字列の置き換え
char *Variable::wsub(char *pOrgStr, char *pSrch, char *pReplace)
{
	int	nIdx = 0, nStat;
	int nStart, nMchLen;
	char *pMSrch, *pMReplace;
	Variable Conv;

	pMSrch = MetaConv(pSrch);
	pMReplace = MetaConv(pReplace);

	nStat = WildSrch(pOrgStr + nIdx, pMSrch, &nStart, &nMchLen);
	while (nStat) {
		Conv.add(pOrgStr + nIdx, nStart);
		Conv.add(pMReplace);
		nIdx += (nStart + nMchLen);

		nStat = WildSrch(pOrgStr + nIdx, pMSrch, &nStart, &nMchLen);
	}
	Conv.add(pOrgStr + nIdx);

	return Conv.join("");
}

// データのワイルドカード文字による文字列の置き換え
Variable::wsub(char *pSrch, char *pReplace)
{
	char *pData, *pRet;

	for(int i = 0; i < m_nMaxSize; ++i) {
		pData = get(i);
		pRet = wsub(pData, pSrch, pReplace);
		if (pRet) {
			set(pRet, i);
			free(pRet);
		}
	}

	return TRUE;
}

// デバッグ表示
void Variable::DspList()
{
	int i;
	char *p;

	printf("\n*** DspList() ***\n");
	printf("nMaxArea: %d\n", m_nMaxArea);
	printf("nMaxSize: %d\n", m_nMaxSize);
	printf("pList   : %08X\n", m_pList);
	for (i = 0; i < m_nMaxArea; ++i) {
		p = *(m_pList + i);
		printf("%2d:%08X -> ", i, p);
		if (p) {
			printf("\"%s\"\n", p);
		} else {
			printf("\n");
		}
	}
	printf("\n");
}

#if DEBUG
// テスト関数
main(int argc, char **argv)
{
	Variable list, drv;
	char ope[1024], p1[1024], p2[1024], p3[1024], p4[1024];
	char *out;
	int flg = TRUE, bat = FALSE;
	int ret;

	if (argc > 1) {
		bat = TRUE;
		if (!strcmp(argv[1], "split") && argc >= 4) {
			ret = list.split(argv[2], argv[3], atoi(argv[4]));
			printf("split() => %d\n", ret);
		} else if (!strcmp(argv[1], "match") && argc >= 3) {
			ret = list.match(argv[2], argv[3]);
			printf("match() => %d\n", ret);
		} else if (!strcmp(argv[1], "scan") && argc >= 3) {
			ret = list.scan(argv[2], argv[3]);
			printf("scan() => %d\n", ret);
		} else if (!strcmp(argv[1], "gsub") && argc >= 4) {
			out = list.gsub(argv[2], argv[3], argv[4]);
			printf("gsub() => \"%s\"\n", out);
			free(out);
		} else if (!strcmp(argv[1], "tr") && argc >= 4) {
			out = list.tr(argv[2], argv[3], argv[4]);
			printf("tr() => \"%s\"\n", out);
			free(out);
		} else if (!strcmp(argv[1], "strconv") && argc >= 3) {
			out = list.StrConv(argv[2], atoi(argv[3]));
			printf("StrConv() => \"%s\"\n", out);
			free(out);
		} else if (!strcmp(argv[1], "metaconv") && argc >= 2) {
			out = list.MetaConv(argv[2]);
			printf("MetaConv() => \"%s\"\n", out);
			free(out);
		} else {
			bat = FALSE;
		}

		if (bat == TRUE)	list.DspList();
	}

	if (bat == FALSE) {
		do {
			printf("Cmd: "); gets(ope);

			if (!strcmp(ope, "add")) {
				printf("String: "); gets(p1);
				list.add(p1);
			} else if (!strcmp(ope, "set")) {
				printf("String: "); gets(p1);
				printf("Index: ");  gets(p2);
				list.set(p1, atoi(p2));
			} else if (!strcmp(ope, "clear")) {
				list.clear();
			} else if (!strcmp(ope, "index")) {
				printf("String: "); gets(p1);
				ret = list.index(p1);
				printf("index(%s) => %d\n", p1, ret);
			} else if (!strcmp(ope, "split")) {
				printf("String: "); gets(p1);
				printf("Delimter: "); gets(p2);
				printf("Count: "); gets(p3);
				ret = list.split(p1, p2, atoi(p3));
				printf("split() => %d\n", ret);
			} else if (!strcmp(ope, "join")) {
				printf("Separetor: "); gets(p1);
				out = list.join(p1);
				printf("join(%s) => \"%s\"\n", p1, out);
				free(out);
			} else if (!strcmp(ope, "match")) {
				printf("String: "); gets(p1);
				printf("Pattern: "); gets(p2);
				ret = list.match(p1, p2);
				printf("match() => %d\n", ret);
			} else if (!strcmp(ope, "scan")) {
				printf("String: "); gets(p1);
				printf("Pattern: "); gets(p2);
				ret = list.scan(p1, p2);
				printf("scan() => %d\n", ret);
			} else if (!strcmp(ope, "move")) {
				printf("OrgIndex: "); gets(p1);
				printf("DstIndex: "); gets(p2);
				ret = list.move(atoi(p1), atoi(p2));
				printf("move() => %d\n", ret);
			} else if (!strcmp(ope, "del")) {
				printf("DelIndex: "); gets(p1);
				ret = list.del(atoi(p1));
				printf("del() => %d\n", ret);
			} else if (!strcmp(ope, "ins")) {
				printf("String: "); gets(p1);
				printf("Index: "); gets(p2);
				ret = list.ins(p1, atoi(p2));
				printf("ins() => %d\n", ret);
			} else if (!strcmp(ope, "sort")) {
				printf("AscFlg: "); gets(p1);
				printf("LogicalFlg: "); gets(p2);
				list.sort(atoi(p1), atoi(p2));
			} else if (!strcmp(ope, "uniq")) {
				ret = list.uniq();
				printf("uniq() => %d\n", ret);
			} else if (!strcmp(ope, "gsub")) {
				printf("String: "); gets(p1);
				printf("Pattern: "); gets(p2);
				printf("Replace: "); gets(p3);
				if (*p1) {
					out = list.gsub(p1, p2, p3);
					printf("gsub() => \"%s\"\n", out);
					free(out);
				} else {
					list.gsub(p2, p3);
				}
			} else if (!strcmp(ope, "tr")) {
				printf("String: "); gets(p1);
				printf("From-List: "); gets(p2);
				printf("To-List: "); gets(p3);
				if (*p1) {
					out = list.tr(p1, p2, p3);
					printf("tr() => \"%s\"\n", out);
					free(out);
				} else {
					list.tr(p2, p3);
				}
			} else if (!strcmp(ope, "csvtr")) {
				printf("String: "); gets(p1);
				printf("From-List: "); gets(p2);
				printf("To-List: "); gets(p3);
				if (*p1) {
					out = list.CsvTr(p1, p2, p3);
					printf("CsvTr() => \"%s\"\n", out);
					free(out);
				} else {
					list.CsvTr(p2, p3);
				}
			} else if (!strcmp(ope, "strconv")) {
				printf("String: "); gets(p1);
				printf("Conv(1:U, 2:L, 4:Ａ, 8:ア, 16:A, 32:ｱ): "); gets(p2);
				if (*p1) {
					out = list.StrConv(p1, atoi(p2));
					printf("StrConv() => \"%s\"\n", out);
					free(out);
				} else {
					list.StrConv(atoi(p2));
				}
			} else if (!strcmp(ope, "metaconv")) {
				printf("String: "); gets(p1);
				if (*p1) {
					out = list.MetaConv(p1);
					printf("MetaConv() => \"%s\"\n", out);
					free(out);
				} else {
					list.MetaConv();
				}
			} else if (!strcmp(ope, "strsplit")) {
				printf("String: "); gets(p1);
				printf("Delimter: "); gets(p2);
				printf("Rept: "); gets(p3);
				printf("WQuot: "); gets(p4);
				ret = list.StrSplit(p1, p2, atoi(p3), atoi(p4));
				printf("StrSplit() => %d\n", ret);
			} else if (!strcmp(ope, "wsub")) {
				printf("String: "); gets(p1);
				printf("Pattern: "); gets(p2);
				printf("Replace: "); gets(p3);
				if (*p1) {
					out = list.wsub(p1, p2, p3);
					printf("wsub() => \"%s\"\n", out);
					free(out);
				} else {
					list.wsub(p2, p3);
				}
			} else if (!strcmp(ope, "exit")) {
				flg = FALSE;
			}
			list.DspList();
		} while (flg == TRUE);
	}
}
#endif
