###########################################################################
#
#   mess.mak
#
#   MESS target makefile
#
###########################################################################



###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################

# uncomment next line to build imgtool
BUILD_IMGTOOL = 1

# uncomment next line to build castool
BUILD_CASTOOL = 1

# uncomment next line to build wimgtool
BUILD_WIMGTOOL = 1

# uncomment next line to build messtest
BUILD_MESSTEST = 1

# uncomment next line to build dat2html
BUILD_DAT2HTML = 1


###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################



# include MESS core defines
include $(SRC)/mess/messcore.mak


#-------------------------------------------------
# specify available CPU cores
#-------------------------------------------------

CPUS += ADSP21062
CPUS += ADSP21XX
CPUS += ALPHA8201
CPUS += APEXC
CPUS += ARM
CPUS += ARM7
CPUS += ASAP
CPUS += AVR8
CPUS += CCPU
CPUS += CDP1802
CPUS += COP400
CPUS += CP1610
CPUS += CUBEQCPU
CPUS += DSP32C
CPUS += DSP56156
CPUS += E1
CPUS += ESRIP
CPUS += F8
CPUS += G65816
CPUS += H6280
CPUS += H83002
CPUS += H83334
CPUS += HD6309
CPUS += I386
CPUS += I4004
CPUS += I8085
CPUS += I86
CPUS += I860
CPUS += I960
CPUS += JAGUAR
CPUS += KONAMI
CPUS += LH5801
CPUS += LR35902
CPUS += M37710
CPUS += M6502
CPUS += M6800
CPUS += M6805
CPUS += M6809
CPUS += M680X0
CPUS += MB86233
CPUS += MB88XX
CPUS += MC68HC11
CPUS += MCS48
CPUS += MCS51
CPUS += MINX
CPUS += MIPS
CPUS += NEC
CPUS += PDP1
CPUS += PIC16C5X
CPUS += POWERPC
CPUS += RSP
CPUS += S2650
CPUS += SATURN
CPUS += SC61860
CPUS += SE3208
CPUS += SH2
CPUS += SH4
CPUS += SM8500
CPUS += SPC700
CPUS += SSEM
CPUS += SSP1601
CPUS += SUPERFX
CPUS += T11
CPUS += TLCS90
CPUS += TLCS900
CPUS += TMS0980
CPUS += TMS32010
CPUS += TMS32025
CPUS += TMS32031
CPUS += TMS32051
CPUS += TMS340X0
CPUS += TMS57002
CPUS += TMS7000
CPUS += TMS9900
CPUS += UPD7810
CPUS += V30MZ
CPUS += V60
CPUS += V810
CPUS += Z180
CPUS += Z8
CPUS += Z80
CPUS += Z8000


#-------------------------------------------------
# specify available sound cores; some of these are
# only for MAME and so aren't included
#-------------------------------------------------

SOUNDS += AICA
SOUNDS += ASTROCADE
SOUNDS += AY8910
SOUNDS += BEEP
SOUNDS += C6280
SOUNDS += CDDA
SOUNDS += CDP1869
SOUNDS += DAC
SOUNDS += DISCRETE
SOUNDS += DMADAC
SOUNDS += ES5503
SOUNDS += K051649
SOUNDS += MSM5205
SOUNDS += NES
SOUNDS += OKIM6258
SOUNDS += OKIM6295
SOUNDS += POKEY
SOUNDS += PSXSPU
SOUNDS += QSOUND
SOUNDS += RF5C68
SOUNDS += SAA1099
SOUNDS += SCSP
SOUNDS += SID6581
SOUNDS += SID8580
SOUNDS += SN76477
SOUNDS += SN76496
SOUNDS += SOCRATES
SOUNDS += SP0256
SOUNDS += SPEAKER
SOUNDS += T6W28
SOUNDS += TIA
SOUNDS += TMC0285
SOUNDS += TMS5200
SOUNDS += TMS5220
SOUNDS += WAVE
SOUNDS += YM2151
SOUNDS += YM2203
SOUNDS += YM2413
SOUNDS += YM2610
SOUNDS += YM2610B
SOUNDS += YM2612
SOUNDS += YM3812
#SOUNDS += ADPCM
#SOUNDS += BSMT2000
#SOUNDS += C140
#SOUNDS += C352
#SOUNDS += CD2801
#SOUNDS += CD2802
#SOUNDS += CEM3394
#SOUNDS += CUSTOM
#SOUNDS += DIGITALKER
#SOUNDS += ES5505
#SOUNDS += ES5506
#SOUNDS += ES8712
#SOUNDS += GAELCO_CG1V
#SOUNDS += GAELCO_GAE1
#SOUNDS += HC55516
#SOUNDS += ICS2115
#SOUNDS += IREMGA20
#SOUNDS += K005289
#SOUNDS += K007232
#SOUNDS += K053260
#SOUNDS += K054539
#SOUNDS += M58817
#SOUNDS += MSM5232
#SOUNDS += MULTIPCM
#SOUNDS += NAMCO
#SOUNDS += NAMCO_15XX
#SOUNDS += NAMCO_52XX
#SOUNDS += NAMCO_63701X
#SOUNDS += NAMCO_CUS30
#SOUNDS += NILE
#SOUNDS += OKIM6376
#SOUNDS += RF5C400
#SOUNDS += S14001A
#SOUNDS += SAMPLES
#SOUNDS += SEGAPCM
#SOUNDS += SNKWAVE
#SOUNDS += SP0250
#SOUNDS += ST0016
#SOUNDS += TMC0281
#SOUNDS += TMS3615
#SOUNDS += TMS36XX
#SOUNDS += TMS5100
#SOUNDS += TMS5110
#SOUNDS += TMS5110A
#SOUNDS += UPD7759
#SOUNDS += VLM5030
#SOUNDS += VOTRAX
#SOUNDS += VRENDER0
#SOUNDS += X1_010
#SOUNDS += Y8950
#SOUNDS += YM2608
#SOUNDS += YM3438
#SOUNDS += YM3526
#SOUNDS += YMF262
#SOUNDS += YMF271
#SOUNDS += YMF278B
#SOUNDS += YMZ280B


#-------------------------------------------------
# this is the list of driver libraries that
# comprise MESS plus messdriv.o which contains
# the list of drivers
#-------------------------------------------------

