# Config for 10G:
# [mb]{p802p2 (90:e2:ba:55:14:11)/dev6} -- {p802p2 (90:e2:ba:55:12:25)/dev8}[tester]
# [mb]{p802p1 (90:e2:ba:55:14:10)/dev5} -- {p802p1 (90:e2:ba:55:12:24)/dev7}[tester]

# direct link tester:
# p802p2 90:e2:ba:55:12:25 -- p801p1 90:e2:ba:55:14:38

KERN_NIC_DRIVER=ixgbe
DPDK_NIC_DRIVER=igb_uio

# Subnets

EXTERNAL_SUBNET=192.168.4.0

# Middlebox

# icnalsp3s2
MB_HOST=icnalsp3s2.epfl.ch
MB_MAC_INTERNAL=90:e2:ba:55:14:11
MB_MAC_EXTERNAL=90:e2:ba:55:14:10
MB_DEVICE_INTERNAL=p802p2
MB_DEVICE_EXTERNAL=p802p1
MB_PCI_INTERNAL=0000:83:00.1
MB_PCI_EXTERNAL=0000:83:00.0
MB_IP_INTERNAL=192.168.6.2
MB_IP_EXTERNAL=192.168.4.2
MB_IPS_BACKENDS="192.168.4.3 192.168.4.4 192.168.4.5 192.168.4.6"


# Tester

# icnalsp3s1
TESTER_HOST=icnalsp3s1.epfl.ch
TESTER_DEVICE_INTERNAL=p802p2
TESTER_DEVICE_EXTERNAL=p802p1
TESTER_PCI_INTERNAL=0000:83:00.1 #dev8
TESTER_PCI_EXTERNAL=0000:83:00.0 #dev7
TESTER_MAC_INTERNAL=90:e2:ba:55:12:25
TESTER_MAC_EXTERNAL=90:e2:ba:55:12:24
TESTER_IP_INTERNAL=192.168.6.5
TESTER_IP_EXTERNAL=192.168.4.10

# Other

CASE_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}")"/../../../../  >/dev/null && pwd )"

# Fix the case root folder for the tester, where it should be just home
if [ "${CASE_ROOT##/home/}" == "${CASE_ROOT}" ]; then
  CASE_ROOT=$HOME
fi

export RTE_SDK=$CASE_ROOT/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc
