/*
 commodore ieee 488 (parallel) bus
*/

/*

                                 IEEE-488

                            by Ruud Baltissen


DISCLAIMER
-    All names with a copyright are acknowledged.
-    If the reader uses information from these documents to write software
     or build hardware, then it is on his own account. I cannot be held
     responsible for blowing up your computer, mother-in-law or whatever; it
     will always be your own fault.
-    I'm not a sexist, with 'he' and 'him' I also mean 'she' and 'her'.

Copyrights
-    You may copy every bit on this page for NON-commercial use. I hope you
     enjoy it. If you use it, just give me at least some credit like "Stolen
     from Ruud" :-)


The story of this document

I own several CBM/PET-types equipped with an IEEE-488 interface. Due to the
lack of any documentation on Internet about the technical background of this
subject, I started to collect all kind information I could lay my hands on to
make my own electronic manual.
Everything in this manual is based on the original concept. Where Commodore
used or did something else, I will mention this


The history

The IEEE-488 bus was developed by Hewlett-Packard to connect and control
programmable instruments and they called it HP-IB. The interface quickly
became very popular in the computer industry. The IEEE committee gave it its
present number and renamed it to GPIB (General Purpose Interface Bus).
Commodore started to use it to connect their floppydisks to their computers.
One of my manuals said that it is used to connect printers as well but to be
honest, I never heard of or saw a printer with an IEEE-interface.


IEEE-488 Overview

You can connect three type of devices to the IEEE-488 bus:
-    Listeners
-    Talkers
-    Controllers

Some devices can perform more than one of these functions. The standard allows
a maximum of 15 devices to be connected on the same bus. A minimum system
consists of one Controller and one Talker or Listener device (i.e. a 8032 and
a printer).
A Listener is a device that can receive data from the bus when instructed by
the Active Controller and a Talker transmits data on to the bus when
instructed. It is possible to have several Controllers on the bus but only one
may be active at any given time. The Active Controller may pass control to
another controller which in turn can pass it back or on to another controller.
The Controller can set up a talker and a group of listeners so that it is
possible to send data between groups of devices as well.
If you have more Controllers connected to each other, only one can be active
at the startup-time. This Controller is called the System Controller. In our
case it is obvious that this will be the CBM. This means that officially only
one CBM is allowed on the bus according to IEEE-488. In fact you can connect
more CBMs to the same bus without any problem as long as you take care that
only one is performing a command at the time. In IEEE-488 terms: you, the
user, acts as the System Controller, allowing one of the connected CBMs to be
a temporary Active Controller for the time being.


Interface Signals

The IEEE-488 interface system consists of 16 signal lines and 8 ground lines.
The 16 signal lines are divided into 3 groups: 8 data lines, 3 handshakelines,
and 5 interface management lines.

     Data Lines
     The lines DIO1 through DIO8 are used to transfer addresses, control
     information and data. The formats for addresses and control bytes are
     defined by the IEEE-488 standard. DIO1 is the Least Significant Bit,
     corresponding to bit 0 on a CBM.

     Handshake Lines
     The three handshakelines (NRFD, NDAC, DAV) control the transfer of
     message bytes among the devices and form the method for acknowledging
     the transfer of data. This handshaking process guarantees that the
     bytes on the data lines are sent and received without any transmission
     errors and is one of the unique features of the IEEE-488 bus.
     The NRFD (Not Ready for Data) handshake line is asserted by a Listener
     to indicate it is not yet ready for the next data or control byte. Note
     that the CBM will not see NRFD released (i.e., ready for data) until
     all devices have released it.
     The NDAC (Not Data Accepted) handshake line is asserted by a Listener
     to indicate it has not yet accepted the data or control byte on the
     data lines. Note that the CBM will not see NDAC released (i.e., data
     accepted) until all devices have released it.
     The DAV (DAta Valid) handshake line is asserted by the Talker to
     indicate that a data or control byte has been placed on the data lines
     and has had the minimum specified stabilizing time. The byte can now be
     safely accepted by the devices.

     Handshaking
     The handshaking process is performed as follows. When the CBM or a
     Talker (i.e.: floppydisk) wishes to transmit data on the bus, it sets
     the DAV line high, data is not valid, and checks to see that the NRFD
     and NDAC lines are both low, and then it puts the data on the data
     lines. When all the devices that can receive the data are ready, each
     releases its NRFD (Not Ready For Data) line. When the last receiver
     releases NRFD, and it goes high, the CBM or Talker takes DAV low
     indicating that valid data is now on the bus. In response each receiver
     takes NRFD low again to indicate it is busy and releases NDAC (Not Data
     Accepted) when it has received the data. When the last receiver has
     accepted the data, NDAC will go high and the CBM or Talker can set DAV
     high again to transmit the next byte of data. Note that if after
     setting the DAV line high, the Controller or Talker senses that both
     NRFD and NDAC are high, an error will occur. Also if any device fails
     to perform its part of the handshake and releases either NDAC or NRFD,
     data cannot be transmitted over the bus. Eventually a timeout error
     will be generated.
     The speed of the data transfer is controlled by the response of the
     slowest device on the bus, for this reason it is difficult to estimate
     data transfer rates on the IEEE-488 bus as they are always device
     dependent.

     Interface Management Lines
     The five interface management lines (ATN, EOI, IFC, REN, SRQ) manage
     the flow of control and data bytes across the interface.
     The ATN (Attention) signal is asserted by the Controller to indicate
     that it is placing an address or control byte on the data bus. ATN is
     released to allow the assigned Talker to place status or data on the
     data bus. The Controller regains control by reasserting ATN; this is
     normally done synchronously with the handshake to avoid confusion
     between control and data bytes.
     The EOI (End or Identify) signal has two uses. A Talker may assert EOI
     simultaneously with the last byte of data to indicate end-of-data. The
     CBM may assert EOI along with ATN to initiate a parallel poll. Although
     many devices do not use parallel poll, all devices should use EOI to
     end transfers (many currently available ones do not).
     The IFC (Interface Clear) signal is asserted only by the System
     Controller in order to initialize all device interfaces to a known
     state. After releasing IFC, the System Controller is the Active
     Controller. In the CBMs IFC is operated by the RESET-line.
     The REN (Remote Enable) signal is asserted only by the System
     Controller. Its assertion does not place devices into remote control
     mode; REN only enables a device to go into remote mode when addressed
     to listen. When in remote mode, a device should ignore its local front
     panel controls. This line is connected to ground inside the Commodores.
     The SRQ (Service Request) line is like an interrupt: it may be asserted
     by any device to request the Controller to take some action. The
     Controller must determine which device is asserting SRQ by conducting
     a serial poll. The requesting device releases SRQ when it is polled.


Resulting scheme

Line         NRFD   NDAC   DAV    ATN    EOI    IFC    REN    SQR
             I  O | I  O | I  O | I  O | I  O | I  O | I  O | I  O |
-----------|------|------|------|------|------|------|------|------|
Listener   |    X |    X | X    | X    | X    | X    | X    |    X |
Talker     | X    | X    |    X | X    |    X | X    | X    |    X |
Controller | X  X | X  X | X  X | X  X | X  X |    X |    G | X    |

X marks line used as "I = Input" or "O = Output"

The block Controller/REN is marked G because a CBM doesn't use this line. In
this case it is hardware-wired to GROUND.


Service Request and Serial Polling

If a device encounters a problem or a pre-programmed condition, it can
activate the SQR-line by setting it (L). If the Controller detects this
request, it can initiate a 'Serial Poll' by transmitting the command SPE ($18)
on the bus. It then places an address von the bus, activates ATN (which makes
the addressed device a talker) an de-activates it.
If the device requested service, it will respond by setting DIO7 (L). It may
indicate the nature of the request by setting other lines low as well. The
Controller can now take appropriate actions.  When all devices have been
polled, the controller terminates the serial poll by issuing the command SPD
($19).


Device Addresses
The IEEE-488 standard allows up to 15 devices to be interconnected on one bus.
Each device is assigned a unique primary address, ranging from 4-30, by
setting the address switches on the device. A secondary address may also be
specified, ranging from 0-31. See the device documentation for more
information on how to set the device primary and optional secondary address.


The hardware

Looking at the circuitdiagrams of a PET, like the 3008, then you can see that
the MC3446 is used as the driver for all the signals. According to "The PET
revealed" by Nick Hampshire, these ICs have 'Open Collector'-outputs which
means you can interconnect them without any fear of 'blowing up' a computer
or device.
Other models are equipped with the SN75160 and the SN75161  from Texas
Instruments. These ICs are drivers especially developed for IEEE-488.
The major difference between the two types is that you need one separate in-
and outputline for every IEEE-line, together 32 lines. This means you need at
least two 6522s or 6520s. For the SN-drivers you only need 19 lines or one
6525.
The 720 is equipped with the SN-drivers. The Pet 3008, CBM 8032SK, 3040 and
4040 are equipped with four MC3446s. The CBM 8296 uses one 7417 and three
MC3446s.


The SN75160

You can roughly describe the 75160 as a bidirectional 8-bits bus interface.
(like the 74245)


The SN75161

The SN75161 is used to buffer the handshake- and interface managementlines.
There is some internal logic to steer the output as follows:

                                         |  ATN
                                |  NDAC  |  IFC
                                |  NRFD  |  REN
  ATN  |  TE   |  DC   ||  EOI  |  DAVV  |  SQR
-------------------------------------------------
   L   |   L   |   L   ||   O   |   O    |   O
   L   |   L   |   H   ||   I   |   O    |   I
   L   |   H   |   L   ||   O   |   I    |   O
   L   |   H   |   H   ||   I   |   I    |   I
   H   |   L   |   L   ||   O   |   O    |   O
   H   |   L   |   H   ||   O   |   O    |   I
   H   |   H   |   L   ||   I   |   I    |   O
   H   |   H   |   H   ||   I   |   I    |   I

In words: ATN, IFC, REN and SQR are outputs if DC = (L), else inputs.
DAVV,NDAC and NRFD are outputs if TE = (L), else inputs. EOI behaves like DAVV
when ATN = (H), else like SQR.


Physical Characteristics

You can link devices in either a linear, star or combination configuration
using a shielded 24-conductor cable. The standard IEEE-488 cable has both a
plug and receptacle connector on both ends. This connector is the Amphenol
CHAMP or Cinch Series 57 MICRO RIBBON type. (it looks like a Centronics
connector, but smaller) As far as I know all Commodore periphiral devices,
like floppy-disks, and the CBM-8xxx series are equiped with this type of
connector.The other CBMs and PETs are equiped with a male edge connector.
(like the userport of a C64 or VIC-20)


Pin  Signal              Abbreviation  Source
---  ----------------    ------------  ------------------
 1   Data Bit 1          DIO1          Talker
 2   Data Bit 2          DIO2          Talker
 3   Data Bit 3          DIO3          Talker
 4   Data Bit 4          DIO4          Talker
 5   End Or Indentity    EOI           Talker/Controller
 6   Data Valid          DAV           Controller
 7   Not Ready For Data  NRFD          Listener
 8   No Data Accepted    NDAC          Listener
 9   Interface Clear     IFC           Controller
10   Service Request     SRQ           Talker
11   Attention           ATN           Controller
12   Shield
13   Data Bit 5          DIO5          Talker
14   Data Bit 6          DIO6          Talker
15   Data Bit 7          DIO7          Talker
16   Data Bit 8          DIO8          Talker
17   Remote Enabled      REN           Controller
18   Ground DAV
19   Ground NRFD
20   Ground NDAC
21   Ground IFC
22   Ground SRQ
23   Ground ATN
24   Logical Ground




                                 Credits:

-    Gary Pfeiffer, Hewlett-Packard
-    Nick Hampshire
-    Marko Makela



                           You can reach me at:

                         rbaltiss@worldaccess.nl


      My Commodore Site: http://www.worldaccess.nl/~rbaltiss/cbm.htm
 */

