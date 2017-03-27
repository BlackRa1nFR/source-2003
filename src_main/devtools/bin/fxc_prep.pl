while( 1 )
{
	$fxc_filename = shift;
	if( $fxc_filename =~ m/-dx9/i )
	{
		$g_dx9 = 1;
	}
	elsif( $fxc_filename =~ m/-nv3x/i )
	{
		$nvidia = 1;
	}
	elsif( $fxc_filename =~ m/-ps20a/i )
	{
		$ps2a = 1;
	}
	else
	{
		last;
	}
}


$debug = 0;
$forcehalf = 0;

sub WriteHelperVar
{
	local( $name ) = shift;
	local( $min ) = shift;
	local( $max ) = shift;
	local( $varname ) = "m_n" . $name;
	local( $boolname ) = "m_b" . $name;
	push @outputHeader, "private:\n";
	push @outputHeader, "\tint $varname;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\tbool $boolname;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "public:\n";
	# int version of set function
	push @outputHeader, "\tvoid Set" . $name . "( int i )\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
	push @outputHeader, "\t\t$varname = i;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\t\t$boolname = true;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "\t}\n";
	# bool version of set function
	push @outputHeader, "\tvoid Set" . $name . "( bool i )\n";
	push @outputHeader, "\t{\n";
#		push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
	push @outputHeader, "\t\t$varname = i ? 1 : 0;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\t\t$boolname = true;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "\t}\n";
}

sub WriteStaticBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteDynamicBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteHelperClasses
{
	local( $basename ) = $fxc_filename;
	$basename =~ s/\.fxc//i;
	$basename =~ tr/A-Z/a-z/;
	local( $classname ) = $basename . "_Index";
	push @outputHeader, "class $classname\n";
	push @outputHeader, "{\n";
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$name = $staticDefineNames[$i];
		$min = $staticDefineMin[$i];
		$max = $staticDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$name = $dynamicDefineNames[$i];
		$min = $dynamicDefineMin[$i];
		$max = $dynamicDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	push @outputHeader, "public:\n";
	push @outputHeader, "\t$classname()\n";
	push @outputHeader, "\t{\n";
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = false;\n";
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = false;\n";
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	push @outputHeader, "\t}\n";
	push @outputHeader, "\tint GetIndex()\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\t// Asserts to make sure that we aren't using any skipped combinations.\n";
	foreach $skip (@perlskipcodeindividual)
	{
		$skip =~ s/\$/m_n/g;
#		push @outputHeader, "\t\tAssert( !( $skip ) );\n";
	}
	push @outputHeader, "\t\t// Asserts to make sure that we are setting all of the combination vars.\n";

	push @outputHeader, "#ifdef _DEBUG\n";
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllStaticVarsDefined = ";
		WriteStaticBoolExpression( "", "&&" );
		push @outputHeader, "\t\tbool bNoStaticVarsDefined = ";
		WriteStaticBoolExpression( "!", "&&" );
	}
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllDynamicVarsDefined = ";
		WriteDynamicBoolExpression( "", "&&" );
		push @outputHeader, "\t\tbool bNoDynamicVarsDefined = ";
		WriteDynamicBoolExpression( "!", "&&" );
	}
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllStaticVarsDefined != bNoStaticVarsDefined );\n";
	}
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllDynamicVarsDefined != bNoDynamicVarsDefined );\n";
	}
	if( scalar( @dynamicDefineNames ) > 0 && scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllDynamicVarsDefined != bAllStaticVarsDefined );\n";
		push @outputHeader, "\t\tAssert( bNoDynamicVarsDefined != bNoStaticVarsDefined );\n";
	}
	push @outputHeader, "#endif // _DEBUG\n";

	push @outputHeader, "\t\treturn ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	push @outputHeader, "0;\n";
	push @outputHeader, "\t}\n";
	push @outputHeader, "};\n";
}

sub BuildDefineOptions
{
	local( $output );
	local( $combo ) = shift;
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) ) + $dynamicDefineMin[$i];
		$output .= "/D$dynamicDefineNames[$i]=$val ";
		$combo = $combo / ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 );
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) ) + $staticDefineMin[$i];
		$output .= "/D$staticDefineNames[$i]=$val ";
		$combo = $combo / ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 );
	}
	return $output;
}

sub SetPerlVarsForCombo
{
	local( $combo ) = shift;
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) ) + $dynamicDefineMin[$i];
		local( $eval ) = "\$" . $dynamicDefineNames[$i] . " = $val;";
		eval $eval;
		$combo = $combo / ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 );
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) ) + $staticDefineMin[$i];
		local( $eval ) = "\$" . $staticDefineNames[$i] . " = $val;";
		eval $eval;
		$combo = $combo / ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 );
	}
}

