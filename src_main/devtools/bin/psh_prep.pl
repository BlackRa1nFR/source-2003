sub BuildDefineOptions
{
	local( $output );
	local( $combo ) = shift;
	local( $i );
	for( $i = 0; $i < scalar( @defineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $defineMax[$i] - $defineMin[$i] + 1 ) ) + $defineMin[$i];
		$output .= "/D$defineNames[$i]=$val ";
		$combo = $combo / ( $defineMax[$i] - $defineMin[$i] + 1 );
	}
	return $output;
}

sub CalcNumCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @defineNames ); $i++ )
	{
		$numCombos *= $defineMax[$i] - $defineMin[$i] + 1;
	}
	return $numCombos;
}

print "--------\n";

$psh_filename = shift;
if( $psh_filename =~ m/-dx9/i )
{
	$g_dx9 = 1;
	$psh_filename = shift;
}

local( @defineNames );
local( @defineMin );
local( @defineMax );

open PSH, "<$psh_filename";
while( <PSH> )
{
	last if( !m,^;, );
	s,^;\s*,,;
	if( m/\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
		print "\"$name\" \"$min..$max\"\n";
		push @defineNames, $name;
		push @defineMin, $min;
		push @defineMax, $max;
	}
}
close PSH;

$numCombos = &CalcNumCombos();
print "$numCombos combos\n";

if( $g_dx9 )
{
	$pshtmp = "pshtmp9";
}
else
{
	$pshtmp = "pshtmp8";
}
$basename = $psh_filename;
$basename =~ s/\.psh$//i;

for( $shaderCombo = 0; $shaderCombo < $numCombos; $shaderCombo++ )
{
	if( $g_dx9 )
	{
		$pshtmp = "pshtmp9";
		$cmd = "..\\..\\devtools\\bin\\psa9 /Zi /Fh /nologo " . &BuildDefineOptions( $shaderCombo ) . "$psh_filename";
	}
	else
	{
		$pshtmp = "pshtmp8";
		$cmd = "..\\..\\devtools\\bin\\psa8_1 -h -p " . &BuildDefineOptions( $shaderCombo ) . "$psh_filename";
	}
	if( !stat $pshtmp )
	{
		mkdir $pshtmp, 0777 || die $!;
	}
	print $cmd . "\n";
	system $cmd || die $!;

	$hdrname = $psh_filename;

	$hdrname =~ s/\.psh$/\.h/i;
	#print $hdrname . "\n";

	if( !open FILE, "<$hdrname" )
	{
		die "can't open $hdrname: $!\n";
	}
	@hdr = <FILE>;
	close FILE;
	unlink $hdrname;

	for( $i = 0; $i < scalar( @hdr ); $i++ )
	{
		if( $g_dx9 )
		{
			$hdr[$i] =~ s/const DWORD/static unsigned int/i;
			$replace = "pixelShader_" . $basename . "_" . $shaderCombo;
			$hdr[$i] =~ s/g_\S+_main/$replace/i;
		}
		else
		{
			$replace = "pixelShader_" . $basename . "_" . $shaderCombo;
			$hdr[$i] =~ s/DWORD dwPixelShader/static unsigned int $replace/i;
		}
	}
	push @outputHeader, @hdr;
}

$basename =~ s/\.fxc//gi;
push @outputHeader, "static PrecompiledShaderByteCode_t " . $basename . "_pixel_shaders[" . $numCombos . "] = \n";
push @outputHeader, "{\n";
local( $j );
for( $j = 0; $j < $numCombos; $j++ )
{
	local( $thing ) = "pixelShader_" . $basename . "_" . $j;
	push @outputHeader, "\t{ " . "$thing, sizeof( $thing ) },\n";
}
push @outputHeader, "};\n";

push @outputHeader, "struct $basename" . "PixelShader_t : public PrecompiledShader_t\n";
push @outputHeader, "{\n";
push @outputHeader, "\t$basename" . "PixelShader_t()\n";
push @outputHeader, "\t{\n";
push @outputHeader, "\t\tm_nFlags = SHADER_CUSTOM_ENUMERATION;\n";
push @outputHeader, "\t\tm_pByteCode = " . $basename . "_pixel_shaders;\n";
push @outputHeader, "\t\tm_nShaderCount = $numCombos;\n";
push @outputHeader, "\t\tm_pName = \"$basename\";\n";
if( $basename =~ /vs\d\d/ ) # hack
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_VERTEX_SHADER, this );\n";
}
else
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_PIXEL_SHADER, this );\n";
}
push @outputHeader, "\t}\n";
push @outputHeader, "};\n";

push @outputHeader, "static $basename" . "PixelShader_t $basename" . "_PixelShaderInstance;\n";


$incname = "$pshtmp/$basename.inc";
print "writing to $incname\n";
open OUTPUT, ">$incname" || die $!;
print OUTPUT @outputHeader;
close OUTPUT;
