/**************************************************************************************

    NEC PC-98 computer series

	preliminary driver by Angelo Salese

	Reminder: tests A20 line feature, afaik it's NOT supported by the i386 core (along with protected mode / MMU)

	TODO:
	- remove the A20 line hack
	- Understand what's going on at bp f8dfa (memo: bp f8dfa, do eip=dff)

========================================================================================

    This series features a huge number of models released between 1982 and 1997. They
    were not IBM PC-compatible, but they had similar hardware (and software: in the
    1990s, they run MS Windows as OS)

    Models:

                      |  CPU                          |   RAM    |            Drives                                     | CBus| Release |
    PC-9801           |  8086 @ 5                     |  128 KB  | -                                                     |  6  | 1982/10 |
    PC-9801F1         |  8086-2 @ 5/8                 |  128 KB  | 5"2DDx1                                               |  4  | 1983/10 |
    PC-9801F2         |  8086-2 @ 5/8                 |  128 KB  | 5"2DDx2                                               |  4  | 1983/10 |
    PC-9801E          |  8086-2 @ 5/8                 |  128 KB  | -                                                     |  6  | 1983/11 |
    PC-9801F3         |  8086-2 @ 5/8                 |  256 KB  | 5"2DDx1, 10M SASI HDD                                 |  2  | 1984/10 |
    PC-9801M2         |  8086-2 @ 5/8                 |  256 KB  | 5"2HDx2                                               |  4  | 1984/11 |
    PC-9801M3         |  8086-2 @ 5/8                 |  256 KB  | 5"2HDx1, 20M SASI HDD                                 |  3  | 1985/02 |
    PC-9801U2         |  V30 @ 8                      |  128 KB  | 3.5"2HDx2                                             |  2  | 1985/05 |
    PC-98XA1          |  80286 @ 8                    |  512 KB  | -                                                     |  6  | 1985/05 |
    PC-98XA2          |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx2                                           |  6  | 1985/05 |
    PC-98XA3          |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  6  | 1985/05 |
    PC-9801VF2        |  V30 @ 8                      |  384 KB  | 5"2DDx2                                               |  4  | 1985/07 |
    PC-9801VM0        |  V30 @ 8/10                   |  384 KB  | -                                                     |  4  | 1985/07 |
    PC-9801VM2        |  V30 @ 8/10                   |  384 KB  | 5"2DD/2HDx2                                           |  4  | 1985/07 |
    PC-9801VM4        |  V30 @ 8/10                   |  384 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1985/10 |
    PC-98XA11         |  80286 @ 8                    |  512 KB  | -                                                     |  6  | 1986/05 |
    PC-98XA21         |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx2                                           |  6  | 1986/05 |
    PC-98XA31         |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  6  | 1986/05 |
    PC-9801UV2        |  V30 @ 8/10                   |  384 KB  | 3.5"2DD/2HDx2                                         |  2  | 1986/05 |
    PC-98LT1          |  V50 @ 8                      |  384 KB  | 3.5"2DD/2HDx1                                         |  0  | 1986/11 |
    PC-98LT2          |  V50 @ 8                      |  384 KB  | 3.5"2DD/2HDx1                                         |  0  | 1986/11 |
    PC-9801VM21       |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1986/11 |
    PC-9801VX0        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | -                                                     |  4  | 1986/11 |
    PC-9801VX2        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1986/11 |
    PC-9801VX4        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1986/11 |
    PC-9801VX4/WN     |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1986/11 |
    PC-98XL1          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | -                                                     |  4  | 1986/12 |
    PC-98XL2          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | 5"2DD/2HDx2                                           |  4  | 1986/12 |
    PC-98XL4          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  4  | 1986/12 |
    PC-9801VX01       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | -                                                     |  4  | 1987/06 |
    PC-9801VX21       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1987/06 |
    PC-9801VX41       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1987/06 |
    PC-9801UV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1987/06 |
    PC-98XL^2         |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1987/10 |
    PC-98LT11         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1987/10 |
    PC-98LT21         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1987/10 |
    PC-9801UX21       |  80286-10 @ 10 & V30 @ 8      |  640 KB  | 3.5"2DD/2HDx2                                         |  3  | 1987/10 |
    PC-9801UX41       |  80286-10 @ 10 & V30 @ 8      |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  3  | 1987/10 |
    PC-9801LV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1988/03 |
    PC-9801CV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1988/03 |
    PC-9801UV11       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1988/03 |
    PC-9801RA2        |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1988/07 |
    PC-9801RA5        |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1988/07 |
    PC-9801RX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1988/07 |
    PC-9801RX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1988/07 |
    PC-98LT22         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1988/11 |
    PC-98LS2          |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  0  | 1988/11 |
    PC-98LS5          |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  0  | 1988/11 |
    PC-9801VM11       |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1988/11 |
    PC-9801LV22       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1989/01 |
    PC-98RL2          |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1989/02 |
    PC-98RL5          |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/02 |
    PC-9801EX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  3  | 1989/04 |
    PC-9801EX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  3  | 1989/04 |
    PC-9801ES2        |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1989/04 |
    PC-9801ES5        |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  3  | 1989/04 |
    PC-9801LX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1989/04 |
    PC-9801LX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  0  | 1989/04 |
    PC-9801LX5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  0  | 1989/06 |
    PC-98DO           |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  1  | 1989/06 |
    PC-9801LX5C       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  0  | 1989/06 |
    PC-9801RX21       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1989/10 |
    PC-9801RX51       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/10 |
    PC-9801RA21       |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1989/11 |
    PC-9801RA51       |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/11 |
    PC-9801RS21       |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1989/11 |
    PC-9801RS51       |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/11 |
    PC-9801N          |  V30 @ 10                     |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1989/11 |
    PC-9801TW2        |  i386SX-20 @ 20 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1990/02 |
    PC-9801TW5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/02 |
    PC-9801TS5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/06 |
    PC-9801NS         |  i386SX-12 @ 12               |  1.6 MB  | 3.5"2DD/2HDx1                                         |  0  | 1990/06 |
    PC-9801TF5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/07 |
    PC-9801NS-20      |  i386SX-12 @ 12               |  1.6 MB  | 3.5"2DD/2HDx1, 20M SASI HDD                           |  0  | 1990/09 |
    PC-98RL21         |  i386DX-20 @ 20 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1990/09 |
    PC-98RL51         |  i386DX-20 @ 20 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx1, 40M SASI HDD                             |  4  | 1990/09 |
    PC-98DO+          |  V33A @ 8/16                  |  640 KB  | 5"2DD/2HDx2                                           |  1  | 1990/10 |
    PC-9801DX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1990/11 |
    PC-9801DX/U2      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1990/11 |
    PC-9801DX5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1990/11 |
    PC-9801DX/U5      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1990/11 |
    PC-9801NV         |  V30HL @ 8/16                 |  1.6 MB  | 3.5"2DD/2HDx1                                         |  0  | 1990/11 |
    PC-9801DS2        |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1991/01 |
    PC-9801DS/U2      |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1991/01 |
    PC-9801DS5        |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1991/01 |
    PC-9801DS/U5      |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1991/01 |
    PC-9801DA2        |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1991/01 |
    PC-9801DA/U2      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1991/01 |
    PC-9801DA5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1991/01 |
    PC-9801DA/U5      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1991/01 |
    PC-9801DA7        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1991/02 |
    PC-9801DA/U7      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1991/02 |
    PC-9801UF         |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1991/02 |
    PC-9801UR         |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  2  | 1991/02 |
    PC-9801UR/20      |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 20M SASI HDD          |  2  | 1991/02 |
    PC-9801NS/E       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1991/06 |
    PC-9801NS/E20     |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 20M SASI HDD          |  0  | 1991/06 |
    PC-9801NS/E40     |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1991/06 |
    PC-9801TW7        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  2  | 1991/07 |
    PC-9801TF51       |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/07 |
    PC-9801TF71       |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  2  | 1991/07 |
    PC-9801NC         |  i386SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1991/10 |
    PC-9801NC40       |  i386SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1991/10 |
    PC-9801CS2        |  i386SX-16 @ 16               |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1991/10 |
    PC-9801CS5        |  i386SX-16 @ 16               |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/10 |
    PC-9801CS5/W      |  i386SX-16 @ 16               |  3.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/11 |
    PC-98GS1          |  i386SX-20 @ 20 & V30 @ 8     |  2.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  3  | 1991/11 |
    PC-98GS2          |  i386SX-20 @ 20 & V30 @ 8     |  2.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD, 1xCD-ROM                 |  3  | 1991/11 |
    PC-9801FA2        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/01 |
    PC-9801FA/U2      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/01 |
    PC-9801FA5        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/01 |
    PC-9801FA/U5      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/01 |
    PC-9801FA7        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1992/01 |
    PC-9801FA/U7      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1992/01 |
    PC-9801FS2        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/05 |
    PC-9801FS/U2      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/05 |
    PC-9801FS5        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/05 |
    PC-9801FS/U5      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/05 |
    PC-9801FS7        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1992/01 |
    PC-9801FS/U7      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1992/01 |
    PC-9801NS/T       |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/01 |
    PC-9801NS/T40     |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/01 |
    PC-9801NS/T80     |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 80M SASI HDD          |  0  | 1992/01 |
    PC-9801NL         |  V30H @ 8/16                  |  640 KB  | 1.25 MB RAM Disk                                      |  0  | 1992/01 |
    PC-9801FX2        |  i386SX-12 @ 10/12            |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/05 |
    PC-9801FX/U2      |  i386SX-12 @ 10/12            |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/05 |
    PC-9801FX5        |  i386SX-12 @ 10/12            |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/05 |
    PC-9801FX/U5      |  i386SX-12 @ 10/12            |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/05 |
    PC-9801US         |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  2  | 1992/07 |
    PC-9801US40       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1992/07 |
    PC-9801US80       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 80M SASI HDD                           |  2  | 1992/07 |
    PC-9801NS/L       |  i386SX-20 @ 10/20            |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/07 |
    PC-9801NS/L40     |  i386SX-20 @ 10/20            |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/07 |
    PC-9801NA         |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/11 |
    PC-9801NA40       |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/11 |
    PC-9801NA120      |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 120M SASI HDD         |  0  | 1992/11 |
    PC-9801NA/C       |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/11 |
    PC-9801NA40/C     |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/11 |
    PC-9801NA120/C    |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 120M SASI HDD         |  0  | 1992/11 |
    PC-9801NS/R       |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk                |  0  | 1993/01 |
    PC-9801NS/R40     |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk, 40M SASI HDD  |  0  | 1993/01 |
    PC-9801NS/R120    |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk, 120M SASI HDD |  0  | 1993/01 |
    PC-9801BA/U2      |  i486DX2-40 @ 40              |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/01 |
    PC-9801BA/U6      |  i486DX2-40 @ 40              |  3.6 MB  | 3.5"2DD/2HDx1, 40M SASI HDD                           |  3  | 1993/01 |
    PC-9801BA/M2      |  i486DX2-40 @ 40              |  1.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/01 |
    PC-9801BX/U2      |  i486SX-20 @ 20               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/01 |
    PC-9801BX/U6      |  i486SX-20 @ 20               |  3.6 MB  | 3.5"2DD/2HDx1, 40M SASI HDD                           |  3  | 1993/01 |
    PC-9801BX/M2      |  i486SX-20 @ 20               |  1.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/01 |
    PC-9801NX/C       |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1993/07 |
    PC-9801NX/C120    |  i486SX(J) @ 20               |  3.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1993/07 |
    PC-9801P40/D      |  i486SX(J) @ 20               |  5.6 MB  | 40MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801P80/W      |  i486SX(J) @ 20               |  7.6 MB  | 80MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801P80/P      |  i486SX(J) @ 20               |  7.6 MB  | 80MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801BA2/U2     |  i486DX2-66 @ 66              |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BA2/U7     |  i486DX2-66 @ 66              |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BA2/M2     |  i486DX2-66 @ 66              |  3.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BS2/U2     |  i486SX-33 @ 33               |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BS2/U7     |  i486SX-33 @ 33               |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BS2/M2     |  i486SX-33 @ 33               |  3.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BX2/U2     |  i486SX-25 @ 25               |  1.8 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BX2/U7     |  i486SX-25 @ 25               |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BX2/M2     |  i486SX-25 @ 25               |  1.8 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BA3/U2     |  i486DX-66 @ 66               |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1995/01 |
    PC-9801BA3/U2/W   |  i486DX-66 @ 66               |  7.6 MB  | 3.5"2DD/2HDx2, 210MB IDE HDD                          |  3  | 1995/01 |
    PC-9801BX3/U2     |  i486SX-33 @ 33               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1995/01 |
    PC-9801BX3/U2/W   |  i486SX-33 @ 33               |  5.6 MB  | 3.5"2DD/2HDx2, 210MB IDE HDD                          |  3  | 1995/01 |
    PC-9801BX4/U2     |  AMD/i 486DX2-66 @ 66         |  2 MB    | 3.5"2DD/2HDx2                                         |  3  | 1995/07 |
    PC-9801BX4/U2/C   |  AMD/i 486DX2-66 @ 66         |  2 MB    | 3.5"2DD/2HDx2, 2xCD-ROM                               |  3  | 1995/07 |
    PC-9801BX4/U2-P   |  Pentium ODP @ 66             |  2 MB    | 3.5"2DD/2HDx2                                         |  3  | 1995/09 |
    PC-9801BX4/U2/C-P |  Pentium ODP @ 66             |  2 MB    | 3.5"2DD/2HDx2, 2xCD-ROM                               |  3  | 1995/09 |

    For more info (e.g. optional hardware), see http://www.geocities.jp/retro_zzz/machines/nec/9801/mdl98cpu.html


    PC-9821 Series

    PC-9821 (1992) - aka 98MULTi, desktop computer, 386 based
    PC-9821A series (1993->1994) - aka 98MATE A, desktop computers, 486 based
    PC-9821B series (1993) - aka 98MATE B, desktop computers, 486 based
    PC-9821C series (1993->1996) - aka 98MULTi CanBe, desktop & tower computers, various CPU
    PC-9821Es (1994) - aka 98FINE, desktop computer with integrated LCD, successor of the PC-98T
    PC-9821X series (1994->1995) - aka 98MATE X, desktop computers, Pentium based
    PC-9821V series (1995) - aka 98MATE Valuestar, desktop computers, Pentium based
    PC-9821S series (1995->2996) - aka 98Pro, tower computers, PentiumPro based
    PC-9821R series (1996->2000) - aka 98MATE R, desktop & tower & server computers, various CPU
    PC-9821C200 (1997) - aka CEREB, desktop computer, Pentium MMX based
    PC-9821 Ne/Ns/Np/Nm (1993->1995) - aka 98NOTE, laptops, 486 based
    PC-9821 Na/Nb/Nw (1995->1997) - aka 98NOTE Lavie, laptops, Pentium based
    PC-9821 Lt/Ld (1995) - aka 98NOTE Light, laptops, 486 based
    PC-9821 La/Ls (1995->1997) - aka 98NOTE Aile, laptops, Pentium based


    Note - It is not clear the origin of the dumps we include here: they could be from later
    models

***************************************************************************************/

