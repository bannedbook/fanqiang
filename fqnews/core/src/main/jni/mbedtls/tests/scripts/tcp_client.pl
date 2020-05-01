#!/usr/bin/env perl

# A simple TCP client that sends some data and expects a response.
# Usage: tcp_client.pl HOSTNAME PORT DATA1 RESPONSE1
#   DATA: hex-encoded data to send to the server
#   RESPONSE: regexp that must match the server's response

use warnings;
use strict;
use IO::Socket::INET;

# Pack hex digits into a binary string, ignoring whitespace.
sub parse_hex {
    my ($hex) = @_;
    $hex =~ s/\s+//g;
    return pack('H*', $hex);
}

## Open a TCP connection to the specified host and port.
sub open_connection {
    my ($host, $port) = @_;
    my $socket = IO::Socket::INET->new(PeerAddr => $host,
                                       PeerPort => $port,
                                       Proto => 'tcp',
                                       Timeout => 1);
    die "Cannot connect to $host:$port: $!" unless $socket;
    return $socket;
}

## Close the TCP connection.
sub close_connection {
    my ($connection) = @_;
    $connection->shutdown(2);
    # Ignore shutdown failures (at least for now)
    return 1;
}

## Write the given data, expressed as hexadecimal
sub write_data {
    my ($connection, $hexdata) = @_;
    my $data = parse_hex($hexdata);
    my $total_sent = 0;
    while ($total_sent < length($data)) {
        my $sent = $connection->send($data, 0);
        if (!defined $sent) {
            die "Unable to send data: $!";
        }
        $total_sent += $sent;
    }
    return 1;
}

## Read a response and check it against an expected prefix
sub read_response {
    my ($connection, $expected_hex) = @_;
    my $expected_data = parse_hex($expected_hex);
    my $start_offset = 0;
    while ($start_offset < length($expected_data)) {
        my $actual_data;
        my $ok = $connection->recv($actual_data, length($expected_data));
        if (!defined $ok) {
            die "Unable to receive data: $!";
        }
        if (($actual_data ^ substr($expected_data, $start_offset)) =~ /[^\000]/) {
            printf STDERR ("Received \\x%02x instead of \\x%02x at offset %d\n",
                           ord(substr($actual_data, $-[0], 1)),
                           ord(substr($expected_data, $start_offset + $-[0], 1)),
                           $start_offset + $-[0]);
            return 0;
        }
        $start_offset += length($actual_data);
    }
    return 1;
}

if (@ARGV != 4) {
    print STDERR "Usage: $0 HOSTNAME PORT DATA1 RESPONSE1\n";
    exit(3);
}
my ($host, $port, $data1, $response1) = @ARGV;
my $connection = open_connection($host, $port);
write_data($connection, $data1);
if (!read_response($connection, $response1)) {
    exit(1);
}
close_connection($connection);
