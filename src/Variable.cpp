/*
 * Variable�N���X�̒�`
 *======================================================================
 *[�ύX����]
 * Ver1.00  2019/05/04 �V�K�쐬
 * Ver1.01  2020/05/10 ��̃��X�g��ins()�o���Ȃ��s��C��
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

// �R���X�g���N�^
Variable::Variable()
{
	m_pList = NULL;
	m_nMaxSize = 0;
	m_nMaxArea = 0;
}

// �f�X�g���N�^
Variable::~Variable()
{
	clear();
	if(m_pList != NULL) {
		free((char *)m_pList);
		m_pList = NULL;
	}
	m_nMaxArea = 0;
}

// �e�[�u���̃|�C���^�擾
char **Variable::GetList(int nIdx, int bExpand)
{
	char **pList, *pNew;
	int nBlkCnt;	// ���������m�ۂ���1�u���b�N��
	int nSize;		// 1���X�g�̃������T�C�Y
	int nCnv;		// nIdx���}�C�i�X���̕␳�l

	nBlkCnt = 10;
	nSize = sizeof(char *);
	if(nIdx < m_nMaxSize) {
		// �o�^�f�[�^�̃|�C���^�ݒ�
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
			// �̈�g���Ȃ��̏ꍇ�ANULL�|�C���^�ݒ�
			pList = NULL;
		} else {
			if(m_nMaxArea <= nIdx) {
				// �V�K�f�[�^�̈�̊m��
				if(m_nMaxSize == 0) {
					m_pList = (char **)new char[nBlkCnt * nSize];
				} else {
					pNew = new char[(m_nMaxArea + nBlkCnt) * nSize];
					memcpy(pNew, m_pList, m_nMaxSize * nSize);
					delete m_pList;
					m_pList = (char **)pNew;
				}
				// �g���f�[�^�̈�̃N���A
				memset(m_pList + m_nMaxArea, 0, nBlkCnt * nSize);

				// �m�ۃf�[�^�̈搔�̍X�V
				m_nMaxArea += nBlkCnt;
			}

			// �o�^�f�[�^�̃|�C���^�ݒ�
			pList = m_pList + nIdx;

			// �o�^�f�[�^���̍X�V
			m_nMaxSize = nIdx + 1;
		}
	}

	return pList;
}

// ���X�g�̃C���f�b�N�X�l�␳
int Variable::AdjIdx(int nIdx)
{
	int nCnv, nRet = -1;

	// �}�C�i�X�̃C���f�b�N�X�l�␳
	if (nIdx < 0) {
		nCnv = m_nMaxSize + nIdx;
	} else {
		nCnv = nIdx;
	}

	// �C���f�b�N�X�l�̐��픻��
	if (0 <= nCnv && nCnv < m_nMaxSize) {
		nRet = nCnv;
	}

	return nRet;
}

// SJIS�̊�������
int Variable::IsKanji(unsigned char c)
{
	if((c >= 0x80 && c < 0xA0) || c >= 0xE0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

// �f�[�^�̃N���A
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

// �f�[�^���̎擾
int Variable::size()
{
	return m_nMaxSize;
}

// �f�[�^�̓o�^
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

// �f�[�^�̒ǉ�
Variable::add(char *pStr)
{
	return set(pStr, m_nMaxSize);
}

// �f�[�^�̒ǉ�
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

// �f�[�^�̑}��
Variable::ins(char *pStr, int nIdx)
{
	int nRet, nIns;

	nIns = AdjIdx(nIdx);
	if(nIns < 0) {
		if (m_nMaxSize == 0 && nIdx == 0) {
			// ���X�g����̏ꍇ��index=0�ւ̑}��
			nRet = set(pStr, 0);
		} else {
			// Idx�l�ُ̈�
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

// �f�[�^�̎擾
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

// �f�[�^�̈ړ�
Variable::move(int nOrgIdx, int nDstIdx)
{
	char *pSave, *pBuf;
	char **pList, **pOrg, **pDst;
	int nOrg, nDst;
	int	nSize;

	// Idx�l�̐��퐫�`�F�b�N
	nOrg = AdjIdx(nOrgIdx);
	nDst = AdjIdx(nDstIdx);
	if(nOrg < 0 || nDst < 0) return FALSE;

	// �ړ����f�[�^�ޔ�
	pList = GetList(nOrg, FALSE);
	pSave = *pList;

	/* �f�[�^�̈ړ� */
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

