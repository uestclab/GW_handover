#/bin/sh

echo 1 > /proc/sys/net/ipv4/ip_forward
ip tun add iptun mode ipip remote 192.168.10.11 local 192.168.10.99

ip addr add 192.168.20.99 dev iptun
ip link set tunl0 up
ip link set iptun up

ifconfig enp3s0 192.168.10.99 # inner

ifconfig enp5s0 192.168.2.139 # extern 

ip route add table 11 default via 192.168.2.1 dev enp3s0
ip route add table 10 default dev iptun

ip rule add fwmark 10 table 10
ip rule add fwmark 11 table 11

iptables -A PREROUTING -t mangle -i enp3s0 -j MARK --set-mark 10
iptables -A PREROUTING -t mangle -i enp5s0 -j MARK --set-mark 11

iptables -t nat -A POSTROUTING -o enp3s0 -j MASQUERADE


