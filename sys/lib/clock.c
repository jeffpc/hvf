#include <clock.h>

#define LS_NONE			0x00	/* non-leap second entry */
#define LS_LS			0x10	/* previous second was leap */

#define LS_MASK			(LS_LS)

#define LS_YM2DATE(y, m, f)	((y) << 8 | (f) | (m))
#define LS_DATE2Y(d)		((d) >> 8)
#define LS_DATE2M(d)		((d) & 0xf)
#define LS_DATE2F(d)		((d) & LS_MASK)

struct leap_sec_info {
	u64 tod;
	u32 date;
};

/*
 * List of TOD clocks for each of the leap seconds
 */
static struct leap_sec_info leap_secs[] = {
	{ 0xC3870CB9BB600000ULL, LS_YM2DATE(2009, 1, LS_LS  ), },
	{ 0xBE251097973C0000ULL, LS_YM2DATE(2006, 1, LS_LS  ), },
	{ 0xB1962F9305180000ULL, LS_YM2DATE(1999, 1, LS_LS  ), },
	{ 0xAEE3EFA402F40000ULL, LS_YM2DATE(1997, 7, LS_LS  ), },
	{ 0xAC34336FECD00000ULL, LS_YM2DATE(1996, 1, LS_LS  ), },
	{ 0xA981F380EAAC0000ULL, LS_YM2DATE(1994, 7, LS_LS  ), },
	{ 0xA7B70ABEB8880000ULL, LS_YM2DATE(1993, 7, LS_LS  ), },
	{ 0xA5EC21FC86640000ULL, LS_YM2DATE(1992, 7, LS_LS  ), },
	{ 0xA33C65C870400000ULL, LS_YM2DATE(1991, 1, LS_LS  ), },
	{ 0xA1717D063E1C0000ULL, LS_YM2DATE(1990, 1, LS_LS  ), },
	{ 0x9DDA69A557F80000ULL, LS_YM2DATE(1988, 1, LS_LS  ), },
	{ 0x995D40F517D40000ULL, LS_YM2DATE(1985, 7, LS_LS  ), },
	{ 0x95C62D9431B00000ULL, LS_YM2DATE(1983, 7, LS_LS  ), },
	{ 0x93FB44D1FF8C0000ULL, LS_YM2DATE(1982, 7, LS_LS  ), },
	{ 0x92305C0FCD680000ULL, LS_YM2DATE(1981, 7, LS_LS  ), },
	{ 0x8F809FDBB7440000ULL, LS_YM2DATE(1980, 1, LS_LS  ), },
	{ 0x8DB5B71985200000ULL, LS_YM2DATE(1979, 1, LS_LS  ), },
	{ 0x8BEACE5752FC0000ULL, LS_YM2DATE(1978, 1, LS_LS  ), },
	{ 0x8A1FE59520D80000ULL, LS_YM2DATE(1977, 1, LS_LS  ), },
	{ 0x8853BAF578B40000ULL, LS_YM2DATE(1976, 1, LS_LS  ), },
	{ 0x8688D23346900000ULL, LS_YM2DATE(1975, 1, LS_LS  ), },
	{ 0x84BDE971146C0000ULL, LS_YM2DATE(1974, 1, LS_LS  ), },
	{ 0x82F300AEE2480000ULL, LS_YM2DATE(1973, 1, LS_LS  ), },
	{ 0x820BA9811E240000ULL, LS_YM2DATE(1972, 7, LS_LS  ), },
	{ 0x8126D60E46000000ULL, LS_YM2DATE(1972, 1, LS_NONE), }, /* UTC begins */
	{ 0x0000000000000000ULL, LS_YM2DATE(1900, 1, LS_NONE), }, /* TOD begins */
};

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
	int add_sec;
	int leap;
	int done;
	int i,j;

	/*
	 * find the most recent info for this tod (i), as well as for one
	 * that's one second ahead (j)
	 */
	for (i=0; leap_secs[i].tod > tod; i++)
		;
	for (j=0; leap_secs[j].tod > (tod + CLK_SEC); j++)
		;

	tod -= leap_secs[i].tod;

	dt->dy = LS_DATE2Y(leap_secs[i].date);
	dt->dm = LS_DATE2M(leap_secs[i].date);

	/*
	 * If we're in the leap second, let's pretend we're a second
	 * earlier, and then add it back in at the end of the function
	 */
	add_sec = 0;

	if ((i != j) && (LS_DATE2F(leap_secs[j].date) == LS_LS)) {
		tod -= CLK_SEC;
		add_sec = 1;
	}

	/* The month should be 1 or 7! */
	BUG_ON((dt->dm != 1) && (dt->dm != 7));

	/*
	 * At this point, we have a TOD value to add to a struct datetime.
	 * We don't have to worry about leap seconds.  Life is good.
	 */

	for(;;) {
		if (leap_year((dt->dm == 1) ? dt->dy : (dt->dy+1)))
			done = tod < (CLK_YEAR + CLK_DAY);
		else
			done = tod < CLK_YEAR;

		if (done)
			break;

		if ((dt->dm == 1 && leap_year(dt->dy)) ||
		    (dt->dm == 7 && leap_year(dt->dy+1)))
			tod -= (CLK_YEAR + CLK_DAY);
		else
			tod -= CLK_YEAR;

		dt->dy++;
	}

	/*
	 * At this point, we have the year and we know the month is either Jan
	 * or Jul; let's get an actual month
	 */

	dt->dd = tod / CLK_DAY;
	tod -= dt->dd * CLK_DAY;

	do {
		leap = leap_year(dt->dy);
		i = month_offsets[leap][dt->dm - 1];

		while((dt->dm < 12) &&
		      ((i + dt->dd) >= month_offsets[leap][dt->dm]))
			dt->dm++;

		dt->dd = dt->dd + i - month_offsets[leap][dt->dm-1];

		if ((dt->dm < 12) || (dt->dd < 31))
			break;

		dt->dy++;
		dt->dm = 1;
		dt->dd -= 31;
	} while(1);

	dt->dd++;

	/*
	 * At this point, we have the year, the month, and even the day;
	 * let's get the hours, minutes and seconds
	 */

	dt->th = tod / CLK_HOUR;
	tod -= dt->th * CLK_HOUR;

	dt->tm = tod / CLK_MIN;
	tod -= dt->tm * CLK_MIN;

	dt->ts = tod / CLK_SEC;
	tod -= dt->ts * CLK_SEC;

	/*
	 * If we borrowed a second before, then add it back! (We borrow only
	 * for leap seconds in progress.)
	 */
	dt->ts += add_sec;

	dt->tmicro = (u32) (tod >> 12);

	return dt;
}
