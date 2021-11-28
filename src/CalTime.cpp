/*
 * 時間の加減算に関する処理
 */

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "CalTime.h"
#include "Variable.h"

static int nMaxDay[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
static int   nBasYear = 1900;
static double dBasHour = (1.0/24);
static double dBasMin  = (1.0/24/60);
static double dBasSec  = (1.0/24/60/60);
static double dRound   = (1.0/24/60/60/2);

// 時間文字列→BINデータ変換
double Date2Bin(char *pStr)
{
	Variable Edit;
	char *pDlmt;
	int nYear = 0, nMon = 0, nDay = 0;
	int nHour = 0, nMin = 0, nSec = 0;
	double dBin;

	// 時間文字列の解析
	Edit.scan(pStr, "[^0-9]+");
	pDlmt = Edit.join("");
	Edit.scan(pStr, "[0-9]+");

	if (!strcmp(pDlmt, "//")) {
		nYear = atoi(Edit.get(0));
		nMon  = atoi(Edit.get(1));
		nDay  = atoi(Edit.get(2));
	} else if (!strcmp(pDlmt, "// ::")) {
		nYear = atoi(Edit.get(0));
		nMon  = atoi(Edit.get(1));
		nDay  = atoi(Edit.get(2));
		nHour = atoi(Edit.get(3));
		nMin  = atoi(Edit.get(4));
		nSec  = atoi(Edit.get(5));
	} else if (!strcmp(pDlmt, "// :")) {
		nYear = atoi(Edit.get(0));
		nMon  = atoi(Edit.get(1));
		nDay  = atoi(Edit.get(2));
		nHour = atoi(Edit.get(3));
		nMin  = atoi(Edit.get(4));
	} else if (!strcmp(pDlmt, ":")) {
		nHour = atoi(Edit.get(0));
		nMin  = atoi(Edit.get(1));
	} else if (!strcmp(pDlmt, "::")) {
		nHour = atoi(Edit.get(0));
		nMin  = atoi(Edit.get(1));
		nSec  = atoi(Edit.get(2));
	}
	free(pDlmt);

	if (nYear > 0) {
		// うるう年判定して、2月の最終日を設定
		if (ChkLeap(nYear)) {
			nMaxDay[2] = 29;
		} else {
			nMaxDay[2] = 28;
		}

		// 1900年から算出前年までの日数算出
		dBin = 365 * (nYear - nBasYear)
				+ LeapDays(nYear - 1) - LeapDays(nBasYear - 1);

		// 算出前月までの日数を加算
		for (int i = 1; i < nMon; ++i)	dBin += nMaxDay[i];

		// 算出月の日数を加算
		dBin += nDay;
	}

	// 時間を加算
	dBin += (nHour * dBasHour + nMin * dBasMin + nSec * dBasSec);

	return dBin;
}

// BINデータ→時間文字列変換
void Bin2Date(double dBin, char *pStr)
{
	int nYear, nMon, nDay;
	int nHour, nMin, nSec;
	double dDays, dTimes;

	// 整数部と小数部を分離
	dDays = int(dBin);
	dTimes = dBin - dDays;

	// 時/分/秒の算出
	nHour = int(dTimes / dBasHour + dRound);
	nMin  = int((dTimes - nHour * dBasHour) / dBasMin + dRound);
	nSec  = int((dTimes - nHour * dBasHour - nMin * dBasMin) / dBasSec + dRound);

	// 年の算出
	nYear = int(dDays / (365 + 0.25 - 0.01 + 0.0025)) + nBasYear;

	// 月日の算出
	dDays = dDays - 365 * (nYear - nBasYear)
			- LeapDays(nYear - 1) + LeapDays(nBasYear - 1);

	if (dDays <= 0) {
		--nYear;
		dDays += (365 + ChkLeap(nYear));
	}

	if (ChkLeap(nYear)) {
		nMaxDay[2] = 29;
	} else {
		nMaxDay[2] = 28;
	}
	nMon = 1;
	while (dDays > nMaxDay[nMon]) {
		dDays -= nMaxDay[nMon];
		++nMon;
	}
	nDay = dDays;

	sprintf(pStr, "%d/%d/%d %d:%02d:%02d", nYear, nMon, nDay, nHour, nMin, nSec);
}

// うるう年の日数
int LeapDays(int nYear)
{
	return ((nYear/4) - (nYear/100) + (nYear/400));
}

// うるう年のチェック
int ChkLeap(int nYear)
{
	if (((nYear % 4 == 0) && (nYear % 100 != 0)) || (nYear % 400 == 0)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#if DEBUG
main(int argc, char **argv)
{
	double dBin = 0, dArg;
	char cOpe = 0, szDate[256];

	if (argc < 2) {
		printf("Usage : CalTime Arg1 Ope Arg2\n");
		printf("          Ope … +:加算、-:減算\n");
		printf("          Arg … YYYY/M/D h:mm:ss\n");
		return 1;
	}

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "+")) {
			cOpe = '+';
		} else if (!strcmp(argv[i], "-")) {
			cOpe = '-';
		} else {
			if (cOpe == '+') {
				dArg = Date2Bin(argv[i]);
				dBin += dArg;
			} else if (cOpe == '-') {
				dArg = Date2Bin(argv[i]);
				dBin -= dArg;
			} else {
				dBin = Date2Bin(argv[i]);
			}
		}
	}

	Bin2Date(dBin, szDate);
	printf("%s\n", szDate);

	return 0;
}
#endif