// �f�[�^�̍폜
Variable::del(int nIdx)
{
	int nDel;
	char **pList;

	// Idx�l�̐��퐫�`�F�b�N
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

// �f�[�^�̌���
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

// �f�[�^�̌���
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

// ������̕���
int Variable::split(char *pStr, char *pDlmt, int nCnt)
{
	return RegexMatch(pStr, pDlmt, nCnt, TRUE);
}

int Variable::split(char *pStr, char *pDlmt)
{
	return RegexMatch(pStr, pDlmt, 0, TRUE);
}

// ������̃p�^�[���}�b�`
Variable::match(char *pStr, char *pRegex)
{
	return RegexMatch(pStr, pRegex, 1, FALSE);
}

// ������̃X�L����
Variable::scan(char *pStr, char *pRegex)
{
	return RegexMatch(pStr, pRegex, 0, FALSE);
}

// ������̐��K�\���}�b�`
// [����]
//  pStr   - ��������镶����
//  pRegex - �������鐳�K�\��
//  nCnt   - ���ʂ�ԋp����f�[�^��(0:�f�[�^���w��Ȃ�)
//  vRvs   - TRUE:�}�b�`���Ȃ������f�[�^��ԋp , FALSE:�}�b�`�����f�[�^��ԋp
// [�߂�l]
//  ���K�\���̊��ʂɂ��L���v�`����
Variable::RegexMatch(char *pStr, char *pRegex, int nCnt, int bRvs)
{
#define REGEX_PTN_MAX	30
    regex_t PtnReg; // ���K�\���I�u�W�F�N�g
    regmatch_t PtnMatch[REGEX_PTN_MAX]; // ���ʂ��i�[����
	int nLen, nAddCnt, nMchCnt, nUnMchTop, nPreUnMch, nLastNext;
	char *pSrchTop, *pMchTop;

	// �f�[�^�̃N���A
	clear();

	// ���K�\���̃R���p�C��
	//if (regcomp(&PtnReg, pRegex, REG_EXTENDED|REG_NEWLINE) != 0)
	if (regcomp(&PtnReg, pRegex, REG_NEWLINE) != 0) {
		return -1;	// ���K�\���̃R���p�C���Ɏ��s
	}

	// ���K�\���ɂ�錟��
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
						// �}�b�`����������̃f�[�^�ǉ�
						pMchTop = pSrchTop + (int)PtnMatch[i].rm_so;
						nLen = (int)PtnMatch[i].rm_eo - (int)PtnMatch[i].rm_so;
						add(pMchTop, nLen);
						++nAddCnt;
					} else {
						// �}�b�`���Ă��Ȃ�������̃f�[�^�ǉ�
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
			// �}�b�`���Ȃ��ꍇ�͏I��
			break;
		}

		if (nMchCnt == nCnt) {
			// �w��f�[�^���̃}�b�`�ŏI��
			break;
		} else {
			// �ŏI�}�b�`�������̕����񂩂�Č���
			pSrchTop += nLastNext;
		}
	}

	if (bRvs == TRUE) {
		if (nMchCnt == 0) {
			// 1���}�b�`���Ȃ��ꍇ
			add(pSrchTop);
		} else if (nMchCnt == nCnt) {
			// �w��f�[�^���ŏI�������ꍇ
			set(pSrchTop + nPreUnMch, -1);
		} else {
			//add(pSrchTop + nUnMchTop);
			add(pSrchTop);
		}
	}

	// �I�u�W�F�N�g�̃������J��
	regfree(&PtnReg);

	if (nMchCnt > 1 && nAddCnt > 1 && bRvs == FALSE) {
		for (int i = (nAddCnt * (nMchCnt - 1)); i >= 0; i -= nAddCnt) {
			del(i);
		}
		--nAddCnt;
	}

	return nAddCnt;
}

