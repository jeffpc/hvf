#include <clock.h>

/*
 * Number of days passed since the beginning of the year
 */
static const short int month_offsets[2][12] = {
	/* J    F    M    A    M    J    J    A    S    O    N    D */
	{   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334},
	{   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
};

struct datetime *parse_tod(struct datetime *dt, u64 tod)
{
	int leap;

	/* year 1900 was NOT leap */
	tod -= 4 * CLK_YEAR;

	dt->dy = 4 * (tod / CLK_FOURYEARS);
	tod -= (dt->dy / 4) * CLK_FOURYEARS;
	dt->dy += 1904;


	dt->dd = tod / CLK_DAY;
	tod -= dt->dd * CLK_DAY;

	for(dt->dm = 0, leap = leap_year(dt->dy);
	    dt->dm < 12 && dt->dd > month_offsets[leap][dt->dm];
	    dt->dm++)
		;
	/* at this point, ->dm is set to a number in 1..12 */

	/* 
	 * subtract the days expressed as months, and adjust to make first
	 * day of month day 1
	 */
	dt->dd -= month_offsets[leap][dt->dm-1] - 1;

	dt->th = tod / CLK_HOUR;
	tod -= dt->th * CLK_HOUR;

	dt->tm = tod / CLK_MIN;
	tod -= dt->tm * CLK_MIN;

	dt->ts = tod / CLK_SEC;

	dt->tmicro = (u32) (tod >> 13);

	return dt;
}
