#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

function my_init_configure {
	ip tunnel add ethn mode ipip local 10.0.1.3 remote 10.0.1.2
	ifconfig ethn 10.1.1.100
	route add -net 10.1.1.0 netmask 255.255.255.0 ethn
	route add -host 10.1.1.101 dev ethn
	ifconfig ens33 hw ether 00:0c:29:a6:93:7f
}

echo "init IP tunnels configuration ..."
my_init_configure
echo "*** PASS ***"
