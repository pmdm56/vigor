#!/bin/bash


#https://gist.github.com/vncsna/64825d5609c146e80de8b1fd623011ca
set -euo pipefail

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

function cleanup {
  sudo killall nf 2>/dev/null || true
  sudo killall iperf 2>/dev/null || true
  sudo ip netns delete lan 2>/dev/null || true
  sudo ip netns delete wan 2>/dev/null || true
  sudo killall tcpdump 2>/dev/null || true 
}
trap cleanup EXIT


function test_nop {
  MAC_1=$1
  MAC_2=$2
  RATE=$3
  BURST=$4
  
  sudo ./build/app/nf \
        --vdev "net_tap0,iface=test_lan" \
        --vdev "net_tap1,iface=test_wan" \
        --no-huge \
        --no-shconf -- \
        --wan 1 \
        --expire 100000000 \
        --max-flows 65536 \
        --eth-dest 0,$MAC_1 \
        --eth-dest 1,$MAC_2 \
        --lan 0 \
        --rate $RATE \
        --burst $BURST \
        --capacity 65536 \
        >vigfw.out 2>&1 &
  NF_PID=$!

  while [ ! -f /sys/class/net/test_lan/tun_flags -o \
          ! -f /sys/class/net/test_wan/tun_flags ]; do
    echo "Waiting for NF to launch...";
    sleep 1;
  done
  sleep 5 

  echo "Setting up namespaces..."

  #create namespaces
  sudo ip netns add lan
  sudo ip netns add wan
  
  # Assign the interfaces to the namespaces
  sudo ip link set test_lan netns lan
  sudo ip link set test_wan netns wan

  # Assign an address to each interface
  sudo ip netns exec lan ip addr add 10.0.0.1/24 dev test_lan
  sudo ip netns exec wan ip addr add 10.0.0.2/24 dev test_wan

  # Change mac address
  sudo ip netns exec lan ifconfig test_lan hw ether $MAC_1 
  sudo ip netns exec wan ifconfig test_wan hw ether $MAC_2
  

  # Bring up interfaces
  sudo ip netns exec lan ip link set test_lan up
  sudo ip netns exec wan ip link set test_wan up
  
  # Update ARP Table
  sudo ip netns exec lan arp -i test_lan -s 10.0.0.2 $MAC_2
  sudo ip netns exec wan arp -i test_wan -s 10.0.0.1 $MAC_1

  # Configure routes
  sudo ip netns exec lan ip route add default via 10.0.0.1 dev test_lan
  sudo ip netns exec wan ip route add default via 10.0.0.2 dev test_wan

  #sudo ip netns exec lan tcpdump -i test_lan -w ~/vigfw_test_lan.pcap & TCP_DUMP_PID_LAN=$!
  #sudo ip netns exec wan tcpdump -i test_wan -w ~/vigfw_test_wan.pcap & TCP_DUMP_PID_WAN=$!

  #sudo ip netns exec lan ping 10.0.0.2 -c 5

  #test known flow between client and external server
  sudo ip netns exec wan iperf -us -i 1 &
  SERVER_PID=$!
  sudo ip netns exec lan iperf -uc 10.0.0.2 -t 1 >/dev/null

  sudo killall iperf
  wait $SERVER_PID 2>/dev/null || true

  #test known flow between external client and  server (check if fw drops all packets)
  #sudo ip netns exec lan iperf -us -i 1 &
  #SERVER_PID=$!
  #sudo ip netns exec wan iperf -uc 10.0.0.1 -t 1 >/dev/null

  #sudo killall iperf
  #wait $SERVER_PID 2>/dev/null || true

  #sudo killall nf
  #wait $NF_PID 2>/dev/null || true

  sudo ip netns delete lan
  sudo ip netns delete wan
}

make clean
make ADDITIONAL_FLAGS="-DSTOP_ON_RX_0 -g"

#test_nop <mac_dev_0> <mac_dev_1> (unicast addresses)
MAC_DEV_1=$(printf '02:%02X:%02X:%02X:%02X:%02X\n' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)))
MAC_DEV_2=$(printf '04:%02X:%02X:%02X:%02X:%02X\n' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)))
cleanup
test_nop $MAC_DEV_1 $MAC_DEV_2 12500 500000

#mv ~/vigfw_test_lan.pcap ../../vigfw_test_lan.pcap
#mv ~/vigfw_test_wan.pcap ../../vigfw_test_wan.pcap

echo "Done."
