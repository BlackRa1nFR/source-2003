# NOTE: These must match the same values in macros.vsh!
$cModelViewProj0	= "c4";
$cModelViewProj1	= "c5";
$cModelViewProj2	= "c6";
$cModelViewProj3	= "c7";

$cModelView0		= "c12";
$cModelView1		= "c13";
$cModelView2		= "c14";
$cModelView3		= "c15";

$cModel0			= "c42";
$cModel1			= "c43";
$cModel2			= "c44";

# NOTE: These must match the same values in macros.vsh!
$vPos				= "v0";
$vBoneWeights		= "v1";
$vBoneIndices		= "v2";
$vNormal			= "v3";
$vColor				= "v5";
$vSpecular			= "v6";
$vTexCoord0			= "v7";
$vTexCoord1			= "v8";
$vTexCoord2			= "v9";
$vTexCoord3			= "v10";
$vTangentS			= "v11";
$vTangentT			= "v12";
$vUserData			= "v14";


sub ReadInputFile
{
	local( $filename ) = shift;
	local( *INPUT );
	local( @output );
	open INPUT, "<$filename" || die;

	local( $line );
	local( $linenum ) = 1;
	while( $line = <INPUT> )
	{
		$line =~ s/\n//g;
		local( $postfix ) = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		$postfix .= "; LINEINFO($filename)($linenum)\n";
		if( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			push @output, &ReadInputFile( $1 );
		}
		else
		{
			push @output, $line . $postfix;
		}
		$linenum++;
	}

	close INPUT;
	return @output;
}