// ������̒u��
char *Variable::gsub(char *pOrgStr, char *pRegex, char *pReplace)
{
#define REGEX_PTN_MAX	30
    regex_t PtnReg; // ���K�\���I�u�W�F�N�g
    regmatch_t PtnMatch[REGEX_PTN_MAX]; // ���ʂ��i�[����
	int nLen, nMchCnt, nLastNext;
	char *pSrchTop, *pMchTop, *pRegRep;
	char szKey[MAX_BUF];
	Variable OutBuf, RepBuf, MchBuf;

	// ���K�\���̃R���p�C��
	if (regcomp(&PtnReg, pRegex, REG_NEWLINE) != 0) {
		return NULL;	// ���K�\���̃R���p�C���Ɏ��s
	}

	// ���K�\���ɂ�錟��
	pSrchTop = pOrgStr;
	while (*pSrchTop) {
		if (regexec(&PtnReg, pSrchTop, REGEX_PTN_MAX, PtnMatch, 0) == 0) {
			for (nMchCnt = 0; nMchCnt < REGEX_PTN_MAX; ++nMchCnt) {
				if (PtnMatch[nMchCnt].rm_so < 0 ||
					PtnMatch[nMchCnt].rm_eo < 0) break;

				nLastNext = (int)PtnMatch[nMchCnt].rm_eo;
			}

			// �}�b�`����������̑O�܂Ńf�[�^�ǉ�
			OutBuf.add(pSrchTop, (int)PtnMatch[0].rm_so);

			if (nMchCnt == 1) {
				// �u����������f�[�^�ǉ�
				OutBuf.add(pReplace);
			} else {
				// �}�b�`�����������u����������̒��ɖ��ߍ���
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
				// �u����������f�[�^�ǉ�
				OutBuf.add(RepBuf.get(0));
			}
		} else {
			// �}�b�`���Ȃ��c��̃f�[�^�ǉ����ďI��
			OutBuf.add(pSrchTop);
			break;
		}

		// �ŏI�}�b�`�������̕����񂩂�Č���
		pSrchTop += nLastNext;
	}

	// �I�u�W�F�N�g�̃������J��
	regfree(&PtnReg);

	return OutBuf.join("");
}

// �f�[�^�̕�����u��
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

// �f�[�^�̏d���폜
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


// �N�C�b�N�\�[�g
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

// �f�[�^�̃\�[�g
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

// ���R���\�[�g�p��r����
Variable::StrCmpLogical(char *pStr1, char *pStr2)
{
	Variable CmpDat1, CmpDat2;
	int nSize1, nSize2, nSize, nIdx, nStat = 0;
	char *pArg1, *pArg2;

	// ���R���\�[�g�p��r�f�[�^�쐬
	MakCmpDat(pStr1, &CmpDat1);
	MakCmpDat(pStr2, &CmpDat2);

	// �z�񐔂̏��Ȃ������擾
	nSize1 = CmpDat1.size();
	nSize2 = CmpDat2.size();
	nSize = (nSize1 < nSize2) ? nSize1 : nSize2;

	for (nIdx = 0; nIdx < nSize; ++nIdx) {
		pArg1 = CmpDat1.get(nIdx);
		pArg2 = CmpDat2.get(nIdx);

		if (isdigit(*pArg1)) {
			if (isdigit(*pArg2)) {
				// ���������̏ꍇ�͐��l�ɕϊ��������������߂�
				nStat = atoi(pArg1) - atoi(pArg2);
			} else {
				// �����ƕ�����̏ꍇ�͕����񂪑傫���Ɣ��f����
				nStat = -1;
			}
		} else {
			if (isdigit(*pArg2)) {
				// �����ƕ�����̏ꍇ�͕����񂪑傫���Ɣ��f����
				nStat = 1;
			} else {
				// ����������̏ꍇ�͕������r����
				nStat = strcmp(pArg1, pArg2);
			}
		}

		// ����������ꍇ���[�v�𔲂���
		if (nStat != 0) break;
	}

	if (nStat == 0) {
		// �������Ȃ��ꍇ�͔z�񐔂Ŕ���
		nStat = nSize1 - nSize2;
	}

	return nStat;
}