#include "driver.h"
#include "cpu/i386/i386.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"
#include "machine/pic8259.h"

static UINT32 *pc9801_vram;
static UINT32 *gfx_bitmap_ram;
static UINT32 *work_ram_banked;
static UINT8 ems_bank;
static UINT8 wram_bank;


static VIDEO_START( pc9801 )
{
	gfx_bitmap_ram = auto_alloc_array(machine, UINT32, 0x18000*2);
	work_ram_banked = auto_alloc_array(machine, UINT32, 0x2000*0xfe); //shouldn't be here
}

/*
attrib ram is:
bit 0: secret / hidden (?)
bit 1: blink
bit 2: reverse video
bit 3: underline
bit 4: vertical line / simple gfx (toggled by flip-flop bit) (?)
bit 5-7: BRG colors
*/

static VIDEO_UPDATE( pc9801 )
{
//	const gfx_element *gfx = screen->machine->gfx[0];
	int y,x;
	int yi,xi;
	UINT8 *gfx_data = memory_region(screen->machine, "gfx1");

	for (y=0;y<25;y++)
	{
		for (x=0;x<40;x++)
		{
			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int tile,attr,pen[3],pen_mask,color;

					tile = (pc9801_vram[(x+y*40)] & 0x00ff0000) >> 16;
					attr = (pc9801_vram[((x+y*40)+0x2000/4)] & 0x00ff0000) >> 16;

					pen_mask = (attr & 0xe0)>>5;

					pen[0] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 1)>>0;
					pen[1] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 2)>>1;
					pen[2] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 4)>>2;

					color = pen[2]<<2|pen[1]<<1|pen[0]<<0;

					if(((x*2+1)*8+xi)<video_screen_get_visible_area(screen->machine->primary_screen)->max_x && (y*8+yi)<video_screen_get_visible_area(screen->machine->primary_screen)->max_y)
						*BITMAP_ADDR16(bitmap, y*8+yi, (x*2+1)*8+xi) = screen->machine->pens[color];

					tile = (pc9801_vram[(x+y*40)] & 0x000000ff) >> 0;
					attr = (pc9801_vram[((x+y*40)+0x2000/4)] & 0x000000ff) >> 0;

					pen_mask = (attr & 0xe0)>>5;

					pen[0] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 1)>>0;
					pen[1] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 2)>>1;
					pen[2] = gfx_data[(tile*8)+yi]>>(7-xi) & (pen_mask & 4)>>2;

					color = pen[2]<<2|pen[1]<<1|pen[0]<<0;

					if(((x*2+0)*8+xi)<video_screen_get_visible_area(screen->machine->primary_screen)->max_x && (y*8+yi)<video_screen_get_visible_area(screen->machine->primary_screen)->max_y)
						*BITMAP_ADDR16(bitmap, y*8+yi, (x*2+0)*8+xi) = screen->machine->pens[color];
				}
			}
		}
	}

	return 0;
}

