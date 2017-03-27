sub BackSlashToForwardSlash
{
	local( $in ) = shift;
	local( $out ) = $in;
	$out =~ s,\\,/,g;
	return $out;
}

sub RemoveFileName
{
	local( $in ) = shift;
	$in = &BackSlashToForwardSlash( $in );
	$in =~ s,/[^/]*$,,;
	return $in;
}

sub RemovePath
{
	local( $in ) = shift;
	$in = &BackSlashToForwardSlash( $in );
	$in =~ s,^(.*)/([^/]*)$,$2,;
	return $in;
}


$infile = shift;
$outfile = shift;

open INFILE, "<$infile" || die;
@infile = <INFILE>;
close INFILE;

$basename = &RemovePath( $infile );

$outfile = &BackSlashToForwardSlash( $outfile );
if( !( $outfile =~ m/\/$/ ) )
{
	$outfile .= "/";
}

$outfile = $outfile . $basename;
print "$infile -> $outfile\n";

$inRemoveBlock = 0;
$inKeepBlock = 0;

open OUTFILE, ">$outfile" || die;
while( $line = shift @infile )
{
	if( $line =~ m/\#ifndef\s+IHVTEST/ )
	{
		$inRemoveBlock = 1;
	}
	elsif( $line =~ m/\#ifdef\s+IHVTEST/ )
	{
		$inKeepBlock = 1;
	}
	elsif( $line =~ m/\#endif.*IHVTEST/ )
	{
		$inRemoveBlock = 0;
		$inKeepBlock = 0;
	}
	elsif( !$inRemoveBlock )
	{
		print OUTFILE $line;	
	}
}
close OUTFILE;