sub GetNewMainName
{
	local( $shadername ) = shift;
	local( $combo ) = shift;
	local( $i );
	$shadername =~ s/\./_/g;
	local( $name ) = $shadername;
	for( $i = 0; $i < scalar( @defineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $defineMax[$i] - $defineMin[$i] + 1 ) ) + $defineMin[$i];
		$name .= "_" . $defineNames[$i] . "_" . $val;
		$combo = $combo / ( $defineMax[$i] - $defineMin[$i] + 1 );
	}
#	return $name;
	return "main";
}

sub RenameMain
{
	local( $shadername ) = shift;
	local( $combo ) = shift;
	local( $name ) = &GetNewMainName( $shadername, $combo );
	return "/Dmain=$name /E$name ";
}

sub GetShaderType
{
	local( $shadername ) = shift;
	if( $shadername =~ m/ps20/i )
	{
		if( $debug )
		{
			return "ps_2_sw";
		}
		else
		{
			if( $ps2a )
			{
				return "ps_2_a";
			}
			else
			{
				return "ps_2_0";
			}
		}
	}
	elsif( $shadername =~ m/ps14/i )
	{
		return "ps_1_4";
	}
	elsif( $shadername =~ m/vs20/i )
	{
		if( $debug )
		{
			return "vs_2_sw";
		}
		else
		{
			return "vs_2_0";
		}
	}
	elsif( $shadername =~ m/vs14/i )
	{
		return "vs_1_1";
	}
	elsif( $shadername =~ m/vs11/i )
	{
		return "vs_1_1";
	}
	else
	{
		die;
	}
}

sub CalcNumCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$numCombos *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	return $numCombos;
}

sub CalcNumDynamicCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	return $numCombos;
}

print "--------\n";

if( $g_dx9 )
{
	if( $nvidia )
	{
		if( $ps2a )
		{
			$fxctmp = "fxctmp9_nv3x";
		}
		else
		{
			$fxctmp = "fxctmp9_nv3x_ps20";
		}
	}
	else
	{
		$fxctmp = "fxctmp9";
	}
}
else
{
	$fxctmp = "fxctmp8";
}

if( !stat $fxctmp )
{
	mkdir $fxctmp, 0777 || die $!;
}

