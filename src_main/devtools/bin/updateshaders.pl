$depnum = 0;

sub GetAsmShaderDependencies_R
{
	local( $shadername ) = shift;
	local( *SHADER );
	open SHADER, "<$shadername";
	while( <SHADER> )
	{
		if( m/^\s*\#\s*include\s+\"(.*)\"/ )
		{
			print "$shadername depends on $1\n";
			if( !defined( $dep{$shadername} ) )
			{
				$dep{$shadername} = $1;
				GetAsmShaderDependencies_R( $1 );
			}
			else
			{
				print "circular dependency in $shadername!\n";
			}
		}
	}
	close SHADER;
}

sub GetAsmShaderDependencies
{
	local( $shadername ) = shift;
	print "-------------------------------\n";
	print "$shadername\n";
	undef %dep;
	GetAsmShaderDependencies_R( $shadername );
	local( $i );
	foreach $i ( keys( %dep ) )
	{
		print "$i depends on $dep{$i}\n";
	}
	return values( %dep );
}

sub DoAsmShader
{
	local( $shadername ) = shift;
	push @newdsp, "# Begin Source File\n\n";
	push @newdsp, "SOURCE=.\\$shadername\n";
	local( $shadertype );
	if( $shadername =~ m/\.vsh/i )
	{
		$shadertype = "vsh";
	}
	elsif( $shadername =~ m/\.psh/i )
	{
		$shadertype = "psh";
	}
	elsif( $shadername =~ m/\.fxc/i )
	{
		$shadertype = "fxc";
	}
	else
	{
		die;
	}
	foreach $config ( @configs )
	{
		if( $config eq $configs[0] )
		{
			push @newdsp, "\!IF  \"\$(CFG)\" == \"" . $config . "\"\n";
		}
		else
		{
			push @newdsp, "\!ELSEIF  \"\$(CFG)\" == \"" . $config . "\"\n";
		}
		$dx9 = "-dx9";
		$dx = "9";
		@dep = &GetAsmShaderDependencies( $shadername );

		push @newdsp, "USERDEP__" . "$depnum" . "=";
		foreach $dep ( sort( @dep ) )
		{
			push @newdsp, "\t\"$dep\"";
		}
		if( $shadername =~ m/\.fxc/i )
		{
			push @newdsp, "\t\"..\\..\\devtools\\bin\\fxc.exe\"";
			push @newdsp, "\t\"..\\..\\devtools\\bin\\fxc_prep.pl\"";
		}
		elsif( $shadername =~ m/\.vsh/i )
		{
			push @newdsp, "\t\"..\\..\\devtools\\bin\\vsa9.exe\"";
			push @newdsp, "\t\"..\\..\\devtools\\bin\\vsh_prep.pl\"";
		}
		elsif( $shadername =~ m/\.psh/i )
		{   
			push @newdsp, "\t\"..\\..\\devtools\\bin\\psa9.exe\"";
			push @newdsp, "\t\"..\\..\\devtools\\bin\\psh_prep.pl\"";
		}
		push @newdsp, "\n";

		push @newdsp, "\# Begin Custom Build\n";
		push @newdsp, "InputPath=.\\" . "$shadername\n";
		$base = $shadername;
		$base =~ s/\.$shadertype//i;
		push @newdsp, "InputName=" . $base . "\n\n";
		if( $shadername =~ m/\.fxc/i && $nv3x )
		{
			if( $config =~ /ps20/i )
			{
				push @newdsp, "\"$shadertype" . "tmp" . $dx . "_nv3x_ps20\\\$(InputName)\.inc\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n";
				push @newdsp, "\t..\\..\\devtools\\bin\\perl ..\\..\\devtools\\bin\\" . $shadertype . "_prep\.pl $dx9 -nv3x \$(InputName)\." . $shadertype . "\n\n";
			}
			else
			{
				push @newdsp, "\"$shadertype" . "tmp" . $dx . "_nv3x\\\$(InputName)\.inc\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n";
				push @newdsp, "\t..\\..\\devtools\\bin\\perl ..\\..\\devtools\\bin\\" . $shadertype . "_prep\.pl $dx9 -nv3x -ps20a \$(InputName)\." . $shadertype . "\n\n";
			}
		}
		else
		{
			push @newdsp, "\"$shadertype" . "tmp" . $dx . "\\\$(InputName)\.inc\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n";
			push @newdsp, "\t..\\..\\devtools\\bin\\perl ..\\..\\devtools\\bin\\" . $shadertype . "_prep\.pl $dx9 \$(InputName)\." . $shadertype . "\n\n";
		}
		push @newdsp, "\# End Custom Build\n";
	}
	push @newdsp, "\!ENDIF\n";
	push @newdsp, "# End Source File\n";
	$depnum++;
}

sub MakeSureFileExists
{
	local( $filename ) = shift;
	local( $testexists ) = shift;
	local( $testwrite ) = shift;

	local( @statresult ) = stat $filename;
	if( !@statresult && $testexists )
	{
		die "$filename doesn't exist!\n";
	}
	local( $mode, $iswritable );
	$mode = oct( $statresult[2] );
	$iswritable = ( $mode & 2 ) != 0;
	if( !$iswritable && $testwrite )
	{
		die "$filename isn't writable!\n";
	}
}

if( scalar( @ARGV ) == 0 )
{
	die "Usage updateshaders.pl shaderprojectbasename\n\tie: updateshaders.pl stdshaders_dx6\n";
}

while( 1 )
{
	$inputbase = shift;

	if( $inputbase =~ m/-p4edit/ )
	{
		$p4edit = 1;
	}
	elsif( $inputbase =~ m/-nv3x/ )
	{
		$nv3x = 1;
	}
	else
	{
		last;
	}
}

if( $p4edit )
{
	system "p4 edit $inputbase.cpp";
	system "p4 edit $inputbase.dsp";
}

&MakeSureFileExists( "$inputbase.dsp", 1, 1 );
&MakeSureFileExists( "$inputbase.cpp", 0, 1 );
&MakeSureFileExists( "$inputbase.txt", 1, 0 );

push @configs, "$inputbase - Win32 Release";
push @configs, "$inputbase - Win32 Debug";
if( $nv3x )
{
	push @configs, "$inputbase - Win32 Release ps20";
	push @configs, "$inputbase - Win32 Debug ps20";
}

open SHADERLISTFILE, "<$inputbase.txt" || die;
@srcfiles = <SHADERLISTFILE>;
close SHADERLISTFILE;

open DSP, "<$inputbase.dsp" || die;

# read up until the point that we want to insert our new stuff.
while( <DSP> )
{
	push @newdsp, $_;
	if( m/\#\s+Begin Group \"shaders\"/i )
	{
		last;
	}	
}

# Insert all of our vertex shaders and depencencies
foreach $line ( @srcfiles )
{
	$line =~ s/\/\/.*$//;
	$line =~ s/^\s*//;
	$line =~ s/\s*$//;
	next if( $line =~ m/^\s*$/ );
	if( $line =~ m/\.fxc/ || $line =~ m/\.vsh/ || $line =~ m/\.psh/ )
	{
		&DoAsmShader( $line );
	}
}


# skip everything until the end of the group
while( <DSP> )
{
	if( m/\# End Group/ )
	{
		push @newdsp, $_;
		last;
	}
}

while( <DSP> )
{
	push @newdsp, $_;
}

close DSP;

open DSP, ">$inputbase.dsp" || die "can't open $inputbase.dsp for writing\n";
print DSP @newdsp;
close DSP;

# Generate shaders.cpp

open SHADERSINC, ">$inputbase.cpp" || die "can't open $inputbase.cpp for writing\n";

# disable this warning:
#	warning C4049: compiler limit : terminating line number emission
printf SHADERSINC "// Get rid of:\n";
printf SHADERSINC "//	warning C4049: compiler limit : terminating line number emission\n";
printf SHADERSINC "\#ifdef _WIN32\n";
printf SHADERSINC "\#pragma warning (disable:4049)\n";
printf SHADERSINC "\#endif\n";
printf SHADERSINC "\#define DEFINE_SHADERS\n";

printf SHADERSINC "\#include \"tier0/dbg.h\"\n";
printf SHADERSINC "\#include \"materialsystem/ishader.h\"\n";
printf SHADERSINC "\#include \"shaderlib/shaderDLL.h\"\n";

$dx = "9";
foreach $line ( sort( @srcfiles ) )
{
#	print SHADERSINC $line . "\n";
	local( $name ) = $line;
	$name =~ s/\/\/.*$//;
	$name =~ s/^\s*//;
	$name =~ s/\s*$//;
	next if( $line =~ m/^\s*$/ );
	if( $name =~ m/\.psh/ )
	{
		$name =~ s/\.psh//i;
		printf SHADERSINC "\#include \"pshtmp" . $dx . "\\$name" . "\.inc\"\n";
	}
	elsif( $name =~ m/\.vsh/ )
	{
		$name =~ s/\.vsh//i;
		printf SHADERSINC "\#include \"vshtmp" . $dx . "\\$name" . "\.inc\"\n";
	}
	elsif( $name =~ m/\.fxc/ )
	{
		$name =~ s/\.fxc//i;
		if( $nv3x )
		{
			printf SHADERSINC "\#ifdef PS20A\n";
			printf SHADERSINC "\#include \"fxctmp" . $dx . "_nv3x\\$name" . "\.inc\"\n";
			printf SHADERSINC "\#else\n";
			printf SHADERSINC "\#include \"fxctmp" . $dx . "_nv3x_ps20\\$name" . "\.inc\"\n";
			printf SHADERSINC "\#endif\n\n";
		}
		else
		{
			printf SHADERSINC "\#include \"fxctmp" . $dx . "\\$name" . "\.inc\"\n";
		}
	}
	else
	{
		die;
	}
}

close SHADERSINC;
