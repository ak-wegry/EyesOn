/*
 * ���Ԃ̉����Z�Ɋւ��鏈���̒�`
 */

#ifndef CALTIME_INCLUDED
#define CALTIME_INCLUDED

double Date2Bin(char *pStr);
void Bin2Date(double dBin, char *pStr);
int LeapDays(int nYear);
int ChkLeap(int nYear);

#endif
