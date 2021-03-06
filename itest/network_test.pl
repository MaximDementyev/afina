#!/usr/bin/perl
use 5.016;
use warnings;
use threads;
use Test::More tests => 65;
use IO::Socket::INET;
use Getopt::Long;

# horrible gut digging to persuade `explain` to print source code
no warnings 'once';
$Data::Dumper::Deparse = 1;

my $server = $ENV{AFINA_SERVER} // "127.0.0.1";
my $port = $ENV{AFINA_PORT} // "8080";
my $silent = 0;

GetOptions(
	"address=s" => \$server,
	"port=i" => \$port,
	"silent" => \$silent,
) or die "Usage: $0 [-a server] [-p port]\n";

alarm(20);

sub afina_request_silent { # 0 tests
	my ($request) = @_;
	my $socket = IO::Socket::INET::->new(
		PeerAddr => "$server:$port",
		Proto => "tcp"
	) or die "socket: $!";
	print($socket $request) or die "print: $!";
	shutdown($socket, SHUT_WR()) or die "shutdown: $!";
	my $received;
	$received .= $_ while (<$socket>);
	$received;
}

sub afina_request { # 3 tests
	my ($request) = @_;
	my $socket = IO::Socket::INET::->new(
		PeerAddr => "$server:$port",
		Proto => "tcp"
	);
	ok($socket, "Connected to Afina");
	ok(
		ref $request ? $request->($socket)
			: print($socket $request),
		"Sent request"
	);
	$silent or note ref $request ? explain $request : $request =~ s/^/-> /mrg;
	ok(shutdown($socket, SHUT_WR()), "Closed writing end of connection");
	my $received;
	$received .= $_ while (<$socket>);
	$silent or note $received =~ s/^/<- /mrg;
	$received;
}

sub afina_test { # 4 tests
	my ($request, $response, $desc) = @_;
	ref $response ? like(afina_request($request), $response, $desc)
		: is(afina_request($request), $response, $desc);
}

afina_test("set foo 0 0 6\r\nfoobar\r\n", "STORED\r\n", "Set command");
afina_test("get foo\r\n", "VALUE foo 0 6\r\nfoobar\r\nEND\r\n", "Get the value we just set");
afina_test("get foo\r\nget foo\r\n", "VALUE foo 0 6\r\nfoobar\r\nEND\r\nVALUE foo 0 6\r\nfoobar\r\nEND\r\n", "Multiple commands");

afina_test(
	"set foo 0 0 3\r\nwtf\r\n"
	."set bar 0 0 3\r\nzzz\r\n"
	."get foo bar\r\n",
	"STORED\r\n"
	."STORED\r\n"
	."VALUE foo 0 3\r\nwtf\r\n"
	."VALUE bar 0 3\r\nzzz\r\n"
	."END\r\n",
	"Multiple commands with body"
);

afina_test(
	"add test 0 0 6\r\nfoobar\r\n",
	"STORED\r\n",
	"Add non-existent key"
);

afina_test(
	"add test 0 0 6\r\nfoobar\r\n",
	"NOT_STORED\r\n",
	"Don't add existent key"
);

afina_test(
	"append test_ 0 0 3\r\nwtf\r\n",
	"NOT_STORED\r\n",
	"Don't append non-existent key"
);

afina_test(
	"append test 0 0 3\r\nwtf\r\n",
	"STORED\r\n",
	"Append an existent key"
);

afina_test(
	"get test\r\n",
	"VALUE test 0 9\r\nfoobarwtf\r\nEND\r\n",
	"Verify the append"
);

TODO: {
	local $TODO = "Replace command isn't yet implemented in the parser";

	afina_test(
		"replace test_ 0 0 3\r\nwtf\r\n",
		"NOT_STORED\r\n",
		"Don't replace non-existent key"
	);

	afina_test(
		"replace test 0 0 3\r\nzzz\r\n",
		"STORED\r\n",
		"Replace an existent key"
	);

	afina_test(
		"get test\r\n",
		"VALUE test 0 3\r\nzzz\r\nEND\r\n",
		"Verify replace"
	);
}

TODO: {
	local $TODO = "Delete command isn't yet implemented";

	afina_test(
		"delete test\r\n",
		"DELETED\r\n",
		"Delete a key"
	);
}

afina_test(
	"blablabla 0 0 0\r\n",
	qr/ERROR/,
	"Must report unknown command error to user"
);

afina_test(
	"get var\r\r",
	qr/ERROR/,
	"Must report desync errors to user"
);

afina_test(
	sub {
		my $socket = shift;
		# make sure everything we write ends up before afina's eyes as we want it
		$socket->autoflush(1);
		print $socket "set foo 0 ";
		sleep 1;
		print $socket "0 3\r";
		sleep 1;
		print $socket "\nwtf\r\n";
	},
	"STORED\r\n",
	"Must correctly handle partial writes"
);

my %par_responses;
$par_responses{$_}++ for (map { $_->join } map { threads::->create(\&afina_request_silent, $_) } map { sprintf "set bar 0 0 3\r\n%03d\r\n", $_ } 1..100);
note "Parallel test responses:";
for (sort { $par_responses{$a} <=> $par_responses{$b} } keys %par_responses) {
	note "$par_responses{$_} ".($_=~s/([\r\n])/{"\r"=>'\r',"\n"=>'\n'}->{$1}/megr)."\n"
}
ok($par_responses{"STORED\r\n"}, "Afina replied with 'STORED' at least once");
