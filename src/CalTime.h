/*
 * ŠÔ‚Ì‰ÁŒ¸Z‚ÉŠÖ‚·‚éˆ—‚Ì’è‹`
 */

#ifndef CALTIME_INCLUDED
#define CALTIME_INCLUDED

double Date2Bin(char *pStr);
void Bin2Date(double dBin, char *pStr);
int LeapDays(int nYear);
int ChkLeap(int nYear);

#endif