#include "driver.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/cbmdrive.h"
#include "includes/cbmserb.h"

#include "includes/cbmieeeb.h"

/*
 data 8 bit input/output

 all handshake lines low active

 handshake lines:
 dav data valid (asserted by talker)
 nrfd nor ready for data (asserted by listener)
 ndac not data accepted (asserted by listener)

 atn attention
 eoi end of information

 ifc
 ren
 srq serial request master input, other output
 */

static struct {
	struct {
		UINT8 data;
/*		int fic; */
		int dav, nrfd, ndac, atn, eoi, ren;
		int srq;
	} bus[2];
} cbmieee= { { { 0 } } };

void cbm_ieee_open(void)
{
	int i;
	for (i=0; i<2; i++) {
		cbmieee.bus[i].dav=1;
		cbmieee.bus[i].nrfd=1;
		cbmieee.bus[i].ndac=1;
		cbmieee.bus[i].atn=1;
		cbmieee.bus[i].eoi=1;
		cbmieee.bus[i].srq=1;

		cbmieee.bus[i].data=0xff;
	}
}

void cbm_ieee_dav_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee dav",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].dav=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_nrfd_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee nrfd",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].nrfd=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_ndac_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee ndac",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].ndac=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_atn_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee atn",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].atn=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_eoi_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee eoi",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].eoi=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_data_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee data",("%.4x dev:%d %.2x\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].data=data;
	if (device==0) c2031_state(cbm_drive);
}