DRVLIBS = \
	$(MESSOBJ)/messdriv.o \
	$(MESSOBJ)/3do.a \
	$(MESSOBJ)/acorn.a \
	$(MESSOBJ)/amiga.a \
	$(MESSOBJ)/amstrad.a \
	$(MESSOBJ)/apf.a \
	$(MESSOBJ)/apple.a \
	$(MESSOBJ)/applied.a \
	$(MESSOBJ)/arcadia.a \
	$(MESSOBJ)/ascii.a \
	$(MESSOBJ)/at.a \
	$(MESSOBJ)/atari.a \
	$(MESSOBJ)/bally.a \
	$(MESSOBJ)/bandai.a \
	$(MESSOBJ)/be.a \
	$(MESSOBJ)/bnpo.a \
	$(MESSOBJ)/bondwell.a \
	$(MESSOBJ)/booth.a \
	$(MESSOBJ)/camputers.a \
	$(MESSOBJ)/canon.a \
	$(MESSOBJ)/capcom.a \
	$(MESSOBJ)/cbm.a \
	$(MESSOBJ)/cbmshare.a \
	$(MESSOBJ)/cccp.a \
	$(MESSOBJ)/cce.a \
	$(MESSOBJ)/coleco.a \
	$(MESSOBJ)/comx.a \
	$(MESSOBJ)/concept.a \
	$(MESSOBJ)/conitec.a \
	$(MESSOBJ)/cybiko.a \
	$(MESSOBJ)/dai.a \
	$(MESSOBJ)/ddr.a \
	$(MESSOBJ)/dec.a \
	$(MESSOBJ)/dicksmth.a \
	$(MESSOBJ)/dragon.a \
	$(MESSOBJ)/drc.a \
	$(MESSOBJ)/eaca.a \
	$(MESSOBJ)/einis.a \
	$(MESSOBJ)/elektor.a \
	$(MESSOBJ)/elektrka.a \
	$(MESSOBJ)/entex.a \
	$(MESSOBJ)/epoch.a \
	$(MESSOBJ)/epson.a \
	$(MESSOBJ)/exeltel.a \
	$(MESSOBJ)/exidy.a \
	$(MESSOBJ)/fairch.a \
	$(MESSOBJ)/fujitsu.a \
	$(MESSOBJ)/galaxy.a \
	$(MESSOBJ)/gce.a \
	$(MESSOBJ)/grundy.a \
	$(MESSOBJ)/hartung.a \
	$(MESSOBJ)/heathkit.a \
	$(MESSOBJ)/hegener.a \
	$(MESSOBJ)/homebrew.a \
	$(MESSOBJ)/homelab.a \
	$(MESSOBJ)/hp.a \
	$(MESSOBJ)/intel.a \
	$(MESSOBJ)/intelgnt.a \
	$(MESSOBJ)/interton.a \
	$(MESSOBJ)/intv.a \
	$(MESSOBJ)/jupiter.a \
	$(MESSOBJ)/kaypro.a \
	$(MESSOBJ)/koei.a \
	$(MESSOBJ)/kyocera.a \
	$(MESSOBJ)/luxor.a \
	$(MESSOBJ)/magnavox.a \
	$(MESSOBJ)/matsushi.a \
	$(MESSOBJ)/mattel.a \
	$(MESSOBJ)/mchester.a \
	$(MESSOBJ)/memotech.a \
	$(MESSOBJ)/mgu.a \
	$(MESSOBJ)/microkey.a \
	$(MESSOBJ)/mit.a \
	$(MESSOBJ)/mos.a \
	$(MESSOBJ)/motorola.a \
	$(MESSOBJ)/multitch.a \
	$(MESSOBJ)/nakajima.a \
	$(MESSOBJ)/nascom.a \
	$(MESSOBJ)/ne.a \
	$(MESSOBJ)/nec.a \
	$(MESSOBJ)/netronic.a \
	$(MESSOBJ)/nintendo.a \
	$(MESSOBJ)/nokia.a \
	$(MESSOBJ)/novag.a \
	$(MESSOBJ)/olivetti.a \
	$(MESSOBJ)/orion.a \
	$(MESSOBJ)/osborne.a \
	$(MESSOBJ)/osi.a \
	$(MESSOBJ)/palm.a \
	$(MESSOBJ)/parker.a \
	$(MESSOBJ)/pc.a \
	$(MESSOBJ)/pcm.a \
	$(MESSOBJ)/pcshare.a \
	$(MESSOBJ)/pdp1.a \
	$(MESSOBJ)/pel.a \
	$(MESSOBJ)/philips.a \
	$(MESSOBJ)/poly88.a \
	$(MESSOBJ)/radio.a \
	$(MESSOBJ)/rca.a \
	$(MESSOBJ)/robotron.a \
	$(MESSOBJ)/rockwell.a \
	$(MESSOBJ)/samcoupe.a \
	$(MESSOBJ)/samsung.a \
	$(MESSOBJ)/sega.a \
	$(MESSOBJ)/sgi.a \
	$(MESSOBJ)/sharp.a \
	$(MESSOBJ)/sinclair.a \
	$(MESSOBJ)/skeleton.a \
	$(MESSOBJ)/snk.a \
	$(MESSOBJ)/sony.a \
	$(MESSOBJ)/sord.a \
	$(MESSOBJ)/special.a \
	$(MESSOBJ)/svi.a \
	$(MESSOBJ)/svision.a \
	$(MESSOBJ)/synertec.a \
	$(MESSOBJ)/tangerin.a \
	$(MESSOBJ)/tatung.a \
	$(MESSOBJ)/teamconc.a \
	$(MESSOBJ)/telenova.a \
	$(MESSOBJ)/telercas.a \
	$(MESSOBJ)/tem.a \
	$(MESSOBJ)/tesla.a \
	$(MESSOBJ)/thomson.a \
	$(MESSOBJ)/ti.a \
	$(MESSOBJ)/tiger.a \
	$(MESSOBJ)/tiki.a \
	$(MESSOBJ)/tomy.a \
	$(MESSOBJ)/trs.a \
	$(MESSOBJ)/unisys.a \
	$(MESSOBJ)/veb.a \
	$(MESSOBJ)/visual.a \
	$(MESSOBJ)/votrax.a \
	$(MESSOBJ)/vtech.a \
	$(MESSOBJ)/xerox.a \
	$(MESSOBJ)/zvt.a \
	$(MESSOBJ)/shared.a \



#-------------------------------------------------
# the following files are general components and
# shared across a number of drivers
#-------------------------------------------------

$(MESSOBJ)/shared.a: \
	$(MAME_MACHINE)/pckeybrd.o	\
	$(MESS_AUDIO)/cdp1863.o		\
	$(MESS_AUDIO)/lmc1992.o		\
	$(MESS_AUDIO)/mea8000.o		\
	$(MESS_DEVICES)/bitbngr.o	\
	$(MESS_DEVICES)/cartslot.o	\
	$(MESS_DEVICES)/cassette.o	\
	$(MESS_DEVICES)/chd_cd.o	\
	$(MESS_DEVICES)/dsk.o		\
	$(MESS_DEVICES)/flopdrv.o	\
	$(MESS_DEVICES)/harddriv.o	\
	$(MESS_DEVICES)/mflopimg.o	\
	$(MESS_DEVICES)/microdrv.o	\
	$(MESS_DEVICES)/multcart.o	\
	$(MESS_DEVICES)/printer.o	\
	$(MESS_DEVICES)/snapquik.o	\
	$(MESS_DEVICES)/z80bin.o	\
	$(MESS_FORMATS)/basicdsk.o	\
	$(MESS_FORMATS)/cassimg.o	\
	$(MESS_FORMATS)/coco_cas.o	\
	$(MESS_FORMATS)/coco_dsk.o	\
	$(MESS_FORMATS)/flopimg.o	\
	$(MESS_FORMATS)/ioprocs.o	\
	$(MESS_FORMATS)/pc_dsk.o	\
	$(MESS_FORMATS)/rk_cas.o	\
	$(MESS_FORMATS)/wavfile.o	\
	$(MESS_MACHINE)/6530miot.o	\
	$(MESS_MACHINE)/6551.o		\
	$(MESS_MACHINE)/68901mfp.o	\
	$(MESS_MACHINE)/74145.o		\
	$(MESS_MACHINE)/8530scc.o	\
	$(MESS_MACHINE)/adc080x.o	\
	$(MESS_MACHINE)/at29040.o	\
	$(MESS_MACHINE)/at45dbxx.o	\
	$(MESS_MACHINE)/ay31015.o	\
	$(MESS_MACHINE)/cdp1871.o	\
	$(MESS_MACHINE)/ctronics.o	\
	$(MESS_MACHINE)/com8116.o	\
	$(MESS_MACHINE)/e0516.o		\
	$(MESS_MACHINE)/hd63450.o   \
	$(MESS_MACHINE)/i8155.o		\
	$(MESS_MACHINE)/i8212.o		\
	$(MESS_MACHINE)/i8214.o		\
	$(MESS_MACHINE)/i82439tx.o	\
	$(MESS_MACHINE)/i8271.o		\
	$(MESS_MACHINE)/i8355.o		\
	$(MESS_MACHINE)/ins8154.o	\
	$(MESS_MACHINE)/ins8250.o	\
	$(MESS_MACHINE)/kb3600.o	\
	$(MESS_MACHINE)/kr2376.o	\
	$(MESS_MACHINE)/mc6843.o	\
	$(MESS_MACHINE)/mc6846.o	\
	$(MESS_MACHINE)/mc6854.o	\
	$(MESS_MACHINE)/mm58274c.o	\
	$(MESS_MACHINE)/mm74c922.o	\
	$(MESS_MACHINE)/mpc105.o	\
	$(MESS_MACHINE)/msm58321.o	\
	$(MESS_MACHINE)/msm8251.o	\
	$(MESS_MACHINE)/nec765.o	\
	$(MESS_MACHINE)/ncr5380.o	\
	$(MESS_MACHINE)/pc_lpt.o	\
	$(MESS_MACHINE)/pc_mouse.o	\
	$(MESS_MACHINE)/pcf8593.o	\
	$(MESS_MACHINE)/rp5c01a.o	\
	$(MESS_MACHINE)/rp5c15.o	\
	$(MESS_MACHINE)/serial.o	\
	$(MESS_MACHINE)/smartmed.o	\
	$(MESS_MACHINE)/smc92x4.o	\
	$(MESS_MACHINE)/sst39vfx.o	\
	$(MESS_MACHINE)/tc8521.o	\
	$(MESS_MACHINE)/upd1990a.o	\
	$(MESS_MACHINE)/upd7002.o	\
	$(MESS_MACHINE)/upd7201.o	\
	$(MESS_MACHINE)/wd17xx.o	\
	$(MESS_MACHINE)/z80dart.o	\
	$(MESS_MACHINE)/z80sti.o	\
	$(MESS_VIDEO)/cdp1861.o		\
	$(MESS_VIDEO)/cdp1862.o		\
	$(MESS_VIDEO)/cdp1864.o		\
	$(MESS_VIDEO)/crtc6845.o	\
	$(MESS_VIDEO)/dl1416.o		\
	$(MESS_VIDEO)/dm9368.o		\
	$(MESS_VIDEO)/hd44102.o		\
	$(MESS_VIDEO)/hd61830.o		\
	$(MESS_VIDEO)/hd66421.o		\
	$(MESS_VIDEO)/i82720.o		\
	$(MESS_VIDEO)/i8275.o		\
	$(MESS_VIDEO)/m6845.o		\
	$(MESS_VIDEO)/m6847.o		\
	$(MESS_VIDEO)/msm6255.o		\
	$(MESS_VIDEO)/saa5050.o		\
	$(MESS_VIDEO)/saa505x.o		\
	$(MESS_VIDEO)/sed1330.o		\
	$(MESS_VIDEO)/tms3556.o		\
	$(MESS_VIDEO)/upd7220.o		\