sub IsPerl
{
	local( $line ) = shift;
	if( $line =~ m/^\s*sub.*\,/ )
	{
		return 0;
	}
	if( $line =~ m/^\s*if\s*\(/		||
		$line =~ m/^\s*else/		||
		$line =~ m/^\s*elsif/		||
		$line =~ m/^\s*for\s*\(/	||
		$line =~ m/^\s*\{/			||
		$line =~ m/^sub\s*/		||
		$line =~ m/^\s*\}/			||
		$line =~ m/^\s*\&/			||
		$line =~ m/^\s*\#/			||
		$line =~ m/^\s*\$/			||
		$line =~ m/^\s*print/			||
		$line =~ m/^\s*return/			||
		$line =~ m/^\s*exit/			||
		$line =~ m/^\s*die/			||
		$line =~ m/^\s*eval/			||
		$line =~ m/^\s*local/		 ||
		$line =~ m/^\s*alloc\s+/		||
		$line =~ m/^\s*free\s+/		
		)
	{
		return 1;
	}
	return 0;
}

# NOTE: These should match g_LightCombinations in vertexshaderdx8.cpp!
@g_staticLightTypeArray = ( "none", "static", 
				  "none", "none", "none", "none", "none", 
				  "none", "none", "none", "none", "none", 
				  "static", "static", "static", "static", "static", 
				  "static", "static", "static", "static", "static" );

@g_ambientLightTypeArray = ( "none", "none", 
				  "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", 
				  "ambient", "ambient", "ambient", "ambient", 
				  "ambient", "ambient", "ambient", "ambient", "ambient", "ambient", 
				  "ambient", "ambient", "ambient", "ambient" );

@g_localLightType1Array = ( "none", "none", 
				  "none", "spot", "point", "directional", "spot", "spot", 
				  "spot", "point", "point", "directional",
				  "none", "spot", "point", "directional", "spot", "spot", 
				  "spot", "point", "point", "directional" );

@g_localLightType2Array = ( "none", "none", 
				  "none", "none", "none", "none", "spot", "point", 
				  "directional", "point", "directional", "directional",
				  "none", "none", "none", "none", "spot", "point", 
				  "directional", "point", "directional", "directional" );

# translate the output into something that takes us back to the source line
# that we care about in msdev
sub TranslateErrorMessages
{
	local( $origline );
	while( $origline = shift )
	{
		if( $origline =~ m/(.*)\((\d+)\)\s+:\s+(.*)$/i )
		{
			local( $filename ) = $1;
			local( $linenum ) = $2;
			local( $error ) = $3;
			local( *FILE );
			open FILE, "<$filename" || die;
			local( $i );
			local( $line );
			for( $i = 0; $i < $linenum; $i++ )
			{
				$line = <FILE>;
			}
			if( $line =~ m/LINEINFO\((.*)\)\((.*)\)/ )
			{
				print "$1\($2\) : $error\n";
				print "$filename\($linenum\) : original error location\n";
			}
			close FILE;
		}
		else
		{
			$origline =~ s/successful compile\!.*//gi;
			if( !( $origline =~ m/^\s*$/ ) )
			{
				#print $origline;
			}
		}
	}
}


sub CountInstructions
{
	local( $line );
	local( $count ) = 0;
	while( $line = shift )
	{
		# get rid of comments
		$line =~ s/;.*//gi;
		$line =~ s/\/\/.*//gi;
		# skip the vs1.1 statement
		$line =~ s/^\s*vs.*//gi;
		# if there's any text left, it's an instruction
		if( $line =~ /\S/gi )
		{
			$count++;
		}
	}
	return $count;
}

sub UsesRegister
{
	local( $registerName ) = shift;
	local( $line );
	while( $line = shift )
	{
#		print $line;
		# get rid of comments
		$line =~ s/;.*//gi;
		$line =~ s/\/\/.*//gi;
		# if there's any text left, it's an instruction
		if( $line =~ /\b$registerName\b/gi )
		{
			return 1;
		}
	}
	return 0;
}

sub PadString
{
	local( $str ) = shift;
	local( $desiredLen ) = shift;
	local( $len ) = length $str;
	while( $len < $desiredLen )
	{
		$str .= " ";
		$len++;
	}
	return $str;
}

sub FixupAllocateFree
{
	local( $line ) = shift;
	$line =~ s/\&AllocateRegister\s*\(\s*\\(\S+)\s*\)/&AllocateRegister( \\$1, \"\\$1\" )/g;
	$line =~ s/\&FreeRegister\s*\(\s*\\(\S+)\s*\)/&FreeRegister( \\$1, \"\\$1\" )/g;
	$line =~ s/alloc\s+(\S+)\s*/local( $1 ); &AllocateRegister( \\$1, \"\\$1\" );/g;
	$line =~ s/free\s+(\S+)\s*/&FreeRegister( \\$1, \"\\$1\" );/g;
	return $line;
}

# This is used to make the generated pl files all pretty.
sub GetLeadingWhiteSpace
{
	local( $str ) = shift;
	if( $str =~ m/^;\S/ || $str =~ m/^; \S/ )
	{
		return "";
	}
	$str =~ s/^;/ /g; # count a leading ";" as whitespace as far as this is concerned.
	$str =~ m/^(\s*)/;
	return $1;
}

$filename = shift;
if( $filename =~ m/-dx9/i )
{
	$g_dx9 = 1;
	$filename = shift;
	$vshtmp = "vshtmp9";
}
else
{
	$vshtmp = "vshtmp8";
}

if( !stat $vshtmp )
{
	mkdir $vshtmp, 0777 || die $!;
}

# suck in all files, including $include files.
@input = &ReadInputFile( $filename );

# See if it uses skinning and/or lighting
$usesSkinning = 0;
$usesLighting = 0;
$usesHeightFog = 0;
for( $i = 0; $i < scalar( @input ); $i++ )
{
	$line = $input[$i];
	# remove comments
	$line =~ s/\#.*$//gi;
	$line =~ s/;.*$//gi;
	if( $line =~ m/\$g_numBones/ )
	{
		$usesSkinning = 1;
	}
	if( $line =~ m/\$g_staticLightType/ ||
		$line =~ m/\$g_ambientLightType/ ||
		$line =~ m/\$g_localLightType1/ ||
		$line =~ m/\$g_localLightType2/ )
	{
		$usesLighting = 1;
	}
	if( $line =~ m/\$g_fogType/ )
	{
		$usesHeightFog = 1;
	}
}

# Translate the input into a perl program that'll unroll everything and
# substitute variables.
while( $inputLine = shift @input )
{
	$inputLine =~ s/\n//g;
	# leave out lines that are only whitespace.
	if( $inputLine =~ m/^\s*; LINEINFO.*$/ )
	{
		next;
	}
	local( $inputLineNoLineNum ) = $inputLine;
	$inputLineNoLineNum =~ s/; LINEINFO.*//gi;
	if( &IsPerl( $inputLineNoLineNum ) )
	{
		$inputLineNoLineNum = &FixupAllocateFree( $inputLineNoLineNum );
		push @outputProgram, $inputLineNoLineNum . "\n";
	}
	else
	{
		# make asm lines that have quotes in them not barf.
		$inputLine =~ s/\"/\\\"/g;
		push @outputProgram, &GetLeadingWhiteSpace( $inputLine ) . "push \@output, \"" . 
			$inputLine . "\\n\";\n";
	}
}

$outputProgram = join "", @outputProgram;

$filename_base = $filename;
$filename_base =~ s/\.vsh//i;

open DEBUGOUT, ">$vshtmp" . "/$filename_base.pl" || die;
print DEBUGOUT $outputProgram;
close DEBUGOUT;
#print $outputProgram;

if( $usesSkinning )
{
	$maxNumBones = 3;
}
else
{
	$maxNumBones = 0;
}

if( $usesLighting )
{
	$totalLightCombos = 22;
}
else
{
	$totalLightCombos = 1;
}

if( $usesHeightFog )
{
	$totalFogCombos = 2;
}
else
{
	$totalFogCombos = 1;
}

#push @finalheader, "// hack to force dependency checking\n";
#push @finalheader, "\#ifdef NEVER\n";
#push @finalheader, "\#include \"" . $filename_base . "\.vsh\"\n";
#push @finalheader, "\#include \"..\\..\\devtools\\bin\\vsh_prep.pl\"\n";
#push @finalheader, "\#endif\n";

$usesModelView = 0;
$usesModelViewProj = 0;
$usesModel = 0;
if( $usesSkinning )
{
	$usesModel = 1;
}

$shaderPermutation = 1;
# Run the output program for all the combinations of bones and lights.
for( $g_numBones = 0; $g_numBones <= $maxNumBones; $g_numBones++ )
{
	for( $lightCombo = 0; $lightCombo < $totalLightCombos; $lightCombo++ )
	{
		for( $fogCombo = 0; $fogCombo < $totalFogCombos; $fogCombo++, $shaderPermutation++ )
		{
			$g_staticLightType = $g_staticLightTypeArray[$lightCombo];
			$g_ambientLightType = $g_ambientLightTypeArray[$lightCombo];
			$g_localLightType1 = $g_localLightType1Array[$lightCombo];
			$g_localLightType2 = $g_localLightType2Array[$lightCombo];
			if( $fogCombo == 0 )
			{
				$g_fogType = "rangefog";
			}
			else
			{
				$g_fogType = "heightfog";
			}
			$g_usesPos				= 0;
			$g_usesBoneWeights		= 0;
			$g_usesBoneIndices		= 0;
			$g_usesNormal			= 0;
			$g_usesColor			= 0;
			$g_usesSpecular			= 0;
			$g_usesTexCoord0		= 0;
			$g_usesTexCoord1		= 0;
			$g_usesTexCoord2		= 0;
			$g_usesTexCoord3		= 0;
			$g_usesTangentS			= 0;
			$g_usesTangentT			= 0;
			$g_usesUserData			= 0;
			undef @output;
			eval $outputProgram;
			if( $g_dx9 )
			{
				# Have to make another pass through after we know which v registers are used. . yuck.
				$g_usesPos				= &UsesRegister( $vPos, @output );
				$g_usesBoneWeights		= &UsesRegister( $vBoneWeights, @output );
				$g_usesBoneIndices		= &UsesRegister( $vBoneIndices, @output );
				$g_usesNormal			= &UsesRegister( $vNormal, @output );
				$g_usesColor			= &UsesRegister( $vColor, @output );
				$g_usesSpecular			= &UsesRegister( $vSpecular, @output );
				$g_usesTexCoord0		= &UsesRegister( $vTexCoord0, @output );
				$g_usesTexCoord1		= &UsesRegister( $vTexCoord1, @output );
				$g_usesTexCoord2		= &UsesRegister( $vTexCoord2, @output );
				$g_usesTexCoord3		= &UsesRegister( $vTexCoord3, @output );
				$g_usesTangentS			= &UsesRegister( $vTangentS, @output );
				$g_usesTangentT			= &UsesRegister( $vTangentT, @output );
				$g_usesUserData			= &UsesRegister( $vUserData, @output );
				undef @output;
				eval $outputProgram;
			}
			&CheckUnfreedRegisters();
			for( $i = 0; $i < scalar( @output ); $i++ )
			{
				# remove whitespace from the beginning of each line.
				$output[$i] =~ s/^\s+//;
				# remove LINEINFO from empty lines.
				$output[$i] =~ s/^; LINEINFO.*//;
			}
	#		print @output;			
			$outfilename_base = 
				"vertexShader_" . $filename_base . "_" . 
				$g_staticLightType . "_" . 
			    $g_ambientLightType . "_" . 
			    $g_localLightType1 . "_" . 
			    $g_localLightType2 . "_" . 
				$g_numBones . "_" . $g_fogType;
			$outfilename = "$vshtmp\\" . $outfilename_base . ".tmp";
			$outhdrfilename = "$vshtmp\\" . $outfilename_base . ".h";
	#		print "$filename_base $g_numBones bones light1: $g_lightType1 light2: $g_lightType2\n";

			open OUTPUT, ">$outfilename" || die;
			print OUTPUT @output;
			close OUTPUT;

			local( $instructionCount ) = &CountInstructions( @output );
			if( &UsesRegister( $cModelView0, @output ) ||
				&UsesRegister( $cModelView1, @output ) ||
				&UsesRegister( $cModelView2, @output ) ||
				&UsesRegister( $cModelView3, @output ) )
			{
				$usesModelView = 1;
			}
			if( &UsesRegister( $cModelViewProj0, @output ) ||
				&UsesRegister( $cModelViewProj1, @output ) ||
				&UsesRegister( $cModelViewProj2, @output ) ||
				&UsesRegister( $cModelViewProj3, @output ) )
			{
				$usesModelViewProj = 1;
			}
			if( &UsesRegister( $cModel0, @output ) ||
				&UsesRegister( $cModel1, @output ) ||
				&UsesRegister( $cModel2, @output ) )
			{
				$usesModel = 1;
			}

			local( $debug );

			$debug = 1;
	#		for( $debug = 1; $debug >= 0; $debug-- )
			{
				# assemble the vertex shader
				unlink $outhdrfilename;
				if( $g_dx9 )
				{
					$vsa = "..\\..\\devtools\\bin\\vsa9";
					$vsadebug = "$vsa /nologo /Zi /Fh$outhdrfilename";
					$vsanodebug = "$vsa /nologo /Fh$outhdrfilename";
				}
				else
				{
					$vsa = "..\\..\\devtools\\bin\\vsa8_1";
					$vsadebug = "$vsa -h";
					$vsanodebug = "$vsa -h";
				}
				if( $debug )
				{
#					print "$vsadebug $outfilename 2>&1\n";
					@vsaoutput = `$vsadebug $outfilename 2>&1`;
					print @vsaoutput;
				}
				else
				{
#					print "$vsanodebug $outfilename 2>&1\n";
					@vsaoutput = `$vsanodebug $outfilename 2>&1`;
#					print @vsaoutput;
				}
				&TranslateErrorMessages( @vsaoutput );
				if( $debug )
				{
					print $filename_base . " ";
					print &PadString( $shaderPermutation, 3 ) . " ";
#					print &PadString( $lightCombo, 2 ) . " ";
					print &PadString( $g_staticLightType, 6 ) . " ";
					print &PadString( $g_ambientLightType, 7 ) . " ";
					print &PadString( $g_localLightType1, 11 ) . " ";
					print &PadString( $g_localLightType2, 11 ) . " ";
					print &PadString( $g_numBones . " bones ", 7 );
					print &PadString( $g_fogType . " ", 10 );
					print "$instructionCount instructions\n";
				}

				if( !stat $outhdrfilename )
				{
					print "didn't create $outhdrfilename\n";
					# should really just die here.
					die;
				}

				# read the header file
				open HDR, "<$outhdrfilename";
				@hdr = <HDR>;
				close HDR;

				# Remove the header file. . we don't need it anymore since we are 
				# going to generate a ".inc" file.
				unlink $outhdrfilename;
				
				local( $i );
				for( $i = 0; $i < scalar( @hdr ); $i++ )
				{
					if( $g_dx9 )
					{
						$hdr[$i] =~ s/g_.*_main/$outfilename_base/;
						$hdr[$i] =~ s/DWORD/static unsigned int/;
						$hdr[$i] =~ s/const//;
					}
					else
					{
						$hdr[$i] =~ s/dwVertexShader/$outfilename_base/;
						$hdr[$i] =~ s/DWORD/static unsigned int/;
					}
				}
			
				if( $debug )
				{
	#				push @finalheader, "#ifdef _DEBUG\n";
				}
				else
				{
	#				push @finalheader, "#ifndef _DEBUG\n";
				}
				push @finalheader, @hdr;
	#			push @finalheader, "#endif\n";
			}
		}
	}
}

print "   usesmodelview: $usesModelView ";
print "usesmodelviewproj: $usesModelViewProj ";
print "usesmodel: $usesModel\n";

# stick info about the shaders at the end of the inc file.
push @finalheader, "static PrecompiledShaderByteCode_t $filename_base" . "_vertex_shaders[] = {\n";
for( $g_numBones = 0; $g_numBones <= $maxNumBones; $g_numBones++ )
{
	for( $lightCombo = 0; $lightCombo < $totalLightCombos; $lightCombo++ )
	{
		for( $fogCombo = 0; $fogCombo < $totalFogCombos; $fogCombo++ )
		{
			$g_staticLightType = $g_staticLightTypeArray[$lightCombo];
			$g_ambientLightType = $g_ambientLightTypeArray[$lightCombo];
			$g_localLightType1 = $g_localLightType1Array[$lightCombo];
			$g_localLightType2 = $g_localLightType2Array[$lightCombo];
			if( $fogCombo == 0 )
			{
				$g_fogType = "rangefog";
			}
			else
			{
				$g_fogType = "heightfog";
			}
			$outfilename_base = 
				"\tvertexShader_" . $filename_base . "_" .
				$g_staticLightType . "_" . $g_ambientLightType . "_" . 
				$g_localLightType1 . "_" . $g_localLightType2 . "_" . 
				$g_numBones . "_" . $g_fogType;
			push @finalheader, "{ $outfilename_base, sizeof( $outfilename_base ) },\n";
		}	
	}
}
push @finalheader, "};\n";


push @finalheader, "struct $filename_base" . "_VertexShader_t : public PrecompiledShader_t\n";
push @finalheader, "{\n";
push @finalheader, "\t$filename_base" . "_VertexShader_t()\n";
push @finalheader, "\t{\n";
push @finalheader, "\t\tm_nFlags = 0;\n";
if( $usesSkinning )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_SKINNING;\n";
}
if( $usesLighting )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_LIGHTING;\n";
}
if( $usesHeightFog )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_HEIGHT_FOG;\n";
}
if( $usesModelView )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_MODEL_VIEW_MATRIX;\n";
}
if( $usesModelViewProj )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_MODEL_VIEW_PROJ_MATRIX;\n";
}
if( $usesModel )
{
	push @finalheader, "\t\tm_nFlags |= SHADER_USES_MODEL_MATRIX;\n";
}

#push @finalheader, "\t\tppVertexShaders = $filename_base" . "_vertex_shaders;\n";
push @finalheader, "\t\tm_pByteCode = $filename_base" . "_vertex_shaders;\n";
push @finalheader, "\t\tm_pName = \"$filename_base\";\n";
push @finalheader, "\t\tm_nShaderCount = " . ( $maxNumBones + 1 ) * $totalFogCombos * $totalLightCombos . ";\n";
push @finalheader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_VERTEX_SHADER, this );\n";
push @finalheader, "\t}\n";
push @finalheader, "};\n";
push @finalheader, "static $filename_base" . "_VertexShader_t $filename_base" . "_VertexShaderInstance;\n";


# Write the final header file with the compiled vertex shader programs.
$finalheadername = "$vshtmp\\" . $filename_base . ".inc";
#print "writing $finalheadername\n";
open FINALHEADER, ">$finalheadername" || die;
print FINALHEADER @finalheader;
close FINALHEADER;