static READ8_HANDLER( sys_port_r )
{
	if(offset & 1)
		return ppi8255_r(devtag_get_device(space->machine, "ppi8255_0"), (offset & 6) >> 1);

	logerror("RS-232c R access %02x\n",offset >> 1);
	return 0xff;
}


static WRITE8_HANDLER( sys_port_w )
{
	if(offset & 1)
		ppi8255_w(devtag_get_device(space->machine, "ppi8255_0"), (offset & 6) >> 1, data);
	else
		logerror("RS-232c W access %02x %02x\n",offset >> 1,data);
}

static READ8_HANDLER( sio_port_r )
{
	if(!(offset & 1))
		return ppi8255_r(devtag_get_device(space->machine, "ppi8255_1"), (offset & 6) >> 1);

	logerror("keyboard R access %02x\n",offset >> 1);
	return 0xff;
}


static WRITE8_HANDLER( sio_port_w )
{
	if(!(offset & 1))
		ppi8255_w(devtag_get_device(space->machine, "ppi8255_1"), (offset & 6) >> 1, data);
	else
		logerror("keyboard W access %02x %02x\n",offset >> 1,data);
}

/*
#define UPD7220_SR_DATA_READY			0x01
#define UPD7220_SR_FIFO_FULL			0x02
#define UPD7220_SR_FIFO_EMPTY			0x04
#define UPD7220_SR_DRAWING_IN_PROGRESS	0x08
#define UPD7220_SR_DMA_EXECUTE			0x10
#define UPD7220_SR_VSYNC_ACTIVE			0x20
#define UPD7220_SR_HBLANK_ACTIVE		0x40
#define UPD7220_SR_LIGHT_PEN_DETECT		0x80
*/
static READ8_HANDLER( crtc_status_r )
{
	UINT8 vsync = input_port_read(space->machine, "VBLANK") & 0x20;

	return 0x04 | vsync;//mame_rand(space->machine);
}