#-------------------------------------------------
# manufacturer-specific groupings for drivers
#-------------------------------------------------

$(MESSOBJ)/3do.a:				\
	$(MESS_DRIVERS)/3do.o		\
	$(MESS_MACHINE)/3do.o		\

$(MESSOBJ)/acorn.a:				\
	$(MESS_DRIVERS)/a6809.o		\
	$(MESS_DRIVERS)/acrnsys1.o	\
	$(MESS_VIDEO)/bbc.o			\
	$(MESS_MACHINE)/bbc.o		\
	$(MESS_DRIVERS)/bbc.o		\
	$(MESS_DRIVERS)/bbcbc.o		\
	$(MESS_DRIVERS)/a310.o		\
	$(MAME_MACHINE)/archimds.o	\
	$(MESS_DRIVERS)/z88.o		\
	$(MESS_VIDEO)/z88.o			\
	$(MESS_VIDEO)/atom.o		\
	$(MESS_DRIVERS)/atom.o		\
	$(MESS_MACHINE)/atom.o		\
	$(MESS_FORMATS)/uef_cas.o	\
	$(MESS_FORMATS)/csw_cas.o	\
	$(MESS_VIDEO)/electron.o	\
	$(MESS_MACHINE)/electron.o	\
	$(MESS_DRIVERS)/electron.o	\

$(MESSOBJ)/amiga.a:				\
	$(MAME_VIDEO)/amiga.o		\
	$(MAME_VIDEO)/amigaaga.o	\
	$(MAME_MACHINE)/amiga.o		\
	$(MAME_AUDIO)/amiga.o		\
	$(MESS_MACHINE)/amigafdc.o	\
	$(MESS_MACHINE)/amigacrt.o	\
	$(MESS_MACHINE)/amigacd.o	\
	$(MESS_MACHINE)/matsucd.o	\
	$(MESS_MACHINE)/amigakbd.o	\
	$(MESS_DRIVERS)/amiga.o		\
	$(MAME_MACHINE)/cubocd32.o	\
	$(MESS_DRIVERS)/ami1200.o	\

$(MESSOBJ)/amstrad.a:			\
	$(MESS_DRIVERS)/amstrad.o	\
	$(MESS_MACHINE)/amstrad.o	\
	$(MESS_VIDEO)/pcw.o			\
	$(MESS_DRIVERS)/pcw.o		\
	$(MESS_DRIVERS)/pcw16.o		\
	$(MESS_VIDEO)/pcw16.o		\
	$(MESS_VIDEO)/nc.o			\
	$(MESS_DRIVERS)/nc.o		\
	$(MESS_MACHINE)/nc.o		\

$(MESSOBJ)/apf.a:				\
	$(MESS_DRIVERS)/apf.o		\
	$(MESS_VIDEO)/apf.o			\
	$(MESS_FORMATS)/apf_apt.o	\

$(MESSOBJ)/apple.a:				\
	$(MESS_VIDEO)/apple2.o		\
	$(MESS_MACHINE)/apple2.o	\
	$(MESS_DRIVERS)/apple2.o	\
	$(MESS_VIDEO)/apple2gs.o	\
	$(MESS_MACHINE)/apple2gs.o	\
	$(MESS_DRIVERS)/apple2gs.o	\
	$(MESS_FORMATS)/ap2_dsk.o	\
	$(MESS_FORMATS)/ap_dsk35.o	\
	$(MESS_MACHINE)/ay3600.o	\
	$(MESS_MACHINE)/ap2_slot.o	\
	$(MESS_MACHINE)/ap2_lang.o	\
	$(MESS_MACHINE)/mockngbd.o	\
	$(MESS_MACHINE)/lisa.o		\
	$(MESS_DRIVERS)/lisa.o		\
	$(MESS_MACHINE)/applefdc.o	\
	$(MESS_DEVICES)/sonydriv.o	\
	$(MESS_DEVICES)/appldriv.o	\
	$(MESS_AUDIO)/mac.o			\
	$(MESS_VIDEO)/mac.o			\
	$(MESS_MACHINE)/mac.o		\
	$(MESS_DRIVERS)/mac.o		\
	$(MESS_VIDEO)/apple1.o		\
	$(MESS_MACHINE)/apple1.o	\
	$(MESS_DRIVERS)/apple1.o	\
	$(MESS_VIDEO)/apple3.o		\
	$(MESS_MACHINE)/apple3.o	\
	$(MESS_DRIVERS)/apple3.o	\

$(MESSOBJ)/applied.a:			\
	$(MESS_VIDEO)/mbee.o		\
	$(MESS_MACHINE)/mbee.o		\
	$(MESS_DRIVERS)/mbee.o		\

$(MESSOBJ)/arcadia.a:			\
	$(MESS_DRIVERS)/arcadia.o	\
	$(MESS_AUDIO)/arcadia.o		\
	$(MESS_VIDEO)/arcadia.o		\

$(MESSOBJ)/ascii.a:				\
	$(MESS_FORMATS)/fmsx_cas.o	\
	$(MESS_DRIVERS)/msx.o		\
	$(MESS_MACHINE)/msx_slot.o	\
	$(MESS_MACHINE)/msx.o		\

$(MESSOBJ)/at.a:				\
	$(MESS_MACHINE)/pc_ide.o	\
	$(MESS_MACHINE)/ps2.o		\
	$(MESS_MACHINE)/at.o		\
	$(MESS_DRIVERS)/at.o		\

$(MESSOBJ)/atari.a:				\
	$(MAME_VIDEO)/tia.o			\
	$(MAME_MACHINE)/atari.o		\
	$(MAME_VIDEO)/atari.o		\
	$(MAME_VIDEO)/antic.o		\
	$(MAME_VIDEO)/gtia.o		\
	$(MESS_MACHINE)/ataricrt.o	\
	$(MESS_MACHINE)/atarifdc.o	\
	$(MESS_DRIVERS)/atari.o		\
	$(MESS_MACHINE)/a7800.o		\
	$(MESS_DRIVERS)/a7800.o		\
	$(MESS_VIDEO)/a7800.o		\
	$(MESS_DRIVERS)/a2600.o		\
	$(MESS_DRIVERS)/jaguar.o	\
	$(MAME_AUDIO)/jaguar.o		\
	$(MAME_VIDEO)/jaguar.o		\
	$(MESS_FORMATS)/a26_cas.o	\
	$(MESS_FORMATS)/atarist_dsk.o	\
	$(MESS_DRIVERS)/atarist.o 	\
	$(MESS_VIDEO)/atarist.o 	\
	$(MESS_DRIVERS)/lynx.o		\
	$(MESS_AUDIO)/lynx.o		\
	$(MESS_MACHINE)/lynx.o		\

$(MESSOBJ)/bally.a:				\
	$(MAME_VIDEO)/astrocde.o	\
	$(MESS_DRIVERS)/astrocde.o	\

$(MESSOBJ)/bandai.a:			\
	$(MESS_DRIVERS)/pippin.o	\
	$(MESS_DRIVERS)/wswan.o		\
	$(MESS_MACHINE)/wswan.o		\
	$(MESS_VIDEO)/wswan.o		\
	$(MESS_AUDIO)/wswan.o		\

$(MESSOBJ)/be.a:				\
	$(MESS_DRIVERS)/bebox.o		\
	$(MESS_MACHINE)/bebox.o		\
	$(MESS_VIDEO)/cirrus.o		\

$(MESSOBJ)/bnpo.a:				\
	$(MESS_DRIVERS)/b2m.o		\
	$(MESS_MACHINE)/b2m.o		\
	$(MESS_VIDEO)/b2m.o			\

$(MESSOBJ)/bondwell.a:			\
	$(MESS_DRIVERS)/bw2.o		\
	$(MESS_DRIVERS)/bw12.o		\

$(MESSOBJ)/booth.a:				\
	$(MESS_DRIVERS)/apexc.o		\

$(MESSOBJ)/camputers.a:			\
	$(MESS_DRIVERS)/camplynx.o	\

$(MESSOBJ)/canon.a:				\
	$(MESS_DRIVERS)/cat.o		\
	$(MESS_DRIVERS)/x07.o		\

$(MESSOBJ)/capcom.a:			\
	$(MESS_DRIVERS)/cpschngr.o	\
	$(MESS_VIDEO)/cpschngr.o	\
	$(MAME_MACHINE)/kabuki.o	\