// ���R���\�[�g�p��r�f�[�^�쐬
void Variable::MakCmpDat(char *pStr, Variable *pDat)
{
	int nIdx, nWrt, nStat, nPos, nCode;
	char *pEdit;
	Variable Edit;

	Edit.set(pStr);

	// �S�p�p�����ł���Δ��p/�啶���ɕϊ����Đݒ�
	Edit.StrConv(CONV_UPPER + CONV_NARROW_ASCII);

	// �R�[�h���s��v�����ł���Γ��ւĐݒ�
	Edit.tr("�㉺�O��", "�����O");

	// �A�����鐔���Ƃ���ȊO�ɕ���
	pEdit = Edit.get(0);
	pDat->clear();
	pDat->scan(pEdit, "([^\\d]+|\\d+|.*)");
}

// �����͈͓̔��ʒu�擾
// [����]
//  pOrg - �`�F�b�N���镶��
//  pRng - ���肷�镶���͈�
//         > ���X�g/�͈͎w���1�����ɑΉ�(ex. \-\\abcd-zA-Z0-9)
//         > �`�F�b�N���镶���ƑS�p/���p�͈�v���邱��
// [�߂�l]
//  �͈͓�:�擪����̈ʒu(0�`)
//  �͈͊O:-1
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
		if (nOrgLen == nCurLen) {	// ��r���镶���̑S�p/���p������
			nStat = nOrgVal - nSttVal;
		}

		if (nStat == 0) {
			// ��v
			nRet = nCurIdx;
			break;
		} else if (nStat > 0) {
			// �͈͓��̉\������
			pCurPos += nCurLen;
			if (*pCurPos == '-') {
				++pCurPos;
				nEndVal = CODE_VAL(pCurPos);
				if (nOrgVal <= nEndVal) {
					// �͈͓�
					nRet = nCurIdx + (nOrgVal - nSttVal);
					break;
				} else {
					// �͈͊O
					pCurPos += CHAR_LEN(pCurPos);
					nCurIdx += (nEndVal - nSttVal);
				}
			}
			++nCurIdx;
		} else {
			// ��
			pCurPos += nCurLen;
			++nCurIdx;
			// �͈͎w��`�F�b�N
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

// �͈͓���1�����擾
// [����]
//  pRng - �����͈�
//         > ���X�g/�͈͎w��(ex. \-\\abcd-zA-Z0-9)
//  nIdx - �����͈͓��̈ʒu(0�`)
// [�߂�l]
//  �͈͓�:1�����̃R�[�h(ASCII or SJIS)
//  �͈͊O:0
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
			// ��v
			nRet = nSttVal;
			break;
		} else if (*pCurPos == '-') {
			++pCurPos;
			nEndVal =  CODE_VAL(pCurPos);
			if (nCurIdx <= nIdx && nIdx <= (nCurIdx + nEndVal - nSttVal)) {
				// �͈͓�
				nRet = nSttVal + (nIdx - nCurIdx);
				break;
			} else {
				// �͈͊O
				pCurPos += CHAR_LEN(pCurPos);
				nCurIdx += (nEndVal - nSttVal);
			}
		}
		++nCurIdx;
	}

	return nRet;
}

