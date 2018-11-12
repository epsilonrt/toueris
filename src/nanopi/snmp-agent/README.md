# How to build ?

This subagent can control 3 coils on Modbus CTOR slave (@8).

**`snmpd` must be installed with configuration files in ../etc/snmp and started.**

## Install LPO-ROUVIERE-MIB

		$ mkdir -p ~/.snmp/mibs && cp ../LPO-ROUVIERE-MIB.txt ~/.snmp/mibs/
		$ sudo service snmpd restart

Verify with :

		$ snmptranslate -M+. -mLPO-ROUVIERE-MIB -Tp -IR lpoRouviereMIB
			+--lpoRouviereMIB(52900)
				 |
				 +--lprMIBObjects(1)
				 |  |
				 |  +--lprSnmpTutorial(1)
				 |  |  |
				 |  |  +--lprSnmpTutorialLeds(1)
				 |  |  |  |
				 |  |  |  +-- -RW- Integer32 led1(1)
				 |  |  |  |        Range: 0..1
				 |  |  |  +-- -RW- Integer32 led2(2)
				 |  |  |  |        Range: 0..1
				 |  |  |  +-- -RW- Integer32 led3(3)
				 |  |  |           Range: 0..1
				 |  |  |
				 |  |  +--lprSnmpTutorialButtons(2)
				 |  |     |
				 |  |     +-- -R-- Integer32 sw1(1)
				 |  |     |        Range: 0..1
				 |  |     +-- -R-- Integer32 sw2(2)
				 |  |     |        Range: 0..1
				 |  |     +-- -R-- Integer32 sw3(3)
				 |  |              Range: 0..1
				 |  |
				 |  +--lprPointCast(2)
				 |     |
				 |     +--lprPointCastCoils(1)
				 |     |  |
				 |     |  +-- -RW- Integer32 coil1(1)
				 |     |  |        Range: 0..1
				 |     |  +-- -RW- Integer32 coil2(2)
				 |     |  |        Range: 0..1
				 |     |  +-- -RW- Integer32 coil3(3)
				 |     |           Range: 0..1
				 |     |
				 |     +--lprPointCastDiscreteInputs(2)
				 |     |
				 |     +--lprPointCastInputRegisters(3)
				 |
				 +--lprMIBConformance(2)

## Build pointcast-agent with Codelite

You need to install the 
[piduino](https://github.com/epsilonrt/piduino/wiki/Install-and-configure) 
and [libmodbus](https://github.com/epsilonrt/mbpoll#installation) 
libraries then you can build with codelite.

## Test pointcast-agent

		$ sudo Debug/pointcast-agent

* Reads coils

      $ snmpwalk -v1 -cepsilonrt localhost LPO-ROUVIERE-MIB::lprPointCastCoils
        LPO-ROUVIERE-MIB::coil1.0 = INTEGER: 0
        LPO-ROUVIERE-MIB::coil2.0 = INTEGER: 0
        LPO-ROUVIERE-MIB::coil3.0 = INTEGER: 0

* Set coil1

      $ snmpset -v1 -cepsilonrt localhost LPO-ROUVIERE-MIB::coil1.0 i 1
        LPO-ROUVIERE-MIB::coil1.0 = INTEGER: 1

* Clear coil1

      $ snmpset -v1 -cepsilonrt localhost LPO-ROUVIERE-MIB::coil1.0 i 0
        LPO-ROUVIERE-MIB::coil1.0 = INTEGER: 0

* remote access

      $ snmpwalk -v1 -cepsilonrt n12 LPO-ROUVIERE-MIB::lprPointCastCoils
      $ snmpset -v1 -cepsilonrt n12 LPO-ROUVIERE-MIB::coil1.0 i 1
      $ snmpset -v1 -cepsilonrt n12 LPO-ROUVIERE-MIB::coil1.0 i 0