$(MESSOBJ)/cbm.a:				\
	$(MESS_VIDEO)/pet.o			\
	$(MESS_DRIVERS)/pet.o		\
	$(MESS_MACHINE)/pet.o		\
	$(MESS_DRIVERS)/c64.o		\
	$(MESS_MACHINE)/vc20.o		\
	$(MESS_DRIVERS)/vc20.o		\
	$(MESS_AUDIO)/ted7360.o		\
	$(MESS_AUDIO)/t6721.o		\
	$(MESS_MACHINE)/c16.o		\
	$(MESS_DRIVERS)/c16.o		\
	$(MESS_DRIVERS)/cbmb.o		\
	$(MESS_MACHINE)/cbmb.o		\
	$(MESS_VIDEO)/cbmb.o		\
	$(MESS_DRIVERS)/c65.o		\
	$(MESS_DRIVERS)/c128.o		\
	$(MESS_AUDIO)/vic6560.o		\
	$(MESS_VIDEO)/ted7360.o		\
	$(MESS_VIDEO)/vic6560.o		\

$(MESSOBJ)/cbmshare.a:			\
	$(MESS_FORMATS)/cbm_tap.o 	\
	$(MESS_MACHINE)/6525tpi.o	\
	$(MESS_MACHINE)/cbm.o		\
	$(MESS_MACHINE)/cbmdrive.o	\
	$(MESS_MACHINE)/vc1541.o	\
	$(MESS_MACHINE)/cbmieeeb.o 	\
	$(MESS_MACHINE)/cbmserb.o 	\
	$(MESS_MACHINE)/cbmipt.o   	\
	$(MESS_MACHINE)/c64.o    	\
	$(MESS_MACHINE)/c65.o		\
	$(MESS_MACHINE)/c128.o		\
	$(MESS_VIDEO)/vdc8563.o		\
	$(MESS_VIDEO)/vic6567.o		\

$(MESSOBJ)/cccp.a:				\
	$(MESS_VIDEO)/lviv.o		\
	$(MESS_DRIVERS)/lviv.o		\
	$(MESS_MACHINE)/lviv.o		\
	$(MESS_FORMATS)/lviv_lvt.o	\
	$(MESS_DRIVERS)/mikro80.o	\
	$(MESS_MACHINE)/mikro80.o	\
	$(MESS_VIDEO)/mikro80.o		\
	$(MESS_DRIVERS)/pk8000.o	\
	$(MAME_VIDEO)/pk8000.o		\
	$(MESS_DRIVERS)/pk8020.o	\
	$(MESS_MACHINE)/pk8020.o	\
	$(MESS_VIDEO)/pk8020.o		\
	$(MESS_DRIVERS)/uknc.o		\
	$(MESS_DRIVERS)/ut88.o		\
	$(MESS_MACHINE)/ut88.o		\
	$(MESS_VIDEO)/ut88.o		\
	$(MESS_DRIVERS)/vector06.o	\
	$(MESS_MACHINE)/vector06.o	\
	$(MESS_VIDEO)/vector06.o	\

$(MESSOBJ)/cce.a:				\
	$(MESS_DRIVERS)/mc1000.o	\

$(MESSOBJ)/coleco.a:			\
	$(MESS_DRIVERS)/coleco.o	\
	$(MESS_MACHINE)/adam.o		\
	$(MESS_DRIVERS)/adam.o		\
	$(MESS_FORMATS)/adam_dsk.o	\

$(MESSOBJ)/comx.a:				\
	$(MESS_DRIVERS)/comx35.o	\
	$(MESS_FORMATS)/comx35_dsk.o	\
	$(MESS_DRIVERS)/comxpl80.o	\
	$(MESS_VIDEO)/comx35.o		\
	$(MESS_MACHINE)/comx35.o	\
	
$(MESSOBJ)/concept.a:			\
	$(MESS_DRIVERS)/concept.o   \
	$(MESS_MACHINE)/concept.o	\
	$(MESS_MACHINE)/corvushd.o	\

$(MESSOBJ)/conitec.a:			\
	$(MESS_DRIVERS)/prof80.o	\
	$(MESS_DRIVERS)/prof180x.o	\

$(MESSOBJ)/cybiko.a:			\
	$(MESS_DRIVERS)/cybiko.o	\
	$(MESS_MACHINE)/cybiko.o	\

$(MESSOBJ)/dai.a:				\
	$(MESS_DRIVERS)/dai.o		\
	$(MESS_VIDEO)/dai.o			\
	$(MESS_AUDIO)/dai.o			\
	$(MESS_MACHINE)/tms5501.o	\
	$(MESS_MACHINE)/dai.o		\

$(MESSOBJ)/ddr.a:				\
	$(MESS_DRIVERS)/ac1.o		\
	$(MESS_MACHINE)/ac1.o		\
	$(MESS_VIDEO)/ac1.o			\
	$(MESS_DRIVERS)/bcs3.o		\
	$(MESS_DRIVERS)/c80.o		\
	$(MESS_DRIVERS)/huebler.o	\
	$(MESS_DRIVERS)/jtc.o		\
	$(MESS_DRIVERS)/kramermc.o	\
	$(MESS_MACHINE)/kramermc.o	\
	$(MESS_VIDEO)/kramermc.o	\
	$(MESS_DRIVERS)/llc.o		\
	$(MESS_VIDEO)/llc.o			\
	$(MESS_MACHINE)/llc.o		\
	$(MESS_DRIVERS)/vcs80.o		\

$(MESSOBJ)/dec.a:				\
	$(MESS_DRIVERS)/dectalk.o	\
	$(MESS_DRIVERS)/vk100.o		\
	$(MESS_DRIVERS)/vt100.o		\
	$(MESS_DRIVERS)/vt220.o		\
	$(MESS_DRIVERS)/vt320.o		\
	$(MESS_DRIVERS)/vt520.o		\
	$(MESS_VIDEO)/vtvideo.o		\

$(MESSOBJ)/dicksmth.a:			\
	$(MESS_DRIVERS)/super80.o	\
	$(MESS_MACHINE)/super80.o	\
	$(MESS_VIDEO)/super80.o		\

$(MESSOBJ)/dragon.a:			\
	$(MESS_MACHINE)/dgn_beta.o	\
	$(MESS_VIDEO)/dgn_beta.o	\
	$(MESS_DRIVERS)/dgn_beta.o	\

$(MESSOBJ)/drc.a:				\
	$(MESS_DRIVERS)/zrt80.o		\

$(MESSOBJ)/eaca.a:				\
	$(MESS_DRIVERS)/cgenie.o	\
	$(MESS_VIDEO)/cgenie.o		\
	$(MESS_AUDIO)/cgenie.o		\
	$(MESS_MACHINE)/cgenie.o	\
	$(MESS_FORMATS)/cgen_cas.o	\

$(MESSOBJ)/einis.a:				\
	$(MESS_DRIVERS)/pecom.o		\
	$(MESS_MACHINE)/pecom.o		\
	$(MESS_VIDEO)/pecom.o		\

$(MESSOBJ)/elektrka.a:			\
	$(MESS_DRIVERS)/bk.o		\
	$(MESS_MACHINE)/bk.o		\
	$(MESS_VIDEO)/bk.o			\
	$(MESS_DRIVERS)/mk85.o		\
	$(MESS_DRIVERS)/mk90.o		\

$(MESSOBJ)/elektor.a:			\
	$(MESS_DRIVERS)/ec65.o		\
	$(MESS_DRIVERS)/junior.o	\
	
$(MESSOBJ)/entex.a:				\
	$(MESS_VIDEO)/advision.o	\
	$(MESS_MACHINE)/advision.o	\
	$(MESS_DRIVERS)/advision.o	\

$(MESSOBJ)/epoch.a:				\
	$(MESS_DRIVERS)/gamepock.o	\
	$(MESS_MACHINE)/gamepock.o	\

$(MESSOBJ)/epson.a:				\
	$(MESS_DRIVERS)/ex800.o		\
	$(MESS_DRIVERS)/lx800.o		\
	$(MESS_MACHINE)/e05a03.o	\
	$(MESS_MACHINE)/pf10.o		\
	$(MESS_DRIVERS)/px4.o		\
	$(MESS_DRIVERS)/px8.o		\
	$(MESS_DRIVERS)/qx10.o		\

$(MESSOBJ)/exeltel.a:			\
	$(MESS_DRIVERS)/exelv.o		\

$(MESSOBJ)/exidy.a:				\
	$(MESS_MACHINE)/exidy.o		\
	$(MESS_DRIVERS)/exidy.o		\
	$(MESS_VIDEO)/exidy.o		\

$(MESSOBJ)/fairch.a:			\
	$(MESS_VIDEO)/channelf.o	\
	$(MESS_AUDIO)/channelf.o	\
	$(MESS_DRIVERS)/channelf.o	\

$(MESSOBJ)/fujitsu.a:			\
	$(MESS_DRIVERS)/fmtowns.o	\
	$(MESS_DRIVERS)/fm7.o		\
	$(MESS_VIDEO)/fm7.o			\
	$(MESS_FORMATS)/fm7_cas.o	\
	$(MESS_FORMATS)/fm7_dsk.o	\

$(MESSOBJ)/galaxy.a:			\
	$(MESS_VIDEO)/galaxy.o		\
	$(MESS_DRIVERS)/galaxy.o	\
	$(MESS_MACHINE)/galaxy.o	\
	$(MESS_FORMATS)/gtp_cas.o	\

$(MESSOBJ)/gce.a:	           	\
	$(MESS_DRIVERS)/vectrex.o	\
	$(MESS_VIDEO)/vectrex.o		\
	$(MESS_MACHINE)/vectrex.o	\

$(MESSOBJ)/grundy.a:			\
	$(MESS_DRIVERS)/newbrain.o	\
	$(MESS_VIDEO)/newbrain.o	\

$(MESSOBJ)/hartung.a:			\
	$(MESS_DRIVERS)/gmaster.o	\
	$(MESS_AUDIO)/gmaster.o		\

$(MESSOBJ)/heathkit.a:			\
	$(MESS_DRIVERS)/et3400.o	\
	$(MESS_DRIVERS)/h8.o		\
	$(MESS_DRIVERS)/h19.o		\
	$(MESS_DRIVERS)/h89.o		\

$(MESSOBJ)/hegener.a:			\
	$(MESS_DRIVERS)/glasgow.o	\
	$(MESS_DRIVERS)/mephisto.o	\

$(MESSOBJ)/homebrew.a:			\
	$(MESS_DRIVERS)/craft.o		\

$(MESSOBJ)/homelab.a:			\
	$(MESS_DRIVERS)/homelab.o	\
	$(MESS_VIDEO)/homelab.o		\
	$(MESS_MACHINE)/homelab.o	\

$(MESSOBJ)/hp.a:				\
	$(MESS_MACHINE)/hp48.o		\
	$(MESS_VIDEO)/hp48.o		\
	$(MESS_DRIVERS)/hp48.o		\
	$(MESS_DEVICES)/xmodem.o	\
	$(MESS_DEVICES)/kermit.o	\

$(MESSOBJ)/intel.a:				\
	$(MESS_DRIVERS)/sdk86.o		\

$(MESSOBJ)/intelgnt.a:			\
	$(MESS_AUDIO)/dave.o		\
	$(MESS_VIDEO)/epnick.o		\
	$(MESS_DRIVERS)/enterp.o	\

$(MESSOBJ)/interton.a:			\
	$(MESS_AUDIO)/vc4000.o		\
	$(MESS_DRIVERS)/vc4000.o	\
	$(MESS_VIDEO)/vc4000.o		\

$(MESSOBJ)/intv.a:				\
	$(MESS_VIDEO)/intv.o		\
	$(MESS_VIDEO)/stic.o		\
	$(MESS_MACHINE)/intv.o		\
	$(MESS_AUDIO)/intv.o		\
	$(MESS_DRIVERS)/intv.o		\

$(MESSOBJ)/jupiter.a:			\
	$(MESS_DRIVERS)/jupiter.o	\
	$(MESS_MACHINE)/jupiter.o	\
	$(MESS_FORMATS)/jupi_tap.o	\

$(MESSOBJ)/kaypro.a:			\
	$(MESS_DRIVERS)/kaypro.o	\
	$(MESS_MACHINE)/kaypro.o	\
	$(MESS_VIDEO)/kaypro.o		\
	$(MESS_MACHINE)/kay_kbd.o	\

$(MESSOBJ)/koei.a:				\
	$(MESS_DRIVERS)/pasogo.o	\

$(MESSOBJ)/kyocera.a:			\
	$(MESS_DRIVERS)/kyocera.o	\
	$(MESS_VIDEO)/kyocera.o		\

$(MESSOBJ)/luxor.a:				\
	$(MESS_DRIVERS)/abc80.o		\
	$(MESS_VIDEO)/abc80.o		\
	$(MESS_DRIVERS)/abc80x.o	\
	$(MESS_VIDEO)/abc800.o		\
	$(MESS_VIDEO)/abc802.o		\
	$(MESS_VIDEO)/abc806.o		\
	$(MESS_MACHINE)/abcbus.o	\
	$(MESS_MACHINE)/abc77.o		\
	$(MESS_MACHINE)/abc99.o		\
	$(MESS_MACHINE)/conkort.o	\

$(MESSOBJ)/magnavox.a:			\
	$(MESS_MACHINE)/odyssey2.o	\
	$(MESS_VIDEO)/odyssey2.o	\
	$(MESS_DRIVERS)/odyssey2.o	\

$(MESSOBJ)/mattel.a:			\
	$(MESS_DRIVERS)/aquarius.o	\
	$(MESS_VIDEO)/aquarius.o	\

$(MESSOBJ)/matsushi.a:			\
	$(MESS_DRIVERS)/jr200.o		\

$(MESSOBJ)/mchester.a:			\
	$(MESS_DRIVERS)/ssem.o		\
	
$(MESSOBJ)/memotech.a:			\
	$(MESS_DRIVERS)/mtx.o		\
	$(MESS_MACHINE)/mtx.o		\

$(MESSOBJ)/mgu.a:				\
	$(MESS_DRIVERS)/irisha.o	\
	$(MESS_MACHINE)/irisha.o	\
	$(MESS_VIDEO)/irisha.o		\

$(MESSOBJ)/microkey.a:			\
	$(MESS_DRIVERS)/primo.o		\
	$(MESS_MACHINE)/primo.o		\
	$(MESS_VIDEO)/primo.o		\
	$(MESS_FORMATS)/primoptp.o	\

$(MESSOBJ)/mit.a:				\
	$(MESS_VIDEO)/crt.o			\
	$(MESS_DRIVERS)/tx0.o		\
	$(MESS_MACHINE)/tx0.o		\
	$(MESS_VIDEO)/tx0.o			\

$(MESSOBJ)/mos.a:				\
	$(MESS_DRIVERS)/kim1.o		\
	$(MESS_FORMATS)/kim1_cas.o	\

$(MESSOBJ)/motorola.a:			\
	$(MESS_VIDEO)/mekd2.o		\
	$(MESS_MACHINE)/mekd2.o		\
	$(MESS_DRIVERS)/mekd2.o		\

$(MESSOBJ)/multitch.a:			\
	$(MESS_DRIVERS)/mpf1.o		\

$(MESSOBJ)/nakajima.a:			\
	$(MESS_DRIVERS)/nakajies.o	\

$(MESSOBJ)/nascom.a:			\
	$(MESS_VIDEO)/nascom1.o		\
	$(MESS_MACHINE)/nascom1.o	\
	$(MESS_DRIVERS)/nascom1.o	\

$(MESSOBJ)/ne.a:				\
	$(MESS_DRIVERS)/z80ne.o     \
	$(MESS_VIDEO)/z80ne.o       \
	$(MESS_FORMATS)/z80ne_dsk.o \
	$(MESS_MACHINE)/z80ne.o     \

$(MESSOBJ)/nec.a:				\
	$(MAME_VIDEO)/vdc.o			\
	$(MESS_MACHINE)/pce.o		\
	$(MESS_DRIVERS)/pce.o		\
	$(MESS_DRIVERS)/pcfx.o		\
	$(MESS_DRIVERS)/pc6001.o	\
	$(MESS_DRIVERS)/pc8401a.o	\
	$(MESS_VIDEO)/pc8401a.o		\
	$(MESS_DRIVERS)/pc8801.o	\
	$(MESS_MACHINE)/pc8801.o	\
	$(MESS_VIDEO)/pc8801.o		\
	$(MESS_DRIVERS)/pc98.o		\

$(MESSOBJ)/netronic.a:			\
	$(MESS_DRIVERS)/elf.o		\
	$(MESS_DRIVERS)/exp85.o		\

$(MESSOBJ)/nintendo.a:			\
	$(MESS_AUDIO)/gb.o			\
	$(MESS_VIDEO)/gb.o			\
	$(MESS_MACHINE)/gb.o		\
	$(MESS_DRIVERS)/gb.o		\
	$(MESS_MACHINE)/nes_mmc.o	\
	$(MAME_VIDEO)/ppu2c0x.o		\
	$(MESS_VIDEO)/nes.o			\
	$(MESS_MACHINE)/nes.o		\
	$(MESS_DRIVERS)/nes.o		\
	$(MAME_AUDIO)/snes.o		\
	$(MAME_MACHINE)/snes.o		\
	$(MAME_VIDEO)/snes.o		\
	$(MESS_MACHINE)/snescart.o	\
	$(MESS_DRIVERS)/snes.o	 	\
	$(MESS_DRIVERS)/n64.o		\
	$(MAME_MACHINE)/n64.o		\
	$(MAME_VIDEO)/n64.o			\
	$(MESS_MACHINE)/pokemini.o	\
	$(MESS_DRIVERS)/pokemini.o	\
	$(MESS_DRIVERS)/vboy.o		\
	$(MESS_DRIVERS)/gba.o		\
	$(MESS_VIDEO)/gba.o		\

