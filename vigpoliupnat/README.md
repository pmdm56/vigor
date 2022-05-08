## Policer (WAN=1 LAN=0)

- Network topology assumes that there's only 1 internal and external device.
- On external flows, drop packets based on whether packets are within 
the policing rate. 
- Forwarding packets from internal devices are not policed.

## Firewall (WAN=1 LAN=0,...)

- Network topology assumes that theres only 1 external device, but several internal devices.
- Drop incoming packets based on whether packets belong to a known flow.

## Poliwall (Same configuration of network topology) (WAN=1 LAN=0)

- Network topology assumes that there's only 1 external and internal device (conflict on the number of internal devices solved by using policer topology)

- Packets from within the network are associated with a known flow (firewall behavior)

- Packets from outside the network can be dropped whether they do not belong to a known flow or if they are outside the policing rate (combining the resultant behavior of both network functions - if at least 1 drop)



