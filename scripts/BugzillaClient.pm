# ------------------------------------------------------------------------------
# Copyright (C) 2007 Thomas Schmidt <tschmidt@suse.de>
#   
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#    
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#    
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ----------------------------------------------------------------------------

package SUSE::BugzillaClient;

use strict;

use Term::ReadKey;
use LWP::UserAgent;
use HTTP::Cookies;
use XML::Parser;
use Text::ParseWords;   # For CSV parsing
#use Data::Dumper;

use base qw(Exporter);

our $VERSION = 0.4;

=head1 NAME

SUSE::BugzillaClient - A Client Library for accessing a bugzilla server. 

=head1 DESCRIPTION

SUSE::BugzillaClient is a client Library for accessing a bugzilla server. 
Note: The server has to support XML output (&ctype=xml).  
It can query bugs by ID or the URL of a search. It returns a hash for each bug 
that contains the bugs properties.

=head1 SYNOPSIS

use SUSE::BugzillaClient;
    
#login to Novell bugzilla
my $jar = bzIChainConnect( 'username','pw' );

#or login to upstream bugzilla installation
my $jar = bzConnect( 'username','pw' );

#call query methods with the $jar object

#do something meaningful with the result, see BugzillaClientTest.pl

=head1 METHODS

=over

=cut

@BugzillaClient::EXPORT = qw( bzConnect getBug getBugs getBugsByIDList getBugsCSV getBugIds $bzDebug $bzBaseUrl $bzCookieDir $bzRecover %bzErrors);

use vars qw(%gbug $gelem $bzDebug $bzBaseUrl $bzCookieDir $bzRecover %bzErrors);

$bzDebug = 0;
$bzCookieDir = "/tmp";
$bzBaseUrl = "https://bugzilla.novell.com";
my $bzIChainUrl = "https://bugzilla.novell.com/ICSLogin/auth-up";
my $useragent = "BugzillaClient/0.1";

# ----------------------------------------------------------------------------

=item B<bzIChainConnect>( $user, $pass )

Login to IChain (Novell Bugzilla) and get the cookie $jar

=cut
# ----------------------------------------------------------------------------
sub bzIChainConnect {
  my ($user, $pass) = @_;

  my %form = ( username => $user, password => $pass,
               url => $bzBaseUrl );

  my $ua = LWP::UserAgent->new( agent => $useragent);
  push @{ $ua->requests_redirectable }, 'POST';
  my $jar = HTTP::Cookies->new(file => "$bzCookieDir/lwpcookies.txt",
			         	autosave => 1, ignore_discard => 1 );
  $ua->cookie_jar( $jar );
  
  my $response = $ua->post( $bzIChainUrl, \%form );
  #print Dumper ($response);

  #if ( $response->is_redirect && (my $cookie = $response->header('Set-Cookie')) ) {
  if ( $response->is_success ) {
    return $jar;

  } else {
    #debug("Response body: " . $response->content . "\n");
    die( "Login failed, " . $response->status_line );
  }
}

# ----------------------------------------------------------------------------

=item B<bzConnect>( $user, $pass )

Login to standard bugzilla and get the cookie $jar

=cut
# ----------------------------------------------------------------------------
sub bzConnect {
  my ($user, $pass) = @_;

  my $ua = LWP::UserAgent->new ( agent => $useragent);
  my $jar = HTTP::Cookies->new(file => "$bzCookieDir/lwpcookies.txt",
			         	autosave => 1, ignore_discard => 1 );
  $ua->cookie_jar( $jar );
	
  my $req = HTTP::Request->new(POST => $bzBaseUrl . '/index.cgi' );
  $req->content_type('application/x-www-form-urlencoded');
  $req->content("Bugzilla_login=$user&Bugzilla_password=$pass");
  my $res = $ua->request($req);
	
  if ( $res->is_success && (my $cookie = $res->header('Set-Cookie')) ) {
    return $jar;

  } else {
    die( "Login failed, Status: " . $res->status_line );
  }
}

sub debug {
	print STDERR @_ if ($bzDebug);
}

# Start of XML Element, create a new gbug Hash if <bug> Element
sub handle_start {
	my ($expat, $elem) = @_;
	# initialize new Bug Hash
	if ($elem eq "bug"){
		debug("Starting new Bug Element: " . $elem . "\n");
		%gbug = ();
		foreach my $valid ( "bug_id", "creation_ts", "short_desc", "delta_ts", 
			"classification", "product", "component", "version", "rep_platform", "op_sys", 
			"bug_status", "priority", "bug_severity", "target_milestone", "resolution", 
			"infoprovider", "assigned_to", "reporter", "qa_contact", "group", "status_whiteboard","cf_partnerid") {
      		$gbug{$valid} = "";
		}
    	} else {
		debug("Starting new Element: " . $elem . "\n");
	}
  	$gelem = $elem;
}



sub handle_char {
  my ($expat, $val) = @_;
  if ( defined $gelem && defined $gbug{$gelem} ) {
    debug("Storing <$val> to element <$gelem>\n");
    $gbug{$gelem} = $gbug{$gelem} . $val;
  }

}



sub handle_end {
	my ($expat, $elem) = @_;
	my $bugs = $expat->{bugs};
        if ($elem eq "bug"){
                debug("Adding Bug " . $gbug{short_desc} . " to Array\n");
		my %temp = %gbug;
		push(@$bugs, \%temp);
	}
	$gelem = undef;
}



sub handle_xmldec {
        my ($expat, $version, $encoding, $standalone) = @_;
        debug("XML Info: \nVersion: ".$version."\nEncoding: ".$encoding."\nStandalone: ".$standalone."\n");
}


# ----------------------------------------------------------------------------

=item B<getBug>( $jar, $bugid )

Get a bug by id, returns a bug hash with the following content: 
( "bug_id", "creation_ts", "short_desc", "delta_ts", "product", "component", "version", "rep_platform", "op_sys", "bug_status", "priority", "bug_severity", "target_milestone", "resolution" )

=cut
# ----------------------------------------------------------------------------
sub getBug($$) {
  my ($jar, $bugid) = @_;

  if( $bugid =~ /^\d+$/ ) {
    my @bugids;
    push @bugids, $bugid;
    my @bugs = getBugsByIDList($jar, @bugids);
    return $bugs[0];
    
  } else {
	print "Please use a decimal BugID\n";
  }

}

# ----------------------------------------------------------------------------

=item B<getBugs>( $jar, $url )

Get a list of bug hashes by providing a search url (like https://bugzilla.novell.com/buglist.cgi?query_format=advanced&product=openSUSE+10.3&bug_status=NEEDINFO)

=cut
# ----------------------------------------------------------------------------
sub getBugs($$) {
	my ($jar, $url) = @_;
	my @bugids = getBugIds($jar, $url);
	return getBugsByIDList( $jar, @bugids );
}

# get an array of bug hashes for a list of bug ids
sub getBugsByIDList($@) {
	%bzErrors = ();
	return _getBugsByIDList(@_);
}

sub _getBugsByIDList {
	my ($jar, @buglist) = @_;
	
	if (!@buglist) {
		return ();
	}
	# construct url for the ids
	my $url = $bzBaseUrl . "/show_bug.cgi?ctype=xml&excludefield=attachmentdata&excludefield=long_desc";
	foreach (@buglist){
		$url = $url . "&id=" . $_;
	}
	
	my $bugstr = getUrl($jar, $url);	

	# parse it into $p2->{bugs}
    	my $p2 = new XML::Parser( Handlers => {
				Start => \&handle_start,
                                End   => \&handle_end,
                                Char  => \&handle_char,
				XMLDecl => \&handle_xmldec }, 
                              ProtocolEncoding => 'UTF-8', 
			      ErrorContext => 1, 
			       );
	$p2->{bugs} = [];

	#workaround for illegal characters: 
	$bugstr =~ s/[\x00-\x08\x0B\x0C\x0E-\x1F]/ /g;
    
	eval {
		$p2->parse( $bugstr );
		if (!@{$p2->{bugs}}) {
			die "No bug data returned (too many bugs?)";
		}
	};
	if ($@) {
		if (!$bzRecover) {
			die $@;
		}
		# bugzilla choked on one of the bugs or there is a permanent
		# error
		if (@buglist < 2) {
			$bzErrors{$buglist[0] || 0} = $@;
			return ();
		}
		my $last = $#buglist;
		debug("Trying to recurse\n");
		return (_getBugsByIDList($jar, @buglist[0..$last/2]),
			_getBugsByIDList($jar, @buglist[$last/2+1..$last]));
	}
	return @{$p2->{bugs}};
}



# Get an Array of Bugs that are shown on the provided Query-Page. 
# WARNING: This Method relies on the HTML Output Format 
# of Novell Bugzillas Query-Result-Page !!

sub getBugIds($$) {
    my ($jar, $url) = @_;
    my $bugstr = getUrl($jar, $url); 
	my @bugids;

	if( $bugstr =~ m/<a href="show_bug.cgi\?id=\d+">/ )
	{
	    while( $bugstr =~ m/<a href="show_bug.cgi\?id=(\d+)">/g )
	    {
		push @bugids, $1;
	    }

	    debug("found " . @bugids . " BugIds for your Query.\n");
	    
	} elsif ($bugstr =~ /Your search did not return any results/s) {
                debug("Your search did not return any results.\n");

        } else {
                print "Cannot fetch Bugids from Bugzilla Search Page.\n";
                print "Maybe the HTML-Code has changed :-(.\n";
        }
        return @bugids;
}



# ----------------------------------------------------------------------------

=item B<getBugsCSV>( $jar, $url )

Get a list of bug hashes by providing a CSV search url (like https://bugzilla.novell.com/buglist.cgi?query_format=advanced&product=openSUSE+10.3&bug_status=NEEDINFO&ctype=csv, get this URL from the "CSV" link on the bottom of the search result page)

Faster than using getBugs() because not all XML output has to be parsed, but only these fields will be 
available in the bug hashes: 
bug_id, changeddate, bug_severity, priority, assigned_to, reporter, bug_status, product, short_short_desc (!)

=cut
# ----------------------------------------------------------------------------
sub getBugsCSV($$) {
    my ($jar, $url) = @_;
    my $content = getUrl($jar, $url);
    my @keys;
    my @bugs;

    foreach my $line (split('\n', $content)) {
        chomp $line;
        my @field = parse_line(',', 0, $line);

        if (!scalar(@keys)) {
            @keys = @field;
            debug("The following keys are defined:\n");
            foreach my $key (@keys) {
                debug(" " . $key);
            }
            debug("\n");
            next;
        }
        my %hash = ();
        for (my $count=0;$count<@keys;$count++) {
            $hash{$keys[$count]} = $field[$count];
        }

        debug("Bugid = " . $hash{'bug_id'} . "<br>\n");
        push (@bugs, \%hash);
    }
    return @bugs;
}

# helper for getting url
sub getUrl($$) {
    my ($jar, $url) = @_;
    debug("Querying URL: $url\n");
    my $ua = LWP::UserAgent->new( agent => $useragent);
    $ua->cookie_jar( $jar );
    my $response = $ua->get( $url );
    return $response->content;
}


1;