$(MESSOBJ)/nokia.a:				\
	$(MESS_DRIVERS)/mikromik.o	\

$(MESSOBJ)/novag.a:				\
	$(MESS_DRIVERS)/mk1.o		\
	$(MESS_DRIVERS)/mk2.o		\
	$(MESS_VIDEO)/ssystem3.o	\
	$(MESS_DRIVERS)/ssystem3.o	\

$(MESSOBJ)/olivetti.a:			\
	$(MESS_DRIVERS)/m20.o		\

$(MESSOBJ)/orion.a:				\
	$(MESS_DRIVERS)/orion.o		\
	$(MESS_MACHINE)/orion.o		\
	$(MESS_VIDEO)/orion.o		\

$(MESSOBJ)/osborne.a:			\
	$(MESS_DRIVERS)/osborne1.o	\
	$(MESS_MACHINE)/osborne1.o	\

$(MESSOBJ)/osi.a:				\
	$(MESS_DRIVERS)/osi.o		\
	$(MESS_VIDEO)/osi.o			\

$(MESSOBJ)/palm.a:				\
	$(MESS_DRIVERS)/palm.o		\
	$(MESS_MACHINE)/mc68328.o	\
	$(MESS_VIDEO)/mc68328.o		\

$(MESSOBJ)/parker.a:			\
	$(MESS_DRIVERS)/stopthie.o	\

$(MESSOBJ)/pc.a:				\
	$(MESS_VIDEO)/pc_aga.o		\
	$(MESS_MACHINE)/ibmpc.o		\
	$(MESS_MACHINE)/tandy1t.o	\
	$(MESS_MACHINE)/amstr_pc.o	\
	$(MESS_MACHINE)/europc.o	\
	$(MESS_MACHINE)/pc.o		\
	$(MESS_DRIVERS)/pc.o		\
	$(MESS_VIDEO)/pc_t1t.o		\

$(MESSOBJ)/pcm.a:				\
	$(MESS_DRIVERS)/pcm.o		\

$(MESSOBJ)/pcshare.a:			\
	$(MAME_MACHINE)/pcshare.o	\
	$(MESS_MACHINE)/pc_turbo.o	\
	$(MESS_AUDIO)/sblaster.o	\
	$(MESS_MACHINE)/pc_fdc.o	\
	$(MESS_MACHINE)/pc_hdc.o	\
	$(MESS_MACHINE)/pc_joy.o	\
	$(MESS_MACHINE)/kb_keytro.o	\
	$(MESS_VIDEO)/pc_video.o	\
	$(MESS_VIDEO)/pc_mda.o		\
	$(MESS_VIDEO)/pc_cga.o		\
	$(MESS_VIDEO)/cgapal.o		\
	$(MESS_VIDEO)/pc_vga.o		\
	$(MESS_VIDEO)/crtc_ega.o	\
	$(MESS_VIDEO)/pc_ega.o		\

$(MESSOBJ)/pdp1.a:				\
	$(MESS_VIDEO)/pdp1.o		\
	$(MESS_MACHINE)/pdp1.o		\
	$(MESS_DRIVERS)/pdp1.o		\

$(MESSOBJ)/pel.a:				\
	$(MESS_DRIVERS)/galeb.o		\
	$(MESS_MACHINE)/galeb.o		\
	$(MESS_VIDEO)/galeb.o		\
	$(MESS_FORMATS)/orao_cas.o	\
	$(MESS_DRIVERS)/orao.o		\
	$(MESS_MACHINE)/orao.o		\
	$(MESS_VIDEO)/orao.o		\

$(MESSOBJ)/philips.a:			\
	$(MESS_DRIVERS)/cdi.o		\
	$(MESS_VIDEO)/p2000m.o		\
	$(MESS_DRIVERS)/p2000t.o	\
	$(MESS_MACHINE)/p2000t.o	\
	$(MESS_DRIVERS)/vg5k.o		\

$(MESSOBJ)/poly88.a:			\
	$(MESS_DRIVERS)/poly88.o	\
	$(MESS_MACHINE)/poly88.o	\
	$(MESS_VIDEO)/poly88.o		\

$(MESSOBJ)/radio.a:				\
	$(MESS_DRIVERS)/radio86.o	\
	$(MESS_MACHINE)/radio86.o	\
	$(MESS_VIDEO)/radio86.o		\
	$(MESS_DRIVERS)/apogee.o	\
	$(MESS_DRIVERS)/partner.o	\
	$(MESS_MACHINE)/partner.o	\
	$(MESS_DRIVERS)/mikrosha.o	\

$(MESSOBJ)/rca.a:				\
	$(MESS_DRIVERS)/vip.o		\
	$(MESS_DRIVERS)/studio2.o	\

$(MESSOBJ)/robotron.a:			\
	$(MESS_DRIVERS)/a5105.o		\
	$(MESS_DRIVERS)/a51xx.o		\
	$(MESS_DRIVERS)/rt1715.o	\
	$(MESS_MACHINE)/rt1715.o	\
	$(MESS_VIDEO)/rt1715.o		\
	$(MESS_DRIVERS)/z1013.o		\
	$(MESS_MACHINE)/z1013.o		\
	$(MESS_VIDEO)/z1013.o		\
	$(MESS_DRIVERS)/z9001.o		\

$(MESSOBJ)/rockwell.a:			\
	$(MESS_VIDEO)/aim65.o		\
	$(MESS_MACHINE)/aim65.o		\
	$(MESS_DRIVERS)/aim65.o		\

$(MESSOBJ)/samcoupe.a:			\
	$(MESS_FORMATS)/coupedsk.o	\
	$(MESS_VIDEO)/samcoupe.o	\
	$(MESS_DRIVERS)/samcoupe.o	\
	$(MESS_MACHINE)/samcoupe.o	\

$(MESSOBJ)/samsung.a:			\
	$(MESS_DRIVERS)/spc1000.o	\

$(MESSOBJ)/sega.a:				\
	$(MESS_DRIVERS)/genesis.o	\
	$(MESS_MACHINE)/genesis.o	\
	$(MESS_DRIVERS)/saturn.o	\
	$(MAME_MACHINE)/stvcd.o		\
	$(MAME_MACHINE)/scudsp.o	\
	$(MAME_VIDEO)/stvvdp1.o		\
	$(MAME_VIDEO)/stvvdp2.o		\
	$(MESS_VIDEO)/smsvdp.o		\
	$(MESS_MACHINE)/sms.o		\
	$(MESS_DRIVERS)/sms.o		\
	$(MAME_DRIVERS)/megadriv.o  \
	$(MESS_DRIVERS)/sg1000.o	\
	$(MESS_DRIVERS)/dc.o		\
	$(MAME_MACHINE)/dc.o 		\
	$(MAME_MACHINE)/naomibd.o	\
	$(MAME_MACHINE)/gdcrypt.o	\
	$(MAME_VIDEO)/dc.o			\

$(MESSOBJ)/sgi.a:				\
	$(MESS_MACHINE)/sgi.o		\
	$(MESS_DRIVERS)/ip20.o		\
	$(MESS_DRIVERS)/ip22.o		\
	$(MESS_VIDEO)/newport.o		\

$(MESSOBJ)/sharp.a:				\
	$(MESS_VIDEO)/mz700.o		\
	$(MESS_DRIVERS)/mz700.o		\
	$(MESS_FORMATS)/mz_cas.o	\
	$(MESS_DRIVERS)/pocketc.o	\
	$(MESS_VIDEO)/pc1401.o		\
	$(MESS_MACHINE)/pc1401.o	\
	$(MESS_VIDEO)/pc1403.o		\
	$(MESS_MACHINE)/pc1403.o	\
	$(MESS_VIDEO)/pc1350.o		\
	$(MESS_MACHINE)/pc1350.o	\
	$(MESS_VIDEO)/pc1251.o		\
	$(MESS_MACHINE)/pc1251.o	\
	$(MESS_VIDEO)/pocketc.o		\
	$(MESS_MACHINE)/mz700.o		\
	$(MESS_DRIVERS)/x68k.o		\
	$(MESS_VIDEO)/x68k.o		\
	$(MESS_FORMATS)/dim_dsk.o	\
	$(MESS_MACHINE)/x68k_hdc.o	\
	$(MESS_DRIVERS)/mz80.o		\
	$(MESS_VIDEO)/mz80.o		\
	$(MESS_MACHINE)/mz80.o		\
	$(MESS_DRIVERS)/x1.o		\

