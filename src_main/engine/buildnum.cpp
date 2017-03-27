
#include "quakedef.h"
#include "vstdlib/strtools.h"

//char *date = "Sep 30 2003"; // "Oct 24 1996";
char *date = __DATE__ ;

char *mon[12] = 
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char mond[12] = 
{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

class CBuildNumber
{
public:
	CBuildNumber( void )
	{
		ComputeBuildNumber();
	}
	
	// returns days since Oct 24 1996
	int	GetBuildNumber( void ) 
	{
		return m_nBuildNumber;
	}

private:
	void		ComputeBuildNumber( void )
	{
		int m = 0; 
		int d = 0;
		int y = 0;

		for (m = 0; m < 11; m++)
		{
			if (Q_strncasecmp( &date[0], mon[m], 3 ) == 0)
				break;
			d += mond[m];
		}

		d += atoi( &date[4] ) - 1;

		y = atoi( &date[7] ) - 1900;

		m_nBuildNumber = d + (int)((y - 1) * 365.25);

		if (((y % 4) == 0) && m > 1)
		{
			m_nBuildNumber += 1;
		}

		//m_nBuildNumber -= 34995; // Oct 24 1996
		m_nBuildNumber -= 37527;  // Sep 30 2003
	}

	int			m_nBuildNumber;
};

// Singleton
static CBuildNumber g_BuildNumber;

//-----------------------------------------------------------------------------
// Purpose: Only compute build number the first time we run the app
// Output : int
//-----------------------------------------------------------------------------
int build_number( void )
{
	return g_BuildNumber.GetBuildNumber();
}