// �������X�g�ɂ��u��
// [����]
//  pStr  - �u���O������
//  pFrom - �u���O���X�g(ex. \-\\abcd-zA-Z0-9)
//  pTo   - �u���ナ�X�g(ex. \-\\abcd-zA-Z0-9)
// [�߂�l]
//  �u���㕶����
char *Variable::tr(char *pStr, char *pFrom, char *pTo)
{
	char *pOrg, szChar[MAX_BUF];
	int nPos, nCode, nLen;
	Variable Save;

	pOrg = pStr;
	while (*pOrg) {
		nLen = CHAR_LEN(pOrg);

		// �u���O���X�g�̌���
		nPos = GetRngPos(pOrg, pFrom);
		if (nPos >= 0) {
			// �u���ナ�X�g���當���擾
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

// �f�[�^�̕������X�g�ɂ��u��
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

// CSV���X�g�ɂ��u��
// [����]
//  pStr  - �u���O������
//  pFrom - �u���O���X�g(ex. aa,bb,cc)
//  pTo   - �u���ナ�X�g(ex. AA,BB,CC)
// [�߂�l]
//  �u���㕶����
char *Variable::CsvTr(char *pStr, char *pFrom, char *pTo)
{
	char *pOrg, *pCmp, *pCnv;
	int i, nIdx, nLen;
	Variable From, To, Save;

	// From,To���X�g�𕪉�
	From.split(pFrom, ",");
	To.split(pTo, ",");

	pOrg = pStr;
	while (*pOrg) {
		// From���X�g�̌���
		nIdx = -1;
		for (i = 0; i < From.size(); ++i) {
			pCmp = From.get(i);
			nLen = strlen(pCmp);
			if (!strncmp(pOrg, pCmp, nLen)) {
				nIdx = i;
				break;
			}
		}

		// From���X�g�ɂ����To���X�g�̕������擾
		pCnv = NULL;
		if (nIdx != -1) {
			pCnv = To.get(nIdx);
		}

		if (pCnv) {
			// To���X�g�̕�����ۑ�
			Save.add(pCnv);
		} else {
			// ����1������ۑ�
			nLen = CHAR_LEN(pOrg);
			Save.add(pOrg, nLen);
		}
		pOrg += nLen;
	}

	return Save.join("");
}

// �f�[�^��CSV���X�g�ɂ��u��
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

#define LOWER_ALPHA	"a-z��-��"
#define UPPER_ALPHA	"A-Z�`-�y"
#define HAN_ASCII	"a-zA-Z0-9@ !\"\"#$%&'()*+,\\-./:;<=>?[\\\\]^_`{|}~"
#define ZEN_ASCII	"��-���`-�y�O-�X���@�I�h�g���������f�i�j���{�C�|�D�^�F�G�������H�m���n�O�Q�e�o�b�p�`"
#define ZEN_KANA1	"�K,�M,�O,�Q,�S,�U,�W,�Y,�[,�],�_,�a,�d,�f,�h,�o,�r,�u,�x,�{,�p,�s,�v,�y,�|,��"
#define HAN_KANA2	"���������������������������������������������������������������"
#define HAN_KANA1	"��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��,��"
#define ZEN_KANA2	"�B�u�v�A�E���@�B�D�F�H�������b�[�A�C�E�G�I�J�L�N�P�R�T�V�X�Z�\�^�`�c�e�g�i�j�k�l�m�n�q�t�w�z�}�~���������������������������J�K"

// ������ϊ� 
char *Variable::StrConv(char *pOrgStr, int nConv)
{
	Variable Edit;

	Edit.set(pOrgStr);

	if ((nConv & 0x3) == CONV_UPPER) {
		// �啶���ϊ�
		Edit.tr(LOWER_ALPHA, UPPER_ALPHA);
	} else if ((nConv & 0x3) == CONV_LOWER) {
		// �������ϊ�
		Edit.tr(UPPER_ALPHA, LOWER_ALPHA);
	}

	if ((nConv & 0xc) == CONV_WIDE_ASCII) {
		// �p�����̑S�p�ϊ�
		Edit.tr(HAN_ASCII, ZEN_ASCII);
	}
	if ((nConv & 0xc) == CONV_WIDE_KANA) {
		// �J�^�J�i�̑S�p�ϊ�
		Edit.CsvTr(HAN_KANA1, ZEN_KANA1);
		Edit.tr(HAN_KANA2, ZEN_KANA2);
	}

	if ((nConv & 0x30) == CONV_NARROW_ASCII) {
		// �p�����̔��p�ϊ�
		Edit.tr(ZEN_ASCII, HAN_ASCII);
	}
	if ((nConv & 0x30) == CONV_NARROW_KANA) {
		// �J�^�J�i�̔��p�ϊ�
		Edit.CsvTr(ZEN_KANA1, HAN_KANA1);
		Edit.tr(ZEN_KANA2, HAN_KANA2);
	}

	return Edit.join("");
}

// �f�[�^�̕�����ϊ�
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

// ���^����������R�[�h��ϊ�
char *Variable::MetaConv(char *pOrgStr)
{
	Variable Edit;
	char *pBuf, *pRef;
	int nPos1, nPos2, nVal1, nVal2;

	// �^�u�A���s�A���y�[�W��ϊ�
	pBuf = Edit.CsvTr(pOrgStr, "\\t,\\n,\\r,\\f", "\t,\r\n,\r,\f");

	pRef = pBuf;
	while (*pRef) {
		if (*pRef == '\\') {
			if (*(pRef + 1) == 'x') {
				// 16�i���\�L�̕ϊ�
				nPos1 = GetRngPos(pRef + 2, "0-9a-fA-F");
				nPos2 = GetRngPos(pRef + 3, "0-9a-fA-F");
				if (nPos1 >= 0 && nPos2 >= 0) {
					nVal1 = nPos1 < 16 ? nPos1 : nPos1 - 6;
					nVal2 = nPos2 < 16 ? nPos2 : nPos2 - 6;
					*pRef = 16 * nVal1 + nVal2;
					strcpy(pRef + 1, pRef + 4);
				}
			} else if (*pRef == 'c') {
				// Ctrl�\�L�̕ϊ�
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

// �f�[�^�̃��^����������R�[�h��ϊ�
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

// ������̕���
// [����]
//  pStr      - ����Ώە�����
//  pDlmt     - �f���~�^[1Byte�����̂ݎw���]
//  bDlmtRept - �w��f���~�^����1�������A������ꍇ��1�̋�؂�Ƃ���
//  bWQuot    - �_�u���N�H�[�g�ň͂܂ꂽ�l����Ƃ݂Ȃ�
// [�߂�l]
//  ����������
int Variable::StrSplit(char *pStr, char *pDlmt, int bDlmtRept = FALSE, int bWQuot = FALSE)
{
	int nIdx = 0,				// �������镶����̎Q�ƈʒu
		nTop,					// ��������e�J�����̐擪�ʒu
		nLen,					// �J����������̒���
		nDlmtLen;				// �f���~�^�̒���
#define PID_SRCH_DLMT	0		// �f���~�^������
#define PID_SRCH_CHAR	1		// �f���~�^�ȊO������
#define PID_SRCH_QUOTE	2		// �_�u���N�H�[�g������
	int nPid = PID_SRCH_DLMT,	// �������
		nRetPid,				// �_�u���N�H�[�g�������̏������
		nAdjUpd = 0;			// �Q�ƈʒu�X�V�̕␳�o�C�g��
	int nAddCnt = 0,			// ���X�g�ւ̏o�̓f�[�^��
		bEndFlg = FALSE;		// ���������̏I���\��
	char *pRet;

	// �f�[�^�̃N���A
	clear();

	// ��������f���~�^�̒����ݒ�
	if (bDlmtRept == FALSE) {
		nDlmtLen = strlen(pDlmt);	// 1�f���~�^�ŕ���
	} else {
		nDlmtLen = 1;	// �A������f���~�^���̕����ŕ���
	}

	nTop = nIdx;
	while (bEndFlg == FALSE) {
		// ��������̃f���~�^�𔻒�
		if (bDlmtRept == FALSE) {
			pRet = NULL;
			if (pStr[nIdx] == '\0' ||
			    strncmp(pStr + nIdx, pDlmt, nDlmtLen) == 0) pRet = pDlmt;
		} else {
			pRet = strchr(pDlmt, pStr[nIdx]);
		}

		if (nPid == PID_SRCH_DLMT) {
			// �f���~�^�������̏ꍇ
			if (pStr[nIdx] == '\"' && bWQuot == TRUE) {
				// �_�u���N�H�[�g�����Ώۂ̏ꍇ
				// ������ʂ�ۑ����A�_�u���N�H�[�g����������
				nRetPid = nPid;
				nPid = PID_SRCH_QUOTE;
			} else if (pRet) {
				// �f���~�^�𔭌������ꍇ
				nAddCnt = 1;
				if (pStr[nIdx]) nAdjUpd = nDlmtLen - 1;

				// �f���~�^�ȊO����������
				nPid = PID_SRCH_CHAR;
			}
		} else if (nPid == PID_SRCH_CHAR) {
			// �f���~�^�ȊO�������̏ꍇ
			if (pRet == NULL) { // �f���~�^�ȊO���H
				nTop = nIdx;

				// �f���~�^����������
				nPid = PID_SRCH_DLMT;

				if (pStr[nIdx] == '\"' && bWQuot == TRUE) {
					// �_�u���N�H�[�g�̏����Ώۂ̏ꍇ
					// ������ʂ�ۑ����A�_�u���N�H�[�g����������
					nRetPid = nPid;
					nPid = PID_SRCH_QUOTE;
				}
			} else {
				if (bDlmtRept == FALSE) { // 1�f���~�^�ŕ���
					nTop = nIdx;
					nAddCnt = 1;

					nAdjUpd = nDlmtLen - 1;
				}
			}
		} else if (nPid == PID_SRCH_QUOTE) {
			// �_�u���N�H�[�g�������̏ꍇ
			if (pStr[nIdx] == '\"') {
				// �_�u���N�H�[�g�̏ꍇ
				if (pStr[nIdx+1] == '\"') {
					// �_�u���N�H�[�g���A������ꍇ
					// �Q�ƈʒu��1�o�C�g�]���ɍX�V
					nAdjUpd = 1;
				} else {
					// ������ʂ����ɖ߂�
					nPid = nRetPid;
				}
			}
		}

		// �s�P�ʂ̕������o�^
		if (nAddCnt) {
			nLen = nIdx - nTop;
			add(pStr + nTop, nLen);

			if (bWQuot == TRUE) {
				// �_�u���N�H�[�g�����̏ꍇ
				// �O��̃_�u���N�H�[�g�폜
				gsub("^\"|\"$", "");
				// �_�u���N�H�[�g2��1�֕ϊ�
				gsub("\"\"", "\"");
			}

			nTop = nIdx;
			nAddCnt = 0;
		}

		if (pStr[nIdx]) {
			// �Q�ƈʒu�X�V
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

// ���C���h�J�[�h�����񔻒�
int Variable::WildSrch(char *pOrgStr, char *pWldStr, int *lpStt, int *lpLen)
{
#define NOMATCH (-1)
#define SEARCH	0
#define BEGIN	1
#define MATCH	2
	int		nStat,			// ��ԕ\�� (0:������ 1:��v�� 2:��v -1:�s��v)
			nMchPos = 0,	// ��v�J�n�ʒu
			bMchFlg,		// ��v���
			bSpcFlg,		// ���ꕶ���\��
			nSubTop,		// �ċA�̎��̊J�n�ʒu
			nSubLen;		// �ċA�̎��̕�����̒���
	char	*pOrgTop,		// ����������̐擪�ʒu
			*pWldTop;		// ���C���h�J�[�h�擪�ʒu

	// ������Ԑݒ�
	if (*pOrgStr) {
		nStat = SEARCH;
	} else {
		nStat = NOMATCH;
	}
	// ����������C���C���h�J�[�h�̐擪�ʒu�ۑ�
	pOrgTop = pOrgStr;
	pWldTop = pWldStr;

	while (nStat == SEARCH || nStat == BEGIN) { // ����I���܂�
		// ���ꕶ���\���N���A
		bSpcFlg = 0;
		switch (*pWldStr) { // ���C���h�J�[�h����
			case '*':
				// ���C���h�J�[�h�������P�i�߂�
				++pWldStr;
				if (*pWldStr == '\0') { // '*'�͍ŏI�������H
					// ����������̈�v�J�n�ʒu��ݒ�
					*lpStt = nMchPos;
					*lpLen = strlen(pOrgTop + nMchPos);
					// ��ԁ���v
					nStat = MATCH;
				} else {
					if (WildSrch(pOrgStr, pWldStr, &nSubTop, &nSubLen)==TRUE) {
						// ����������̈�v�J�n�ʒu��ݒ�
						*lpStt = nMchPos;
						*lpLen = (pOrgStr - pOrgTop) - nMchPos 
						             + nSubTop + nSubLen;
						// ��ԁ���v
						nStat = MATCH;
					}
				}
				break;

			case '?':
				if (*pOrgStr == '\0') { // ����������͏I�����H
					// ��ԕ\�����s��v
					nStat = NOMATCH;
				} else {
					// ��ԕ\������v��
					nStat = BEGIN;
				}
				// ���������ƃ��C���h�J�[�h�������P�i�߂�
				pOrgStr += CHAR_LEN(pOrgStr);
				++pWldStr;
				break;

			case '\0': // Null
				if (nStat == SEARCH) { // ��Ԃ͌��������H
					// ��ԁ��s��v
					nStat = NOMATCH;
				} else {
					// ����������̈�v�J�n�ʒu��ݒ�
					*lpStt = nMchPos;
					*lpLen = (pOrgStr - pOrgTop) - nMchPos;
					// ��ԁ���v
					nStat = MATCH;
				}
				break;

			case '\\':
				// ���ꕶ���\����ݒ肷��
				bSpcFlg = 1;
				; // No BREAK

			default: // �ʏ핶��
				// ������v����
				bMchFlg = FALSE;
				if (IsKanji(*pOrgStr)) { // �������H
					if (*pOrgStr == *(pWldStr + bSpcFlg) &&
						*(pOrgStr + 1) == *(pWldStr + bSpcFlg + 1)) {
						bMchFlg = TRUE;
					}
				} else {
					if (*pOrgStr == *(pWldStr + bSpcFlg)) bMchFlg = TRUE;
				}

				if (bMchFlg) { // ������v���H
					// ��Ԃ��������ł���΁A��ԁ���v��
					if (nStat == SEARCH) nStat = BEGIN;
					// ���������ƃ��C���h�J�[�h�������P�i�߂�
					pOrgStr += CHAR_LEN(pOrgStr);
					pWldStr += CHAR_LEN(pWldStr);
					pWldStr += bSpcFlg;
				} else {
					if (nStat == BEGIN) { // ��Ԃ͈�v�����H
						// ��ԁ�������
						nStat = SEARCH;
						// ������������v�J�n�ʒu�ɖ߂�
						pOrgStr = pOrgTop + nMchPos;
						// ���C���h�J�[�h������擪�ɖ߂�
						pWldStr = pWldTop;
					}
					// �����������P�i�߂�
					pOrgStr += CHAR_LEN(pOrgStr);
				}
				break;
		}

		if (nStat == SEARCH) { // ��Ԃ͌��������H
			// ��v�J�n�ʒu�������ʒu�ɂ���
			nMchPos = pOrgStr - pOrgTop;
			if (*pOrgStr == '\0') { // ���������I�����H
				// ��ԁ��s��v
				nStat = NOMATCH;
			}
		}
	}

	if (nStat == MATCH) { // ��v���H
		return TRUE; // ��v
	} else {
		return FALSE; // �s��v
	}
}

// ���C���h�J�[�h�����ɂ�镶����̒u������
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

// �f�[�^�̃��C���h�J�[�h�����ɂ�镶����̒u������
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

// �f�o�b�O�\��
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
// �e�X�g�֐�
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
				printf("Conv(1:U, 2:L, 4:�`, 8:�A, 16:A, 32:�): "); gets(p2);
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