static READ8_HANDLER( crtc_fifo_r )
{
	return 0x00;//mame_rand(space->machine);
}

static WRITE8_HANDLER( crtc_param_w )
{
}

static WRITE8_HANDLER( crtc_cmd_w )
{
}


static READ8_HANDLER( port_60_r )
{
	if(offset == 0)
		return crtc_status_r(space, 0);
	if(offset == 2)
		return crtc_fifo_r(space,0);

	return 0xff;
}

static WRITE8_HANDLER( port_60_w )
{
	if(offset == 0)
		crtc_param_w(space, 0,data);
	if(offset == 2)
		crtc_cmd_w(space,0,data);
}

static READ8_HANDLER( port_a0_r )
{
	if(offset == 0)
		return crtc_status_r(space, 0);
	if(offset == 2)
		return crtc_fifo_r(space,0);

	return 0xff;
}

static WRITE8_HANDLER( port_a0_w )
{
	if(offset == 0)
		crtc_param_w(space, 0,data);
	if(offset == 2)
		crtc_cmd_w(space,0,data);
}

static UINT8 rom_bank;

static WRITE8_HANDLER( ems_sel_w )
{
//	printf("%02x %02x\n",offset,data);

	if(offset == 1)
	{
		UINT8 *ROM = memory_region(space->machine, "cpudata");

		if(data == 0x00 || data == 0x10)
		{
			rom_bank = 1;
			memory_set_bankptr(space->machine, 1, &ROM[0x20000]);
		}

		if(data == 0x02 || data == 0x12)
		{
			rom_bank = 0;
			memory_set_bankptr(space->machine, 1, &ROM[0x00000]);
		}
	}

	if(offset == 3)
	{
		if(data == 0x22)
			ems_bank = 1;
		if(data == 0x20)
			ems_bank = 0;
		//printf("%02x %02x\n",offset,data);
	}
}

static WRITE8_HANDLER( rom_bank_w )
{
	UINT8 *ROM = memory_region(space->machine, "cpudata");

//	printf("%08x %04x %08x\n",ROM[offset+(rom_bank*0x20000/4)],offset+(rom_bank*0x20000/4),data);

	/* NOP any attempt to write on ROM (TODO: why it's overwriting here anyway? Doesn't make sense...) */
	if((rom_bank == 0 && offset < 0x8000) || (rom_bank == 1 && offset < 0x18000))
		ROM[offset+(rom_bank*0x20000)] = data;
}

static READ8_HANDLER( port_70_r )
{
	if(offset & 1)
	{
		logerror("pit port $70 R access %02x\n",offset >> 1);
		/* FIXME */
		return pit8253_r(devtag_get_device(space->machine, "pit8253"), (offset & 6) >> 1);
	}

	logerror("crtc port $70 R access %02x\n",offset >> 1);
	return 0xff;
}

static WRITE8_HANDLER( port_70_w )
{
	if(offset & 1)
	{
		logerror("pit port $70 W access %02x %02x\n",offset >> 1,data);
		pit8253_w(devtag_get_device(space->machine, "pit8253"), (offset & 6) >> 1, data);
	}
	else
		logerror("crtc port $70 W access %02x %02x\n",offset >> 1,data);
}

static READ8_HANDLER( port_00_r )
{
	if(!(offset & 1))
	{
		logerror("pic8259 port $00 R access %02x\n",offset >> 1);
		if((offset>>1) >= 4)
			return pic8259_r(devtag_get_device(space->machine, "pic8259_slave"), (offset & 2) >> 1);
		else
			return pic8259_r(devtag_get_device(space->machine, "pic8259_master"), (offset & 2) >> 1);
	}

	//logerror("DMA port $00 R access %02x\n",offset >> 1);
	return dma8237_r(devtag_get_device(space->machine, "dma8237_1"), (offset & 0x1e) >> 1);
}

static WRITE8_HANDLER( port_00_w )
{
	if(!(offset & 1))
	{
		logerror("pic8259 port $00 W access %02x %02x\n",offset >> 1,data);
		if((offset>>1) >= 4)
			pic8259_w(devtag_get_device(space->machine, "pic8259_slave"), (offset & 2) >> 1, data);
		else
			pic8259_w(devtag_get_device(space->machine, "pic8259_master"), (offset & 2) >> 1, data);
	}
	else
	{
		//logerror("DMA port $00 W access %02x %02x\n",offset >> 1,data);
		dma8237_w(devtag_get_device(space->machine, "dma8237_1"), (offset & 0x1e) >> 1, data);
	}
}

static UINT8 gate_a20;

static READ8_HANDLER( port_f0_r )
{
	if(offset == 0x02)
		return (gate_a20 ^ 1) | 0x2e;
	else if(offset == 0x06)
		return (gate_a20 ^ 1) | 0x5e;

	return 0x00;
}

static WRITE8_HANDLER( port_f0_w )
{
	if(offset == 0x00)
	{
		//cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
	}

	if(offset == 0x02)
	{
		gate_a20 = 1;
		cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_A20, gate_a20);
	}
	if(offset == 0x06)
	{
		if(data == 0x02)
		{
			gate_a20 = 1;
			cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_A20, gate_a20);
		}
		else if(data == 0x03)
		{
			gate_a20 = 0;
			cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_A20, gate_a20);
		}
	}
}

static UINT8 wram_latch_reg;

static READ8_HANDLER( wram_sel_r )
{
	return (wram_bank << 1) | wram_latch_reg;
}

static WRITE8_HANDLER( wram_sel_w )
{
	wram_bank = (data & 0xfe) >> 1;
	wram_latch_reg = data & 1; //<- correct?
	//printf("%02x\n",wram_bank);
}

static READ8_HANDLER( pc98_mouse_r )
{
	//TODO: 0x7fd8 bit 6 controls testing of bank 3 (0x80000-0x9ffff), afaik the mouse ppi is at odd addresses...
	return 0xff;
}

static READ32_HANDLER( gfx_bitmap_ram_r ) { return gfx_bitmap_ram[offset+ems_bank*0x18000/4]; }
static WRITE32_HANDLER( gfx_bitmap_ram_w ) { COMBINE_DATA(&gfx_bitmap_ram[offset+ems_bank*0x18000/4]); }

static READ32_HANDLER( wram_bank_r ) { return work_ram_banked[offset+wram_bank*0x2000/4]; }
static WRITE32_HANDLER( wram_bank_w ) { COMBINE_DATA(&work_ram_banked[offset+wram_bank*0x2000/4]); }

static READ32_HANDLER( wram_ide_r ) { return work_ram_banked[offset+4*0x2000/4]; } //todo: ide hookup & calculate the correct bank number here
static WRITE32_HANDLER( wram_ide_w ) { COMBINE_DATA(&work_ram_banked[offset+4*0x2000/4]); }

static ADDRESS_MAP_START( pc9801_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM
	AM_RANGE(0x00080000, 0x0009ffff) AM_READWRITE(wram_bank_r,wram_bank_w)
	AM_RANGE(0x000a0000, 0x000a3fff) AM_RAM AM_BASE(&pc9801_vram) //vram + attr
	AM_RANGE(0x000a4000, 0x000a4fff) AM_RAM //cg window
	AM_RANGE(0x000a5000, 0x000a7fff) AM_RAM //??? (presumably another work ram bank)
	AM_RANGE(0x000a8000, 0x000bffff) AM_READWRITE(gfx_bitmap_ram_r,gfx_bitmap_ram_w)
	AM_RANGE(0x000c0000, 0x000dffff) AM_READWRITE(wram_ide_r,wram_ide_w)
	AM_RANGE(0x000e0000, 0x000fffff) AM_ROMBANK(1) AM_WRITE8(rom_bank_w,0xffffffff)
	AM_RANGE(0xfffe0000, 0xffffffff) AM_ROMBANK(1) AM_WRITE8(rom_bank_w,0xffffffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9801_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE8(port_00_r,port_00_w,0xffffffff) // pic8259 (even ports) / dma (odd ports)
//	AM_RANGE(0x0020, 0x0020) rtc
//	AM_RANGE(0x0022, 0x0022)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(sys_port_r,sys_port_w,0xffffffff) // rs232c (even ports) / system ppi8255 (odd ports)
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(sio_port_r,sio_port_w,0xffffffff) // printer ppi8255 (even ports) / keyboard (odd ports)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(port_60_r,port_60_w,0xffffffff) // uPD7220 status & fifo (R) / param & cmd (W) master (even ports)
//	AM_RANGE(0x0064, 0x0064) V-SYNC related write
//	AM_RANGE(0x0068, 0x0068) Flip-Flop 1 r/w
//	AM_RANGE(0x006a, 0x006a) Flip-Flop 2 r/w
//	AM_RANGE(0x006e, 0x006e) Flip-Flop 3 r/w
//	AM_RANGE(0x0070, 0x0070) crtc registers
//	AM_RANGE(0x0072, 0x0072)
//	AM_RANGE(0x0074, 0x0074)
//	AM_RANGE(0x0076, 0x0076)
//	AM_RANGE(0x0078, 0x0078)
//	AM_RANGE(0x007a, 0x007a)
//	AM_RANGE(0x007c, 0x007c) GRCG mode write
//	AM_RANGE(0x007e, 0x007e) GRCG tile write
	AM_RANGE(0x0070, 0x007f) AM_READWRITE8(port_70_r,port_70_w,0xffffffff) // crtc regs (even ports) / pit8253 (odd ports)
	AM_RANGE(0x00a0, 0x00a3) AM_READWRITE8(port_a0_r,port_a0_w,0xffffffff) // uPD7220 status & fifo (R) / param & cmd (W) slave (even ports)
// 	AM_RANGE(0x00e0, 0x00ef) DMA
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE8(port_f0_r,port_f0_w,0xffffffff)
	AM_RANGE(0x043c, 0x043f) AM_WRITE8(ems_sel_w,0xffffffff)
	AM_RANGE(0x0460, 0x0463) AM_READWRITE8(wram_sel_r,wram_sel_w,0xffffffff)
	AM_RANGE(0x7fd8, 0x7fdf) AM_READ8(pc98_mouse_r,0xffffffff)

//	(and many more...)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9821_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pc9801 )
	PORT_START("VBLANK")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static IRQ_CALLBACK(irq_callback)
{
	int r = 0;
	r = pic8259_acknowledge( devtag_get_device( device->machine, "pic8259_slave" ));
	if (r==0)
	{
		r = pic8259_acknowledge( devtag_get_device( device->machine, "pic8259_master" ));
		//printf("%02x ACK\n",r);
	}
	return r;
}

#include "cpu/i386/i386priv.h"


static MACHINE_RESET(pc9801)
{
	UINT8 *ROM = memory_region(machine, "cpudata");

	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), irq_callback);

	cputag_set_input_line(machine, "maincpu", INPUT_LINE_A20, 0);

	gate_a20 = 0;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), I386_EIP, 0xffff0+0x10000);

	memory_set_bankptr(machine, 1, &ROM[0x20000]);

	wram_bank = 0;
	rom_bank = 1;
}

