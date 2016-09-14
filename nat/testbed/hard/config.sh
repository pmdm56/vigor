
# the network:
# tester{icnalsp3s1}[em3] 192.168.3.5 -- 192.168.3.2 [em3]mdbox{icnalsp3s2}
# tester{icnalsp3s1}[em2] 192.168.2.10 -- 192.168.2.2 [em2]mdbox{icnalsp3s2}
# mdbox{icnalsp3s2}[em4] 192.168.200.2 -- 192.168.200.10 [em4]server{icnalsp3s3}

TESTER_HOST=icnalsp3s1.epfl.ch
MB_HOST=icnalsp3s2.epfl.ch
SERVER_HOST=icnalsp3s3.epfl.ch

MB_INTERNAL_MAC=00:1e:67:92:29:6d
MB_EXTERNAL_MAC=00:1e:67:92:29:6c
MB_TO_SRV_MAC=00:1e:67:92:29:6e

MB_IP_INTERNAL=192.168.3.2
EXTERNAL_SUBNET=192.168.2.0
SERVER_SUBNET=192.160.200.0
MB_IP_EXTERNAL=192.168.2.2
MB_IP_TO_SRV=192.168.200.2

MB_DEVICE_INTERNAL=em3
MB_DEVICE_EXTERNAL=em2

MB_PCI_INTERNAL='0000:02:00.1'
MB_PCI_EXTERNAL='0000:02:00.2'

TESTER_DEVICE_INTERNAL=em3
TESTER_DEVICE_EXTERNAL=em2

TESTER_PCI_INTERNAL=0000:02:00.2
TESTER_PCI_EXTERNAL=0000:02:00.1

TESTER_MAC_INTERNAL=00:1e:67:92:2a:bd
TESTER_MAC_EXTERNAL=00:1e:67:92:2a:bc

TESTER_IP_INTERNAL=192.168.3.5
TESTER_IP_EXTERNAL=192.168.2.10
TESTER_IP_FOR_STUB=192.168.200.5

SERVER_IP=192.168.200.10
SERVER_MAC=00:1e:67:92:2a:2b
SERVER_DEVICE=em4

export RTE_SDK=/home/necto/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc

NAT_SRC_PATH=/home/necto/vnds/nat
STUB_SRC_PATH=/home/necto/vnb-nat