$(MESSOBJ)/sinclair.a:			\
	$(MESS_VIDEO)/border.o		\
	$(MESS_VIDEO)/spectrum.o	\
	$(MESS_VIDEO)/timex.o		\
	$(MESS_VIDEO)/zx.o			\
	$(MESS_DRIVERS)/zx.o		\
	$(MESS_MACHINE)/zx.o		\
	$(MESS_DRIVERS)/spectrum.o	\
	$(MESS_DRIVERS)/spec128.o	\
	$(MESS_DRIVERS)/timex.o		\
	$(MESS_DRIVERS)/specpls3.o	\
	$(MESS_DRIVERS)/scorpion.o	\
	$(MESS_DRIVERS)/atm.o		\
	$(MESS_DRIVERS)/pentagon.o	\
	$(MESS_MACHINE)/spectrum.o	\
	$(MESS_FORMATS)/trd_dsk.o	\
	$(MESS_MACHINE)/beta.o		\
	$(MESS_FORMATS)/zx81_p.o	\
	$(MESS_FORMATS)/tzx_cas.o	\
	$(MESS_DRIVERS)/ql.o		\
	$(MESS_VIDEO)/zx8301.o		\
	$(MESS_MACHINE)/zx8302.o	\

$(MESSOBJ)/neocd.a:				\
	$(MESS_DRIVERS)/neocd.o		\
	$(EMU_MACHINE)/neogeo.o		\
	$(EMU_VIDEO)/neogeo.o		\
	$(EMU_MACHINE)/pd4990a.o	\

$(MESSOBJ)/snk.a:				\
	$(MESS_DRIVERS)/ngp.o		\
	$(MESS_VIDEO)/k1ge.o		\
	$(MESS_DEVICES)/aescart.o	\
	$(MESS_DRIVERS)/ng_aes.o	\
	$(MAME_VIDEO)/neogeo.o		\
	$(MAME_MACHINE)/neoprot.o	\

$(MESSOBJ)/sony.a:				\
	$(MESS_DRIVERS)/psx.o		\
	$(MAME_MACHINE)/psx.o		\
	$(MAME_VIDEO)/psx.o			\
	$(MESS_DRIVERS)/pockstat.o	\

$(MESSOBJ)/sord.a:				\
	$(MESS_DRIVERS)/sord.o		\
	$(MESS_FORMATS)/sord_cas.o	\

$(MESSOBJ)/special.a:			\
	$(MESS_AUDIO)/special.o		\
	$(MESS_DRIVERS)/special.o	\
	$(MESS_MACHINE)/special.o	\
	$(MESS_VIDEO)/special.o		\

$(MESSOBJ)/svi.a:				\
	$(MESS_MACHINE)/svi318.o	\
	$(MESS_DRIVERS)/svi318.o	\
	$(MESS_FORMATS)/svi_dsk.o	\
	$(MESS_FORMATS)/svi_cas.o	\

$(MESSOBJ)/svision.a:			\
	$(MESS_DRIVERS)/svision.o	\
	$(MESS_AUDIO)/svision.o		\

$(MESSOBJ)/synertec.a:			\
	$(MESS_MACHINE)/sym1.o		\
	$(MESS_DRIVERS)/sym1.o		\

$(MESSOBJ)/tangerin.a:			\
	$(MESS_DEVICES)/mfmdisk.o	\
	$(MESS_VIDEO)/microtan.o	\
	$(MESS_MACHINE)/microtan.o	\
	$(MESS_DRIVERS)/microtan.o	\
	$(MESS_FORMATS)/oric_tap.o	\
	$(MESS_DRIVERS)/oric.o		\
	$(MESS_VIDEO)/oric.o		\
	$(MESS_MACHINE)/oric.o		\

$(MESSOBJ)/tatung.a:			\
	$(MESS_DRIVERS)/einstein.o	\

$(MESSOBJ)/teamconc.a:			\
	$(MESS_VIDEO)/comquest.o	\
	$(MESS_DRIVERS)/comquest.o	\

$(MESSOBJ)/telenova.a:			\
	$(MESS_DRIVERS)/compis.o	\
	$(MESS_MACHINE)/compis.o	\
	$(MESS_FORMATS)/cpis_dsk.o	\

$(MESSOBJ)/telercas.a:			\
	$(MESS_DRIVERS)/tmc1800.o	\
	$(MESS_VIDEO)/tmc1800.o		\
	$(MESS_DRIVERS)/tmc600.o	\
	$(MESS_VIDEO)/tmc600.o		\
	$(MESS_DRIVERS)/tmc2000e.o	\

$(MESSOBJ)/tem.a:				\
	$(MESS_DRIVERS)/tec1.o		\

$(MESSOBJ)/tesla.a:				\
	$(MESS_DRIVERS)/ondra.o		\
	$(MESS_MACHINE)/ondra.o		\
	$(MESS_VIDEO)/ondra.o		\
	$(MESS_VIDEO)/pmd85.o		\
	$(MESS_DRIVERS)/pmd85.o		\
	$(MESS_MACHINE)/pmd85.o		\
	$(MESS_FORMATS)/pmd_pmd.o	\
	$(MESS_DRIVERS)/pmi80.o		\
	$(MESS_DRIVERS)/sapi1.o		\
	$(MESS_MACHINE)/sapi1.o		\
	$(MESS_VIDEO)/sapi1.o		\

$(MESSOBJ)/thomson.a:			\
	$(MESS_DRIVERS)/thomson.o	\
	$(MESS_MACHINE)/thomson.o	\
	$(MESS_VIDEO)/thomson.o		\
	$(MESS_DEVICES)/thomflop.o	\
	$(MESS_FORMATS)/thom_dsk.o	\
	$(MESS_FORMATS)/thom_cas.o	\

$(MESSOBJ)/ti.a:				\
	$(MESS_VIDEO)/avigo.o		\
	$(MESS_DRIVERS)/avigo.o		\
	$(MESS_DRIVERS)/ti85.o		\
	$(MESS_FORMATS)/ti85_ser.o	\
	$(MESS_VIDEO)/ti85.o		\
	$(MESS_MACHINE)/ti85.o		\
	$(MESS_DRIVERS)/ti89.o		\
	$(MESS_DEVICES)/ti99cart.o	\
	$(MESS_MACHINE)/tms9901.o	\
	$(MESS_MACHINE)/tms9902.o	\
	$(MESS_MACHINE)/ti99_4x.o	\
	$(MESS_MACHINE)/990_hd.o	\
	$(MESS_MACHINE)/990_tap.o	\
	$(MESS_MACHINE)/ti990.o		\
	$(MESS_MACHINE)/994x_ser.o	\
	$(MESS_MACHINE)/99_dsk.o	\
	$(MESS_MACHINE)/99_ide.o	\
	$(MESS_MACHINE)/99_peb.o	\
	$(MESS_MACHINE)/99_hsgpl.o	\
	$(MESS_MACHINE)/99_usbsm.o	\
	$(MESS_MACHINE)/ti99pcod.o	\
	$(MESS_MACHINE)/strata.o	\
	$(MESS_MACHINE)/geneve.o	\
	$(MESS_MACHINE)/990_dk.o	\
	$(MESS_AUDIO)/spchroms.o	\
	$(MESS_DRIVERS)/ti990_4.o	\
	$(MESS_DRIVERS)/ti99_4x.o	\
	$(MESS_DRIVERS)/ti99_4p.o	\
	$(MESS_DRIVERS)/geneve.o	\
	$(MESS_DRIVERS)/tm990189.o	\
	$(MESS_DRIVERS)/ti99_8.o	\
	$(MESS_VIDEO)/911_vdt.o		\
	$(MESS_VIDEO)/733_asr.o		\
	$(MESS_DRIVERS)/ti990_10.o	\
	$(MESS_DRIVERS)/ti99_2.o	\

$(MESSOBJ)/tiger.a:				\
	$(MESS_DRIVERS)/gamecom.o	\
	$(MESS_MACHINE)/gamecom.o	\
	$(MESS_VIDEO)/gamecom.o		\

$(MESSOBJ)/tiki.a:				\
	$(MESS_DRIVERS)/tiki100.o	\

$(MESSOBJ)/tomy.a:				\
	$(MESS_DRIVERS)/tutor.o		\

$(MESSOBJ)/trs.a:				\
	$(MESS_MACHINE)/6883sam.o	\
	$(MESS_MACHINE)/ds1315.o	\
	$(MESS_MACHINE)/coco.o		\
	$(MESS_VIDEO)/coco.o		\
	$(MESS_DRIVERS)/coco.o		\
	$(MESS_VIDEO)/coco3.o		\
	$(MESS_FORMATS)/cocopak.o	\
	$(MESS_DEVICES)/coco_vhd.o	\
	$(MESS_DEVICES)/cococart.o	\
	$(MESS_DEVICES)/coco_fdc.o	\
	$(MESS_DEVICES)/coco_pak.o	\
	$(MESS_DEVICES)/coco_232.o	\
	$(MESS_DEVICES)/orch90.o	\
	$(MESS_MACHINE)/mc10.o		\
	$(MESS_DRIVERS)/mc10.o		\
	$(MESS_MACHINE)/trs80.o		\
	$(MESS_VIDEO)/trs80.o		\
	$(MESS_FORMATS)/trs_dsk.o	\
	$(MESS_FORMATS)/trs_cas.o	\
	$(MESS_DRIVERS)/trs80.o		\

