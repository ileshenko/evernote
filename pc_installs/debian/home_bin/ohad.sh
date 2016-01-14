#!/bin/bash
# This script is meant to be used by EZsim in TAP mode.
# It receives the following arguments:
#   *  -user|-u <user> :              owner of the TAPs
#   *  -interface|-i <interface>  :   NPS interface in the format <side#>_eng<eng#>_<Type>_<if#>  (e.g. 0_eng0_10GE_00)
#
# The script will create a TAP named <interface> and configure it with the hard coded IP 192.168.100.1.
# Then a static ARP entry is inserted with the mapping 192.168.100.2--> 00:c0:00:12:34:1
# Finally, the script will echo the needed information for the simulator.

# hardcoded information
mask="255.255.255.0"

# Parse arguments
user="none"
interface="none"

while [[ $# > 1 ]]
  do 
  key="$1"
  case $key in
    -user|-u)
      user="$2"
      shift
      ;;
    -interface|-i)
#       interface="$2"
       interface=($USER"1")
       shift
       ;;
    *)

      ;;
  esac
  shift
done

ip="192.168.11.2"
arp_ip="192.168.11.1"
arp_mac="b2:aa:bb:cc:dd:ee"

# Create TAP named <interface> (supress output using "> /dev/null"). Output MUST be suppressed since output is used to communicate with EZsim.
sudo tunctl -p -t ${interface} -u ${user} > /dev/null

# Configure TAP with an the hardcoded IP address 192.168.100.1, type C subnet (mask = 255.255.255.0)
sudo ifconfig ${interface} ${ip} netmask ${mask} up mtu 3000

# Insert a static ARP entry to ARP table in which IP address 192.168.100.2 (belong to subnet) is reached from hardcoded MAC 00:c0:00:12:34:01
#sudo arp -s ${arp_ip} ${arp_mac}

# echo the needed information for the simulator
# Format:   -interface <interface> -net_if_keep_persist false -net_if_name <interface> ip=<IPaddress in TAP subnet>:<IPaddress of TAP>::<mask>:ezsim:eth0:off mac=<mac>
echo -interface ${interface} -net_if_keep_persist false -net_if_name ${interface} -linux_if_cmd_line \"ip=${arp_ip}:${ip}::${mask}:ezsim:eth0:off mac=${arp_mac} \"
#-interface ${interface} 	-net_if_keep_persist false -net_if_name tap2 -linux_if_cmd_line "ip=192.168.2.2:192.168.2.1::255.255.255.0:ezsim:eth0:off mac=00:c0:00:a1:8b:e7 "