static MACHINE_RESET(pc9821)
{
	UINT8 *ROM = memory_region(machine, "cpudata");
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), irq_callback);

	cputag_set_input_line(machine, "maincpu", INPUT_LINE_A20, 0);

	gate_a20 = 0;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), I386_EIP, 0xffff0+0x10000);

	memory_set_bankptr(machine, 1, &ROM[0x20000]);

	wram_bank = 0;
	rom_bank = 1;
}

/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

static PIC8259_SET_INT_LINE( pc98_master_set_int_line ) {
	//printf("%02x\n",interrupt);
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static PIC8259_SET_INT_LINE( pc98_slave_set_int_line ) {
	pic8259_set_irq_line( devtag_get_device( device->machine, "pic8259_master" ), 2, interrupt);
}


static const struct pic8259_interface pic8259_master_config = {
	pc98_master_set_int_line
};


static const struct pic8259_interface pic8259_slave_config = {
	pc98_slave_set_int_line
};

static VIDEO_START( pc9821 )
{
}

static VIDEO_UPDATE( pc9821 )
{
	return 0;
}

static const gfx_layout charset =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( pc9801 )
	GFXDECODE_ENTRY( "gfx1",   0x00000, charset,    0x000, 0x01 )
GFXDECODE_END

/*
 * PPI8255
 */

static READ8_DEVICE_HANDLER( pc98_porta_r )
{
	//printf("PPI Port A read\n");
	return 0x73; //bit 2 is interlace apparently
}

static READ8_DEVICE_HANDLER( pc98_portb_r )
{
	static UINT8 tmp;

	tmp ^= 1; //<- actually related to the RTC somehow
	//printf("PPI Port B read\n");
	return 0xf8 | tmp;
}

static READ8_DEVICE_HANDLER( pc98_portc_r )
{
	//printf("PPI Port C read\n");
	return 0x00;
}

static WRITE8_DEVICE_HANDLER( pc98_porta_w )
{
	//printf("PPI Port A write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( pc98_portb_w )
{
	//printf("PPI Port B write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( pc98_portc_w )
{
	//printf("PPI Port C write %02x\n",data);
}

static const ppi8255_interface ppi8255_intf =
{
	DEVCB_HANDLER(pc98_porta_r),					/* Port A read */
	DEVCB_HANDLER(pc98_portb_r),					/* Port B read */
	DEVCB_HANDLER(pc98_portc_r),					/* Port C read */
	DEVCB_HANDLER(pc98_porta_w),					/* Port A write */
	DEVCB_HANDLER(pc98_portb_w),					/* Port B write */
	DEVCB_HANDLER(pc98_portc_w)						/* Port C write */
};


static READ8_DEVICE_HANDLER( printer_porta_r )
{
	printf("PPI2 Port A read\n");
	return 0x00;
}

static READ8_DEVICE_HANDLER( printer_portb_r )
{
//	printf("PPI2 Port B read\n");
	return 0xb4;
}

static READ8_DEVICE_HANDLER( printer_portc_r )
{
	printf("PPI2 Port C read\n");
	return 0x00;
}


static const ppi8255_interface printer_intf =
{
	DEVCB_HANDLER(printer_porta_r),					/* Port A read */
	DEVCB_HANDLER(printer_portb_r),					/* Port B read */
	DEVCB_HANDLER(printer_portc_r),					/* Port C read */
	DEVCB_NULL,					/* Port A write */
	DEVCB_NULL,					/* Port B write */
	DEVCB_NULL					/* Port C write */
};

static PIT8253_OUTPUT_CHANGED( pc_timer0_w )
{
	pic8259_set_irq_line(devtag_get_device( device->machine, "pic8259_master" ), 0, state);
}

static const struct pit8253_config pit8253_config =
{
	{
		{
			16000000/4,				/* heartbeat IRQ */
			pc_timer0_w
		}, {
			16000000/4,				/* dram refresh */
			NULL
		}, {
			16000000/4,				/* pio port c pin 4, and speaker polling enough */
			NULL
		}
	}
};

static PALETTE_INIT( pc9801 )
{
	int i;

	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i+0x000, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

static UINT8 dma_offset[2][4];

#if 0
static UINT8 at_pages[0x10];


static READ8_HANDLER(at_page8_r)
{
	UINT8 data = at_pages[offset % 0x10];

	switch(offset % 8) {
	case 1:
		data = dma_offset[(offset / 8) & 1][2];
		break;
	case 2:
		data = dma_offset[(offset / 8) & 1][3];
		break;
	case 3:
		data = dma_offset[(offset / 8) & 1][1];
		break;
	case 7:
		data = dma_offset[(offset / 8) & 1][0];
		break;
	}
	return data;
}


static WRITE8_HANDLER(at_page8_w)
{
	at_pages[offset % 0x10] = data;

	switch(offset % 8) {
	case 1:
		dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}
#endif

static DMA8237_HRQ_CHANGED( pc_dma_hrq_changed )
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	dma8237_set_hlda( device, state );
}


static DMA8237_MEM_READ( pc_dma_read_byte )
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0xFF0000;

	return memory_read_byte(space, page_offset + offset);
}


static DMA8237_MEM_WRITE( pc_dma_write_byte )
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0xFF0000;

	memory_write_byte(space, page_offset + offset, data);
}


static const struct dma8237_interface dma8237_1_config =
{
	XTAL_14_31818MHz/3,

	pc_dma_hrq_changed,
	pc_dma_read_byte,
	pc_dma_write_byte,

	{ NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL },
	NULL
};

/* I suspect the dump for pc9801 comes from a i386 later model... the original machine would use a i8086 @ 5Mhz CPU (see notes at top) */
/* More investigations are required, but in the meanwhile I set a I386 as main CPU */
static MACHINE_DRIVER_START( pc9801 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I386, 16000000)
	MDRV_CPU_PROGRAM_MAP(pc9801_mem)
	MDRV_CPU_IO_MAP(pc9801_io)

	MDRV_MACHINE_RESET(pc9801)

	MDRV_PPI8255_ADD( "ppi8255_0", ppi8255_intf )
	MDRV_PPI8255_ADD( "ppi8255_1", printer_intf )
	MDRV_PIT8253_ADD( "pit8253", pit8253_config )
	MDRV_DMA8237_ADD( "dma8237_1", dma8237_1_config )
	MDRV_PIC8259_ADD( "pic8259_master", pic8259_master_config )
	MDRV_PIC8259_ADD( "pic8259_slave", pic8259_slave_config )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(pc9801)
	MDRV_GFXDECODE(pc9801)

	MDRV_VIDEO_START(pc9801)
	MDRV_VIDEO_UPDATE(pc9801)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc9821 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I486, 25000000)
	MDRV_CPU_PROGRAM_MAP(pc9821_mem)

	MDRV_MACHINE_RESET(pc9821)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(pc9821)
	MDRV_VIDEO_UPDATE(pc9821)
MACHINE_DRIVER_END


/* ROM definition */


/* I strongly suspect this dump comes from a later machine, like an i386-based one, but I could be wrong... */
ROM_START( pc9801 )
	ROM_REGION( 0x60000, "cpudata", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x08000, 0x18000, CRC(315d2703) SHA1(4f208d1dbb68373080d23bff5636bb6b71eb7565) )
	ROM_LOAD( "itf.rom",  0x38000, 0x08000, CRC(c1815325) SHA1(a2fb11c000ed7c976520622cfb7940ed6ddc904e) )

	/* where shall we load this? */
	ROM_REGION( 0x100000, "memory", 0 )
	ROM_LOAD( "00000.rom", 0x00000, 0x8000, CRC(6e299128) SHA1(d0e7d016c005cdce53ea5ecac01c6f883b752b80) )
	ROM_LOAD( "c0000.rom", 0xc0000, 0x8000, CRC(1b43eabd) SHA1(ca711c69165e1fa5be72993b9a7870ef6d485249) )	// 0xff everywhere
	ROM_LOAD( "c8000.rom", 0xc8000, 0x8000, CRC(f2a262b0) SHA1(fe97d2068d18bbb7425d9774e2e56982df2aa1fb) )
	ROM_LOAD( "d0000.rom", 0xd0000, 0x8000, CRC(1b43eabd) SHA1(ca711c69165e1fa5be72993b9a7870ef6d485249) )	// 0xff everywhere
	ROM_LOAD( "d8000.rom", 0xd8000, 0x8000, CRC(5dda57cc) SHA1(d0dead41c5b763008a4d777aedddce651eb6dcbb) )
	ROM_LOAD( "e8000.rom", 0xe8000, 0x8000, CRC(4e32081e) SHA1(e23571273b7cad01aa116cb7414c5115a1093f85) )	// contains n-88 basic (86) v2.0
	ROM_LOAD( "f0000.rom", 0xf0000, 0x8000, CRC(4da85a6c) SHA1(18dccfaf6329387c0c64cc4c91b32c25cde8bd5a) )
	ROM_LOAD( "f8000.rom", 0xf8000, 0x8000, CRC(2b1e45b1) SHA1(1fec35f17d96b2e2359e3c71670575ad9ff5007e) )

	ROM_REGION( 0x10000, "soundcpu", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x50000, "gfx1", 0 )
	ROM_LOAD( "font.rom", 0x00000, 0x46800, BAD_DUMP CRC(da370e7a) SHA1(584d0c7fde8c7eac1f76dc5e242102261a878c5e) )
ROM_END

ROM_START( pc9821 )
	ROM_REGION( 0x60000, "cpudata", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x08000, 0x18000, CRC(34a19a59) SHA1(2e92346727b0355bc1ec9a7ded1b444a4917f2b9) )
	ROM_LOAD( "itf.rom",  0x38000, 0x08000, CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )

	ROM_REGION( 0x10000, "soundcpu", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x50000, "gfx1", 0 )
	ROM_LOAD( "font.rom", 0x00000, 0x46800, BAD_DUMP CRC(a61c0649) SHA1(554b87377d176830d21bd03964dc71f8e98676b1) )
ROM_END


/*    YEAR  NAME      PARENT   COMPAT MACHINE   INPUT     INIT    CONFIG     COMPANY                        FULLNAME    FLAGS */
COMP( 1981, pc9801,   0,       0,     pc9801,   pc9801,   0,      0,    "Nippon Electronic Company",   "PC-9801",  GAME_NOT_WORKING )
COMP( 1993, pc9821,   0,       0,     pc9801,   pc9801,   0,      0,    "Nippon Electronic Company",   "PC-9821 (98MATE)",  GAME_NOT_WORKING )