int cbm_ieee_srq_r(void)
{
	int data=cbmieee.bus[1].srq;
	DBG_LOG(1,"cbm ieee srq",("read %d\n", data));
	return data;
}

int cbm_ieee_dav_r(void)
{
	int data=cbmieee.bus[0].dav&&cbmieee.bus[1].dav;
	DBG_LOG(1,"cbm ieee dav",("read %d\n", data));
	return data;
}

int cbm_ieee_nrfd_r(void)
{
	int data=cbmieee.bus[0].nrfd && cbmieee.bus[1].nrfd;
	DBG_LOG(1,"cbm ieee nrfd",("read %d\n", data));
	return data;
}

int cbm_ieee_ndac_r(void)
{
	int data=cbmieee.bus[0].ndac&&cbmieee.bus[1].ndac;
	DBG_LOG(1,"cbm ieee ndac",("read %d\n", data));
	return data;
}

int cbm_ieee_atn_r(void)
{
	int data=cbmieee.bus[0].atn&&cbmieee.bus[1].atn;
	DBG_LOG(1,"cbm ieee atn",("read %d\n", data));
	return data;
}

int cbm_ieee_eoi_r(void)
{
	int data=cbmieee.bus[0].eoi&&cbmieee.bus[1].eoi;
/*	DBG_LOG(1,"cbm ieee eoi",("read %d\n", data)); */
	return data;
}

int cbm_ieee_data_r(void)
{
	int data=cbmieee.bus[0].data&cbmieee.bus[1].data;
	DBG_LOG(1,"cbm ieee data",("read %.2x\n", data));
	return data;
}


 READ8_HANDLER(cbm_ieee_state)
{
	switch (offset) {
	case 0: return cbmieee.bus[0].data;
	case 1: return cbmieee.bus[0].atn;
	case 2: return cbmieee.bus[0].dav;
	case 3: return cbmieee.bus[0].ndac;
	case 4: return cbmieee.bus[0].nrfd;
	case 5: return cbmieee.bus[0].eoi;
	case 6: return cbmieee.bus[0].ren;
	case 7: return cbmieee.bus[0].srq;

	case 0x10: return cbmieee.bus[1].data;
	case 0x11: return cbmieee.bus[1].atn;
	case 0x12: return cbmieee.bus[1].dav;
	case 0x13: return cbmieee.bus[1].ndac;
	case 0x14: return cbmieee.bus[1].nrfd;
	case 0x15: return cbmieee.bus[1].eoi;
	case 0x16: return cbmieee.bus[1].ren;
	case 0x17: return cbmieee.bus[1].srq;
	}
	return 0;
}