open FXC, "<$fxc_filename";
# READ THE TOP OF THE FILE TO FIND SHADER COMBOS
while( <FXC> )
{
	next if( m/^\s*$/ );
	last if( !m,^//, );
	s,^//\s*,,;
	if( m/\s*STATIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
		print "\"$name\" \"$min..$max\"\n";
		push @staticDefineNames, $name;
		push @staticDefineMin, $min;
		push @staticDefineMax, $max;
	}
	elsif( m/\s*DYNAMIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
		print "\"$name\" \"$min..$max\"\n";
		push @dynamicDefineNames, $name;
		push @dynamicDefineMin, $min;
		push @dynamicDefineMax, $max;
	}
}
seek FXC, 0, 0;
# READ THE WHOLE FILE AND FIND SKIP STATEMENTS
while( <FXC> )
{
	if( m/^\s*\/\/\s*SKIP\s*\:\s*(.*)$/ )
	{
		$perlskipcode .= "(" . $1 . ")||";
		push @perlskipcodeindividual, $1;
	}	
}
if( defined $perlskipcode )
{
	$perlskipcode .= "0";
	$perlskipcode =~ s/\n//g;
}
seek FXC, 0, 0;
# READ THE WHOLE FILE AND FIND CENTROID STATEMENTS
while( <FXC> )
{
	if( m/^\s*\/\/\s*CENTROID\s*\:\s*TEXCOORD(\d+)\s*$/ )
	{
		$centroidEnable{$1} = 1;
#		print "CENTROID: $1\n";
	}
}
close FXC;

# Go ahead an compute the mask of samplers that need to be centroid sampled
$centroidMask = 0;
foreach $centroidRegNum ( keys( %centroidEnable ) )
{
#	print "THING: $samplerName $centroidRegNum\n";
	$centroidMask += 1 << $centroidRegNum;
}

#printf "0x%x\n", $centroidMask;

#print "\$perlskipcode: $perlskipcode\n";

$numCombos = &CalcNumCombos();
print "$numCombos combos\n";

# Write out the C++ helper class for picking shader combos
&WriteHelperClasses();

push @outputHeader, "#ifdef DEFINE_SHADERS\n";

for( $i = 0; $i < $numCombos; $i++ )
{
	&SetPerlVarsForCombo( $i );
	local( $compileFailed );
	if( !defined $perlskipcode )
	{
		$ret = 0;
	}
	else
	{
		$ret = eval $perlskipcode;
	}
#	$ret = 0;
#	print "\$ret = $ret\n";
	if( !defined $ret )
	{
		die "$@\n";
	}
	if( $ret )
	{
		# skip this combo!
#		print "$i/$numCombos: SKIP\n";
		$compileFailed = 1;
		$numSkipped++;
	}
	else
	{
		local( $cmd );
		if( $fxc_filename =~ /_vs/i )
		{
			#$cmd = "..\\..\\devtools\\bin\\oldfxc.exe ";
			$cmd = "..\\..\\devtools\\bin\\fxc.exe ";
		}
		else
		{
			$cmd = "..\\..\\devtools\\bin\\fxc.exe ";
		}
		$cmd .= &BuildDefineOptions( $i );
		$cmd .= &RenameMain( $fxc_filename, $i );
		$cmd .= "/T" . &GetShaderType( $fxc_filename ) . " ";
		if( $debug )
		{
			$cmd .= "/Od "; # disable optimizations
			$cmd .= "/Zi "; # enable debug info
		}
		$cmd .= "/nologo ";
	#	$cmd .= "/Gp ";  # predication
		if( $fxc_filename =~ /ps20/i && $forcehalf )
		{
			$cmd .= "/Gpp "; # use half everywhere
		}
		$cmd .= "/Fhtmpshader.h ";
		$cmd .= "$fxc_filename";
		print $i . "/" . $numCombos . " " . $cmd . "\n";

		# GROSS!!  Have to suppress stderr output by redirecting to a file. . has to be a better way.
	#	@out = `$cmd 1<&2`;
		system "$cmd > shit.txt 2>&1";
		@out = `type shit.txt`;
		print @out;
		
		local( *FILE );
		if( !open FILE, "<tmpshader.h" )
		{
			$compileFailed = 1;
			die "compile failed\n";
		}
		else
		{
			$compileFailed = 0;

			@header = <FILE>;
			close FILE;
			unlink "tmpshader.h";
		}
	}

#	print "compileFailed: $compileFailed\n";

	if( !$compileFailed )
	{
#		print @header;

		local( $name ) = $fxc_filename;
		$name =~ s/\.fxc//gi;
		local( $line );
		foreach $line( @header )
		{
			if( $line =~ m/g_.*\[\]/ )
			{
				$line = "static unsigned int pixelShader_" . $name . "_" . $i . "[] =\n";
				$line =~ s/\./_/g;
			}
			push @outputHeader, $line;
		}
	}
	else
	{
		local( $name ) = $fxc_filename;
		$name =~ s/\.fxc//gi;

		push @outputHeader, "\n// Invalid material\n";
		push @outputHeader, "static unsigned int pixelShader_" . $name . "_" . $i . "[] =\n";
		push @outputHeader, "{\n";
		push @outputHeader, "\t0x00000000\n";
		push @outputHeader, "};\n\n";
	}
}

print "$numSkipped/$numCombos skipped\n";

local( $name ) = $fxc_filename;
$name =~ s/\.fxc//gi;
push @outputHeader, "static PrecompiledShaderByteCode_t " . $name . "_pixel_shaders[" . $numCombos . "] = \n";
push @outputHeader, "{\n";
local( $j );
for( $j = 0; $j < $numCombos; $j++ )
{
	local( $thing ) = "pixelShader_" . $name . "_" . $j;
	push @outputHeader, "\t{ " . "$thing, sizeof( $thing ) },\n";
}
push @outputHeader, "};\n";

push @outputHeader, "struct $name" . "PixelShader_t : public PrecompiledShader_t\n";
push @outputHeader, "{\n";
push @outputHeader, "\t$name" . "PixelShader_t()\n";
push @outputHeader, "\t{\n";
push @outputHeader, "\t\tm_nFlags = SHADER_CUSTOM_ENUMERATION;\n";
push @outputHeader, sprintf "\t\tm_nCentroidMask = 0x%x;\n", $centroidMask;
push @outputHeader, "\t\tm_pByteCode = " . $name . "_pixel_shaders;\n";
push @outputHeader, "\t\tm_nShaderCount = $numCombos;\n";
push @outputHeader, "\t\tm_pName = \"$name\";\n";
push @outputHeader, "\t\tm_nDynamicCombos = " . &CalcNumDynamicCombos . ";\n";
if( $name =~ /vs\d\d/ ) # hack
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_VERTEX_SHADER, this );\n";
}
else
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_PIXEL_SHADER, this );\n";
}
push @outputHeader, "\t}\n";
push @outputHeader, "};\n";

push @outputHeader, "static $name" . "PixelShader_t $name" . "_PixelShaderInstance;\n";

push @outputHeader, "#endif // DEFINE_SHADERS\n";

printf "writing $fxctmp/$name" . ".inc\n";

local( *FILE );
if( !open FILE, ">$fxctmp/$name" . ".inc" )
{
	die;
}
print FILE @outputHeader;
close FILE;
undef @outputHeader;

