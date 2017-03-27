
#include "cmdlib.h"
#include "mathlib.h"
#include "bsplib.h"
#include "vstdlib/icommandline.h"

void main (int argc, char **argv)
{
	int			i;
	char		source[1024];
	int			size;
	FILE		*f;

	CommandLine()->CreateCmdLine( argc, argv );

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	if (argc == 1)
		Error ("usage: vbspinfo bspfile [bspfiles]");
		
	for (i=1 ; i<argc ; i++)
	{
		printf ("---------------------\n");
		strcpy (source, argv[i]);
		DefaultExtension (source, ".bsp");
		f = fopen (source, "rb");
		if (f)
		{
			fseek( f, 0, SEEK_END );
			size = ftell( f );
			fclose (f);
		}
		else
			size = 0;
		printf ("%s: %i\n", source, size);
		
		LoadBSPFile (source);		
		PrintBSPFileSizes ();
		printf ("---------------------\n");
	}
}