$(MESSOBJ)/unisys.a:			\
	$(MESS_DRIVERS)/univac.o	\

$(MESSOBJ)/veb.a:				\
	$(MESS_DRIVERS)/chessmst.o	\
	$(MESS_VIDEO)/kc.o			\
	$(MESS_DRIVERS)/kc.o		\
	$(MESS_MACHINE)/kc.o		\
	$(MESS_DRIVERS)/lc80.o		\
	$(MESS_DRIVERS)/mc80.o		\
	$(MESS_VIDEO)/mc80.o		\
	$(MESS_MACHINE)/mc80.o		\
	$(MESS_DRIVERS)/poly880.o	\
	$(MESS_DRIVERS)/sc1.o		\
	$(MESS_DRIVERS)/sc2.o		\

$(MESSOBJ)/visual.a:			\
	$(MESS_DRIVERS)/v1050.o		\
	$(MESS_VIDEO)/v1050.o		\

$(MESSOBJ)/votrax.a:			\
	$(MESS_DRIVERS)/votrpss.o	\
	$(MESS_DRIVERS)/votrtnt.o	\

$(MESSOBJ)/vtech.a:				\
	$(MESS_VIDEO)/vtech1.o		\
	$(MESS_MACHINE)/vtech1.o	\
	$(MESS_DRIVERS)/vtech1.o	\
	$(MESS_VIDEO)/vtech2.o		\
	$(MESS_MACHINE)/vtech2.o	\
	$(MESS_DRIVERS)/vtech2.o	\
	$(MESS_FORMATS)/vt_cas.o	\
	$(MESS_FORMATS)/vt_dsk.o	\
	$(MESS_DRIVERS)/crvision.o	\
	$(MESS_AUDIO)/socrates.o	\
	$(MESS_DRIVERS)/socrates.o	\

$(MESSOBJ)/xerox.a:				\
	$(MESS_DRIVERS)/xerox820.o	\

$(MESSOBJ)/zvt.a:				\
	$(MESS_DRIVERS)/pp01.o		\
	$(MESS_MACHINE)/pp01.o		\
	$(MESS_VIDEO)/pp01.o		\

$(MESSOBJ)/skeleton.a:			\
	$(MESS_DRIVERS)/4004clk.o	\
	$(MESS_DRIVERS)/act.o		\
	$(MESS_DRIVERS)/amico2k.o	\
	$(MESS_DRIVERS)/bob85.o		\
	$(MESS_DRIVERS)/busicom.o	\
	$(MESS_VIDEO)/busicom.o		\
	$(MESS_DRIVERS)/d6809.o		\
	$(MESS_DRIVERS)/elwro800.o	\
	$(MESS_DRIVERS)/fk1.o		\
	$(MESS_DRIVERS)/interact.o	\
	$(MESS_DRIVERS)/hec2hrp.o	\
	$(MESS_DRIVERS)/sys2900.o	\
	$(MESS_DRIVERS)/xor100.o	\
	$(MESS_DRIVERS)/iq151.o		\
	$(MESS_DRIVERS)/pyl601.o	\
	$(MESS_DRIVERS)/nanos.o		\
	$(MESS_DRIVERS)/beehive.o	\
	$(MESS_DRIVERS)/unior.o		\
	$(MESS_DRIVERS)/tvc.o		\
	$(MESS_DRIVERS)/mmd1.o		\
	$(MESS_DRIVERS)/beta.o		\
	$(MESS_DRIVERS)/ptcsol.o	\

#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(MESSOBJ)/mess.o:	$(MESS_LAYOUT)/lcd.lh

$(MESS_DRIVERS)/acrnsys1.o:	$(MESS_LAYOUT)/acrnsys1.lh
$(MESS_DRIVERS)/aim65.o:	$(MESS_LAYOUT)/aim65.lh
$(MESS_DRIVERS)/amico2k.o:	$(MESS_LAYOUT)/amico2k.lh
$(MESS_DRIVERS)/bob85.o:	$(MESS_LAYOUT)/bob85.lh
$(MESS_DRIVERS)/coco.o:		$(MESS_LAYOUT)/coco3.lh
$(MESS_DRIVERS)/c80.o:		$(MESS_LAYOUT)/c80.lh
$(MESS_DRIVERS)/dectalk.o:	$(MESS_LAYOUT)/dectalk.lh
$(MESS_DRIVERS)/elf.o:		$(MESS_LAYOUT)/elf2.lh
$(MESS_DRIVERS)/ex800.o:	$(MESS_LAYOUT)/ex800.lh
$(MESS_DRIVERS)/glasgow.o:	$(MESS_LAYOUT)/glasgow.lh
$(MESS_DRIVERS)/kim1.o:		$(MESS_LAYOUT)/kim1.lh
$(MESS_DRIVERS)/junior.o:	$(MESS_LAYOUT)/junior.lh
$(MESS_DRIVERS)/lc80.o:		$(MESS_LAYOUT)/lc80.lh
$(MESS_DRIVERS)/lx800.o:	$(MESS_LAYOUT)/lx800.lh
$(MESS_DRIVERS)/mephisto.o:	$(MESS_LAYOUT)/mephisto.lh
$(MESS_DRIVERS)/mk1.o:		$(MESS_LAYOUT)/mk1.lh
$(MESS_DRIVERS)/mk2.o:		$(MESS_LAYOUT)/mk2.lh
$(MESS_DRIVERS)/mpf1.o:		$(MESS_LAYOUT)/mpf1.lh
$(MESS_DRIVERS)/mmd1.o:		$(MESS_LAYOUT)/mmd1.lh \
							$(MESS_LAYOUT)/mmd2.lh
$(MESS_DRIVERS)/poly880.o:	$(MESS_LAYOUT)/poly880.lh
$(MESS_VIDEO)/pc8401a.o:	$(MESS_LAYOUT)/pc8500.lh
$(MESS_DRIVERS)/px4.o:		$(MESS_LAYOUT)/px4.lh
$(MESS_DRIVERS)/stopthie.o:	$(MESS_LAYOUT)/stopthie.lh
$(MESS_DRIVERS)/super80.o:	$(MESS_LAYOUT)/super80.lh
$(MESS_DRIVERS)/svision.o:	$(MESS_LAYOUT)/svision.lh
$(MESS_DRIVERS)/sym1.o:		$(MESS_LAYOUT)/sym1.lh
$(MESS_DRIVERS)/tec1.o:		$(MESS_LAYOUT)/tec1.lh
$(MESS_DRIVERS)/ut88.o:		$(MESS_LAYOUT)/ut88mini.lh
$(MESS_DRIVERS)/vboy.o:		$(MESS_LAYOUT)/vboy.lh
$(MESS_DRIVERS)/vcs80.o:	$(MESS_LAYOUT)/vcs80.lh
$(MESS_DRIVERS)/votrpss.o:	$(MESS_LAYOUT)/votrpss.lh
$(MESS_DRIVERS)/votrtnt.o:	$(MESS_LAYOUT)/votrtnt.lh
$(MESS_DRIVERS)/vt100.o:	$(MESS_LAYOUT)/vt100.lh
$(MESS_DRIVERS)/x68k.o:		$(MESS_LAYOUT)/x68000.lh
$(MESS_DRIVERS)/z80ne.o:	$(MESS_LAYOUT)/z80ne.lh   \
							$(MESS_LAYOUT)/z80net.lh  \
							$(MESS_LAYOUT)/z80netb.lh \
							$(MESS_LAYOUT)/z80netf.lh

$(MAME_MACHINE)/snes.o: 	$(MAMESRC)/machine/snesdsp1.c \
				$(MAMESRC)/machine/snesdsp2.c \
				$(MAMESRC)/machine/snesobc1.c \
				$(MAMESRC)/machine/snesrtc.c \
				$(MAMESRC)/machine/snessdd1.c

#-------------------------------------------------
# MESS-specific tools
#-------------------------------------------------

ifdef BUILD_IMGTOOL
include $(MESSSRC)/tools/imgtool/imgtool.mak
TOOLS += $(IMGTOOL)
endif

ifdef BUILD_CASTOOL
include $(MESSSRC)/tools/castool/castool.mak
TOOLS += $(CASTOOL)
endif

ifdef BUILD_MESSTEST
include $(MESSSRC)/tools/messtest/messtest.mak
TOOLS += $(MESSTEST)
endif

ifdef BUILD_DAT2HTML
include $(MESSSRC)/tools/dat2html/dat2html.mak
TOOLS += $(DAT2HTML)
endif

# include OS-specific MESS stuff
ifeq ($(OSD),windows)
include $(MESSSRC)/tools/messdocs/messdocs.mak

ifdef BUILD_WIMGTOOL
include $(MESSSRC)/tools/imgtool/windows/wimgtool.mak
TOOLS += $(WIMGTOOL)
endif
endif



#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak
