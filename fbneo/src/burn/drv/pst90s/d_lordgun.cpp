// FB Neo Lord of Gun driver module
// Based on MAME driver by Luca Elia, XingXing, and David Haywood

#include "tiles_generic.h"
#include "m68000_intf.h"
#include "z80_intf.h"
#include "burn_ymf278b.h"
#include "burn_ym3812.h"
#include "msm6295.h"
#include "8255ppi.h"
#include "eeprom.h"
#include "burn_gun.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *Drv68KROM;
static UINT8 *DrvZ80ROM;
static UINT8 *DrvGfxROM[4];
static UINT8 *DrvSndROM[3];
static UINT8 *Drv68KRAM;
static UINT8 *DrvPriRAM;
static UINT8 *DrvPalRAM;
static UINT8 *DrvVidRAM0;
static UINT8 *DrvVidRAM1;
static UINT8 *DrvVidRAM2;
static UINT8 *DrvVidRAM3;
static UINT8 *DrvScrRAM;
static UINT8 *DrvSprRAM;
static UINT8 *DrvZ80RAM;
static UINT8 *EEPROM;

static UINT32 *DrvPalette;
static UINT8  DrvRecalc;

static UINT16 *scrollx;
static UINT16 *scrolly;
static UINT16 *priority;
static UINT8 *soundlatch;
static UINT8 *okibank;

static UINT8 *DrvTransTable[5];
static UINT16 *draw_bitmap[5];

static UINT8 aliencha_dip_sel;
static UINT8 lordgun_whitescreen;
static INT32 eeprom_old;

static UINT16 lordgun_protection_data;

static INT16 DrvAxis[4];
static UINT16 DrvAnalogInput[4];
static UINT16 DrvInputs[5];
static UINT8 DrvJoy1[16];
static UINT8 DrvJoy2[16];
static UINT8 DrvJoy3[16];
static UINT8 DrvJoy4[16];
static UINT8 DrvJoy5[16];
static UINT8 DrvDips[4];
static UINT8 DrvReset;

static INT32 lordgun_gun_hw_x[2];
static INT32 lordgun_gun_hw_y[2];

#define A(a, b, c, d) { a, b, (UINT8*)(c), d }
static struct BurnInputInfo LordgunInputList[] = {
	{"P1 Coin",				BIT_DIGITAL,	DrvJoy4 + 0,	"p1 coin"		},
	{"P1 Start",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 start"		},
	A("P1 Right / left",	BIT_ANALOG_REL, DrvAxis + 0,	"mouse x-axis"	),
	A("P1 Up / Down",		BIT_ANALOG_REL, DrvAxis + 1,	"mouse y-axis"	),
	{"P1 Button 1",			BIT_DIGITAL,	DrvJoy5 + 4,	"mouse button"	},

	{"P2 Coin",				BIT_DIGITAL,	DrvJoy4 + 1,	"p2 coin"		},
	{"P2 Start",			BIT_DIGITAL,	DrvJoy2 + 0,	"p2 start"		},
	A("P2 Right / left",	BIT_ANALOG_REL, DrvAxis + 2,	"mouse x-axis"	),
	A("P2 Up / Down",		BIT_ANALOG_REL, DrvAxis + 3,	"mouse y-axis"	),
	{"P2 Button 1",			BIT_DIGITAL,	DrvJoy5 + 5,	"mouse button"	},

	{"Reset",				BIT_DIGITAL,	&DrvReset,		"reset"			},
	{"Service",				BIT_DIGITAL,	DrvJoy3 + 4,	"service"		},
	{"Service",				BIT_DIGITAL,	DrvJoy1 + 7,	"service"		},
	{"Dip A",				BIT_DIPSWITCH,	DrvDips + 0,	"dip"			},
};

STDINPUTINFO(Lordgun)
#undef A

static struct BurnInputInfo AlienchaInputList[] = {
	{"P1 Coin",				BIT_DIGITAL,	DrvJoy4 + 0,	"p1 coin"		},
	{"P1 Start",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 start"		},
	{"P1 Up",				BIT_DIGITAL,	DrvJoy1 + 1,	"p1 up"			},
	{"P1 Down",				BIT_DIGITAL,	DrvJoy1 + 2,	"p1 down"		},
	{"P1 Left",				BIT_DIGITAL,	DrvJoy1 + 3,	"p1 left"		},
	{"P1 Right",			BIT_DIGITAL,	DrvJoy1 + 4,	"p1 right"		},
	{"P1 Button 1",			BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 1"		},
	{"P1 Button 2",			BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 2"		},
	{"P1 Button 3",			BIT_DIGITAL,	DrvJoy1 + 7,	"p1 fire 3"		},
	{"P1 Button 4",			BIT_DIGITAL,	DrvJoy4 + 2,	"p1 fire 4"		},
	{"P1 Button 5",			BIT_DIGITAL,	DrvJoy4 + 3,	"p1 fire 5"		},
	{"P1 Button 6",			BIT_DIGITAL,	DrvJoy4 + 4,	"p1 fire 6"		},

	{"P2 Coin",				BIT_DIGITAL,	DrvJoy4 + 1,	"p2 coin"		},
	{"P2 Start",			BIT_DIGITAL,	DrvJoy2 + 0,	"p2 start"		},
	{"P2 Up",				BIT_DIGITAL,	DrvJoy2 + 1,	"p2 up"			},
	{"P2 Down",				BIT_DIGITAL,	DrvJoy2 + 2,	"p2 down"		},
	{"P2 Left",				BIT_DIGITAL,	DrvJoy2 + 3,	"p2 left"		},
	{"P2 Right",			BIT_DIGITAL,	DrvJoy2 + 4,	"p2 right"		},
	{"P2 Button 1",			BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 1"		},
	{"P2 Button 2",			BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 2"		},
	{"P2 Button 3",			BIT_DIGITAL,	DrvJoy2 + 7,	"p2 fire 3"		},
	{"P2 Button 4",			BIT_DIGITAL,	DrvJoy4 + 5,	"p2 fire 4"		},
	{"P2 Button 5",			BIT_DIGITAL,	DrvJoy4 + 6,	"p2 fire 5"		},
	{"P2 Button 6",			BIT_DIGITAL,	DrvJoy4 + 7,	"p2 fire 6"		},

	{"Reset",				BIT_DIGITAL,	&DrvReset,		"reset"			},
	{"Service",				BIT_DIGITAL,	DrvJoy3 + 1,	"service"		},
	{"Dip A",				BIT_DIPSWITCH,	DrvDips + 0,	"dip"			},
	{"Dip B",				BIT_DIPSWITCH,	DrvDips + 1,	"dip"			},
	{"Dip C",				BIT_DIPSWITCH,	DrvDips + 2,	"dip"			},
	{"Fake Dip",			BIT_DIPSWITCH,  DrvDips + 3,    "dip"			},
};

STDINPUTINFO(Aliencha)

static struct BurnDIPInfo LordgunDIPList[]=
{
	{0x0d, 0xff, 0xff, 0x7f, NULL					},

	{0   , 0xfe, 0   ,    2, "Stage Select"			},
	{0x0d, 0x01, 0x01, 0x01, "Off"					},
	{0x0d, 0x01, 0x01, 0x00, "On"					},

	{0   , 0xfe, 0   ,    2, "Guns"					},
	{0x0d, 0x01, 0x02, 0x02, "IGS"					},
	{0x0d, 0x01, 0x02, 0x00, "Konami"				},

	{0   , 0xfe, 0   ,    2, "Ranking Music"		},
	{0x0d, 0x01, 0x04, 0x04, "Exciting"				},
	{0x0d, 0x01, 0x04, 0x00, "Tender"				},

	{0   , 0xfe, 0   ,    2, "Coin Slots"			},
	{0x0d, 0x01, 0x08, 0x00, "1"					},
	{0x0d, 0x01, 0x08, 0x08, "2"					},

	{0   , 0xfe, 0   ,    2, "Service Mode"			},
	{0x0d, 0x01, 0x40, 0x40, "Off"					},
	{0x0d, 0x01, 0x40, 0x00, "On"					},
};

STDDIPINFO(Lordgun)

static struct BurnDIPInfo AlienchaDIPList[]=
{
	{0x1a, 0xff, 0xff, 0xff, NULL					},
	{0x1b, 0xff, 0xff, 0xff, NULL					},
	{0x1c, 0xff, 0xff, 0xff, NULL					},
	{0x1d, 0xff, 0xff, 0x00, NULL					},

	{0   , 0xfe, 0   ,    2, "Credits To Start"		},
	{0x1a, 0x01, 0x01, 0x01, "1"					},
	{0x1a, 0x01, 0x01, 0x00, "2"					},

	{0   , 0xfe, 0   ,    8, "Coin A"				},
	{0x1a, 0x01, 0x0e, 0x00, "5 Coins 1 Credits"	},
	{0x1a, 0x01, 0x0e, 0x02, "4 Coins 1 Credits"	},
	{0x1a, 0x01, 0x0e, 0x04, "3 Coins 1 Credits"	},
	{0x1a, 0x01, 0x0e, 0x06, "2 Coins 1 Credits"	},
	{0x1a, 0x01, 0x0e, 0x0e, "1 Coin  1 Credits"	},
	{0x1a, 0x01, 0x0e, 0x0c, "1 Coin  2 Credits"	},
	{0x1a, 0x01, 0x0e, 0x0a, "1 Coin  3 Credits"	},
	{0x1a, 0x01, 0x0e, 0x08, "1 Coin  4 Credits"	},

	{0   , 0xfe, 0   ,    8, "Coin B"		},
	{0x1a, 0x01, 0x70, 0x00, "5 Coins 1 Credits"	},
	{0x1a, 0x01, 0x70, 0x10, "4 Coins 1 Credits"	},
	{0x1a, 0x01, 0x70, 0x20, "3 Coins 1 Credits"	},
	{0x1a, 0x01, 0x70, 0x30, "2 Coins 1 Credits"	},
	{0x1a, 0x01, 0x70, 0x70, "1 Coin  1 Credits"	},
	{0x1a, 0x01, 0x70, 0x60, "1 Coin  2 Credits"	},
	{0x1a, 0x01, 0x70, 0x50, "1 Coin  3 Credits"	},
	{0x1a, 0x01, 0x70, 0x40, "1 Coin  4 Credits"	},

	{0   , 0xfe, 0   ,    2, "Coin Slots"			},
	{0x1a, 0x01, 0x80, 0x80, "1"					},
	{0x1a, 0x01, 0x80, 0x00, "2"					},

	{0   , 0xfe, 0   ,    4, "Difficulty"			},
	{0x1b, 0x01, 0x03, 0x03, "0"					},
	{0x1b, 0x01, 0x03, 0x02, "1"					},
	{0x1b, 0x01, 0x03, 0x01, "2"					},
	{0x1b, 0x01, 0x03, 0x00, "3"					},

	{0   , 0xfe, 0   ,    2, "Service Mode"			},
	{0x1b, 0x01, 0x04, 0x04, "Off"					},
	{0x1b, 0x01, 0x04, 0x00, "On"					},

	{0   , 0xfe, 0   ,    2, "Round Time"			},
	{0x1b, 0x01, 0x08, 0x00, "32 s"					},
	{0x1b, 0x01, 0x08, 0x08, "40 s"					},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"			},
	{0x1b, 0x01, 0x10, 0x00, "Off"					},
	{0x1b, 0x01, 0x10, 0x10, "On"					},

	{0   , 0xfe, 0   ,    2, "Free Play"			},
	{0x1b, 0x01, 0x20, 0x20, "Off"					},
	{0x1b, 0x01, 0x20, 0x00, "On"					},
	
	{0   , 0xfe, 0   ,    2, "Allow Join"			},
	{0x1b, 0x01, 0x40, 0x00, "No"					},
	{0x1b, 0x01, 0x40, 0x40, "Yes"					},

	{0   , 0xfe, 0   ,    2, "Allow Continue"		},
	{0x1b, 0x01, 0x80, 0x00, "No"					},
	{0x1b, 0x01, 0x80, 0x80, "Yes"					},

	{0   , 0xfe, 0   ,    3, "Buttons"				},
	{0x1c, 0x01, 0x03, 0x01, "3"					},
	{0x1c, 0x01, 0x03, 0x02, "4"					},
	{0x1c, 0x01, 0x03, 0x03, "6"					},

	{0   , 0xfe, 0   ,    2, "Vs. Rounds"			},
	{0x1c, 0x01, 0x04, 0x00, "3"					},
	{0x1c, 0x01, 0x04, 0x04, "5"					},

	{0   , 0xfe, 0   ,    2, "Language (text)"		},
	{0x1d, 0x01, 0x01, 0x00, "English"				},
	{0x1d, 0x01, 0x01, 0x01, "Chinese"				},

	{0   , 0xfe, 0   ,    2, "Language (char names)"},
	{0x1d, 0x01, 0x02, 0x00, "English"				},
	{0x1d, 0x01, 0x02, 0x02, "Chinese"				},

	{0   , 0xfe, 0   ,    2, "Title"				},
	{0x1d, 0x01, 0x04, 0x00, "Alien Challenge"		},
	{0x1d, 0x01, 0x04, 0x04, "Round House Rumble"	},
};

STDDIPINFO(Aliencha)

static struct BurnDIPInfo AlienchacDIPList[] = {
	{0x1d, 0xff, 0xff, 0x03, NULL					},
};

STDDIPINFOEXT(Alienchac, Aliencha, Alienchac)

static const short lordgun_gun_x_table[] =
{
	-100, 0x001,0x001,0x002,0x002,0x003,0x003,0x004,0x005,0x006,0x007,0x008,0x009,0x00A,0x00B,0x00C,
	0x00D,0x00E,0x00F,0x010,0x011,0x012,0x013,0x014,0x015,0x016,0x017,0x018,0x019,0x01A,0x01B,0x01C,
	0x01D,0x01E,0x01F,0x020,0x021,0x022,0x023,0x024,0x025,0x026,0x027,0x028,0x029,0x02A,0x02B,0x02C,
	0x02D,0x02E,0x02F,0x030,0x031,0x032,0x033,0x034,0x035,0x036,0x037,0x038,0x039,0x03A,0x03B,0x03C,
	0x03D,0x03E,0x03F,0x040,0x041,0x043,0x044,0x045,0x046,0x047,0x048,0x049,0x04A,0x04B,0x04C,0x04E,
	0x04F,0x050,0x051,0x052,0x053,0x054,0x055,0x056,0x057,0x059,0x05A,0x05B,0x05C,0x05D,0x05E,0x05F,
	0x060,0x061,0x05A,0x063,0x065,0x066,0x067,0x068,0x069,0x06A,0x06B,0x06C,0x06D,0x06E,0x06F,0x071,
	0x072,0x074,0x075,0x077,0x078,0x07A,0x07B,0x07D,0x07E,0x080,0x081,0x083,0x085,0x087,0x089,0x08B,
	0x08D,0x08E,0x08F,0x090,0x092,0x093,0x095,0x097,0x098,0x099,0x09A,0x09B,0x09C,0x09D,0x09E,0x0A0,
	0x0A1,0x0A2,0x0A3,0x0A4,0x0A5,0x0A6,0x0A7,0x0A8,0x0A9,0x0AA,0x0AC,0x0AD,0x0AE,0x0AF,0x0B0,0x0B1,
	0x0B2,0x0B3,0x0B4,0x0B5,0x0B6,0x0B8,0x0B9,0x0BA,0x0BB,0x0BC,0x0BD,0x0BE,0x0BF,0x0C0,0x0C1,0x0C2,
	0x0C4,0x0C5,0x0C6,0x0C7,0x0C8,0x0C9,0x0CA,0x0CB,0x0CC,0x0CD,0x0CF,0x0D0,0x0D1,0x0D2,0x0D3,0x0D4,
	0x0D5,0x0D6,0x0D7,0x0D8,0x0D9,0x0DB,0x0DC,0x0DD,0x0DE,0x0DF,0x0E0,0x0E1,0x0E2,0x0E3,0x0E4,0x0E5,
	0x0E7,0x0E8,0x0E9,0x0EA,0x0EB,0x0EC,0x0ED,0x0EE,0x0EF,0x0F0,0x0F1,0x0F3,0x0F4,0x0F5,0x0F6,0x0F7,
	0x0F8,0x0F9,0x0FA,0x0FB,0x0FC,0x0FE,0x0FF,0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,
	0x10A,0x10B,0x10C,0x10D,0x10E,0x10F,0x110,0x111,0x112,0x113,0x114,0x116,0x117,0x118,0x119,0x11A,
	0x11B,0x11C,0x11D,0x11E,0x11F,0x120,0x122,0x123,0x124,0x125,0x126,0x127,0x128,0x129,0x12A,0x12B,
	0x12C,0x12E,0x12F,0x130,0x131,0x132,0x133,0x134,0x135,0x136,0x137,0x139,0x13A,0x13B,0x13C,0x13D,
	0x13E,0x13F,0x140,0x141,0x142,0x143,0x145,0x146,0x147,0x148,0x149,0x14A,0x14B,0x14C,0x14D,0x14E,
	0x14F,0x151,0x152,0x153,0x154,0x155,0x156,0x157,0x158,0x159,0x15A,0x15B,0x15D,0x15E,0x15F,0x160,
	0x161,0x162,0x163,0x164,0x165,0x166,0x167,0x169,0x16A,0x16B,0x16C,0x16D,0x16E,0x16F,0x170,0x171,
	0x172,0x174,0x175,0x176,0x177,0x178,0x179,0x17A,0x17B,0x17C,0x17D,0x17E,0x17F,0x180,0x181,0x182,
	0x183,0x184,0x185,0x186,0x187,0x188,0x189,0x18A,0x18B,0x18C,0x18D,0x18E,0x18F,0x190,0x191,0x192,
	0x193,0x194,0x195,0x196,0x197,0x198,0x199,0x19A,0x19B,0x19C,0x19D,0x19E,0x19F,0x1A0,0x1A1,0x1A2,
	0x1A3,0x1A4,0x1A5,0x1A6,0x1A7,0x1A8,0x1A9,0x1AA,0x1AB,0x1AC,0x1AD,0x1AE,0x1AF,0x1B0,0x1B1,0x1B2,
	0x1B3,0x1B4,0x1B5,0x1B6,0x1B7,0x1B8,0x1B9,0x1BA,0x1BB,0x1BC,0x1BD,0x1BE,-100,-100
};

static void lordgun_update_gun(INT32 i)
{
	INT32 x = DrvAnalogInput[i] - 0x3c;

	lordgun_gun_hw_x[i] = DrvAnalogInput[i];
	lordgun_gun_hw_y[i] = DrvAnalogInput[i+2];

	if ((x < 0) || (x > (INT32)(sizeof(lordgun_gun_x_table)/sizeof(lordgun_gun_x_table[0]))) )
		x = 0;

	INT32 scrx = lordgun_gun_x_table[x];
	INT32 scry = DrvAnalogInput[i+2];

	if ((scrx < 0) || (scrx >= nScreenWidth) || (scry <= 0) || (scry >= 0xe0)) {
		lordgun_gun_hw_x[i] = lordgun_gun_hw_y[i] = 0; // reload! (any border shot)
	}
}

static void lordgun_protection_write(UINT32 offset)
{
	switch (offset & 0x60)
	{
		case 0x00/2: // increment counter
		{
			lordgun_protection_data++;
			lordgun_protection_data &= 0x1f;
		}
		return;

		case 0xc0/2: // reset protection device
		{
			lordgun_protection_data = 0;
		}
		return;
	}
}

static UINT8 lordgun_protection_read(INT32 offset)
{
	switch (offset & 0x60)
	{
		case 0x40/2: // bitswap and xor counter
		{
			UINT8 x = lordgun_protection_data;

			lordgun_protection_data  = ((( x >> 0) | ( x >> 1)) & 1) << 4;
			lordgun_protection_data |=  ((~x >> 2) & 1) << 3;
			lordgun_protection_data |= (((~x >> 4) | ( x >> 0)) & 1) << 2;
			lordgun_protection_data |=  (( x >> 3) & 1) << 1;
			lordgun_protection_data |= (((~x >> 0) | ( x >> 2)) & 1) << 0;

			return 0;
		}

		case 0x80/2: // return value if conditions are met
		{
			if ((lordgun_protection_data & 0x11) == 0x01) return 0x10;
			if ((lordgun_protection_data & 0x06) == 0x02) return 0x10;
			if ((lordgun_protection_data & 0x09) == 0x08) return 0x10;

			return 0;
		}
	}

	return 0;
}

static void aliencha_protection_write(UINT32 offset)
{
	switch (offset & 0x60)
	{
		case 0xc0/2: // reset protection device
		{
			lordgun_protection_data = 0;

			return;
		}
	}
}

static UINT8 aliencha_protection_read(INT32 offset)
{
	switch (offset & 0x60)
	{
		case 0x00/2: // de-increment counter
		{
			lordgun_protection_data--;
			lordgun_protection_data &= 0x1f;

			return 0;
		}

		case 0x40/2: // bitswap and xor counter
		{
			UINT8 x = lordgun_protection_data;

			lordgun_protection_data  = (((x >> 3) ^ (x >> 2)) & 1) << 4;
			lordgun_protection_data |= (((x >> 2) ^ (x >> 1)) & 1) << 3;
			lordgun_protection_data |= (((x >> 1) ^ (x >> 0)) & 1) << 2;
			lordgun_protection_data |= (((x >> 4) ^ (x >> 0)) & 1) << 1;
			lordgun_protection_data |= (((x >> 4) ^ (x >> 3)) & 1) << 0;

			return 0;
		}

		case 0x80/2: // return value if conditions are met
		{
			if ((lordgun_protection_data & 0x11) == 0x00) return 0x20;
			if ((lordgun_protection_data & 0x06) != 0x06) return 0x20;
			if ((lordgun_protection_data & 0x18) == 0x00) return 0x20;

			return 0;
		}
	}

	return 0;
}

static void __fastcall lordgun_write_word(UINT32 address, UINT16 data)
{
	if ((address & 0xfffff00) == 0x50a900) {
		lordgun_protection_write(address/2);
		return;
	}

	if ((address & 0xfffff00) == 0x50b900) {
		aliencha_protection_write(address/2);
		return;
	}

	switch (address)
	{
		case 0x502000:
		case 0x502200:
		case 0x502400:
		case 0x502600:
			scrollx[(address >> 9) & 3] = data;
		return;

		case 0x502800:
		case 0x502a00:
		case 0x502c00:
		case 0x502e00:
			scrolly[(address >> 9) & 3] = data;
		return;

		case 0x503000:
			priority[0] = data;
		return;

		case 0x504000:
			soundlatch[0] = (data >> 8) & 0xff;
			soundlatch[1] = data & 0xff;
			ZetNmi();
		return;

		case 0x506000:
		case 0x506002:
		case 0x506004:
		case 0x506006:
			ppi8255_w(0, (address >> 1) & 3, data);
		return;

		case 0x508000:
		case 0x508002:
		case 0x508004:
		case 0x508006:
			ppi8255_w(1, (address >> 1) & 3, data);
		return;
	}
}

static void __fastcall lordgun_write_byte(UINT32 /*address*/, UINT8 /*data*/)
{
//	bprintf(0, _T("write_byte: %X %X\n"), address, data);
}

static UINT16 __fastcall lordgun_read_word(UINT32 address)
{
	if ((address & 0xfffff00) == 0x50a900) {
		return lordgun_protection_read(address/2);
	}

	if ((address & 0xfffff00) == 0x50b900) {
		return aliencha_protection_read(address/2);
	}

	switch (address)
	{
		case 0x503800:
			return lordgun_gun_hw_x[0];

		case 0x503a00:
			return lordgun_gun_hw_x[1];

		case 0x503c00:
			return lordgun_gun_hw_y[0];

		case 0x503e00:
			return lordgun_gun_hw_y[1];

		case 0x506000:
		case 0x506002:
		case 0x506004:
		case 0x506006:
			return ppi8255_r(0, (address >> 1) & 3);

		case 0x508000:
		case 0x508002:
		case 0x508004:
		case 0x508006:
			return ppi8255_r(1, (address >> 1) & 3);
	}

	return 0;
}

static UINT8 __fastcall lordgun_read_byte(UINT32 address)
{
	switch (address)
	{
		case 0x506001:
		case 0x506003:
		case 0x506005:
		case 0x506007:
			return ppi8255_r(0, (address >> 1) & 3);

		case 0x508001:
		case 0x508003:
		case 0x508005:
		case 0x508007:
			return ppi8255_r(1, (address >> 1) & 3);
	}

	return 0;
}

static void set_oki_bank(INT32 bank)
{
	okibank[0] = bank;

	bank = (bank >> 1) & 1;
	MSM6295SetBank(0, DrvSndROM[0] + (bank * 0x40000), 0, 0x3ffff);
}

static void __fastcall lordgun_sound_write_port(UINT16 port, UINT8 data)
{
	switch (port)
	{
		case 0x1000:	// lordgun
		case 0x1001:
			BurnYM3812Write(0, port & 1, data);
		return;

		case 0x2000:	// lordgun
			MSM6295Write(0, data);
		return;

		case 0x6000:	// lordgun
			set_oki_bank(data);
		return;

		case 0x7000: 	// aliencha
		case 0x7001:
		case 0x7002:
		case 0x7003:
		case 0x7004:
		case 0x7005:
			BurnYMF278BWrite(port & 7, data);
		return;

		case 0x7400:	// aliencha
			MSM6295Write(0, data);
		return;

		case 0x7800:	// aliencha
			MSM6295Write(1, data);
		return;
	}
}

static UINT8 __fastcall lordgun_sound_read_port(UINT16 port)
{
	switch (port)
	{
		case 0x2000:	// lordgun
			return MSM6295Read(0);

		case 0x3000:
			return soundlatch[0];

		case 0x4000:
			return soundlatch[1];

		case 0x7000:	// aliencha
			return BurnYMF278BReadStatus();

		case 0x7400:	// aliencha
			return MSM6295Read(0);

		case 0x7800:	// aliencha
			return MSM6295Read(1);
	}

	return 0;
}

// ppi8255 handlers

static void aliencha_dip_select(UINT8 data)
{
	aliencha_dip_sel = data;
}

static UINT8 lordgun_dip_read()
{
	return (DrvDips[0] & 0x4f) | (EEPROMRead() ? 0x80 : 0) | (DrvInputs[4] & 0x30);
}

static void lordgun_eeprom_write(UINT8 data)
{
	for (INT32 i = 0; i < 2; i++)
		if ((data & (0x04 << i)) && !(eeprom_old & (0x04 << i)))
			lordgun_update_gun(i);

	// coin counter 0x01 (0)
	
	EEPROMWrite((data & 0x20), (data & 0x10), (data & 0x40));

	lordgun_whitescreen = data & 0x80;

	eeprom_old = data;
}

static void aliencha_eeprom_write(UINT8 data)
{
	lordgun_whitescreen = !(data & 0x02);

	// coin counters 0x08 (0) and 0x10 (1)

	EEPROMWrite((data & 0x40), (data & 0x20), (data & 0x80));
}

static UINT8 lordgun_start1_read() { return DrvInputs[0]; }
static UINT8 lordgun_start2_read() { return DrvInputs[1]; }
static UINT8 lordgun_service_read(){ return DrvInputs[2]; } 
static UINT8 lordgun_coin_read()   { return DrvInputs[3]; }

static UINT8 aliencha_service_read(){ return (DrvInputs[2] & 0xfe) | (EEPROMRead() ? 0x01 : 0); } // aliencha eeprom read...

static UINT8 aliencha_dip_read()
{
	switch (aliencha_dip_sel & 0x70) {
		case 0x30: return DrvDips[0];
		case 0x60: return DrvDips[1];
		case 0x50: return DrvDips[2];
	}
	return 0xff;
}

// sound irq / timing handlers

static void DrvFMIRQHandler(INT32, INT32 nStatus)
{
	ZetSetIRQLine(0, nStatus ? CPU_IRQSTATUS_ACK : CPU_IRQSTATUS_NONE);
}

static INT32 DrvSynchroniseStream(INT32 nSoundRate)
{
	return (INT64)ZetTotalCycles() * nSoundRate / 5000000;
}

static INT32 DrvDoReset()
{
	memset(AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	ZetOpen(0);
	ZetReset();
	BurnYMF278BReset(); // aliencha
	BurnYM3812Reset();
	MSM6295Reset();
	ZetClose();

	set_oki_bank(0); // lordgun

	EEPROMReset();

	aliencha_dip_sel	= 0;
	lordgun_whitescreen	= 0;
	eeprom_old = 0;

	// rom hacks
	if (!strncmp(BurnDrvGetTextA(DRV_NAME), "aliencha", 8)) {
		*((UINT16*)(Drv68KROM + 0x00a34)) = BURN_ENDIAN_SWAP_INT16(0x7000) | ((DrvDips[3] >> 0) & 1); // text language
		*((UINT16*)(Drv68KROM + 0x00a38)) = BURN_ENDIAN_SWAP_INT16(0x7000) | ((DrvDips[3] >> 1) & 1); // character name language
		*((UINT16*)(Drv68KROM + 0x00a3c)) = BURN_ENDIAN_SWAP_INT16(0x7000) | ((DrvDips[3] >> 2) & 1); // title
	} else {

		if (EEPROMAvailable() == 0) {
			EEPROMFill(EEPROM, 0, 0x80);
		}
	}

	return 0;
}

static INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x200000;
	DrvZ80ROM	= Next; Next += 0x010000;

	DrvGfxROM[0]	= Next; Next += 0x400000;
	DrvGfxROM[1]	= Next; Next += 0x800000;
	DrvGfxROM[2]	= Next; Next += 0x800000;
	DrvGfxROM[3]	= Next; Next += 0x1000000;

	DrvTransTable[0]= Next; Next += (0x0400000 / ( 8 *  8));
	DrvTransTable[1]= Next; Next += (0x0800000 / (16 * 16));
	DrvTransTable[2]= Next; Next += (0x0800000 / (32 * 32));
	DrvTransTable[3]= Next; Next += (0x1000000 / (16 * 16));
	DrvTransTable[4]= Next; Next += (0x0800000 / (16 * 16)) * 16;

	DrvSndROM[0]	= Next; Next += 0x100000;
	DrvSndROM[1]	= Next; Next += 0x100000;
	DrvSndROM[2]	= Next; Next += 0x200000;

	EEPROM			= Next; Next += 0x000080;

	DrvPalette	= (UINT32*)Next; Next += (0x0800 + 1) * sizeof(UINT32);

	draw_bitmap[0]	= (UINT16*)Next; Next += 448 * 240 * 2;
	draw_bitmap[1]	= (UINT16*)Next; Next += 448 * 240 * 2;
	draw_bitmap[2]	= (UINT16*)Next; Next += 448 * 240 * 2;
	draw_bitmap[3]	= (UINT16*)Next; Next += 448 * 240 * 2;
	draw_bitmap[4]	= (UINT16*)Next; Next += 448 * 240 * 2;

	AllRam		= Next;

	Drv68KRAM	= Next; Next += 0x010000;
	DrvPriRAM	= Next; Next += 0x010000;
	DrvVidRAM0	= Next; Next += 0x010000;
	DrvVidRAM1	= Next; Next += 0x004000;
	DrvVidRAM2	= Next; Next += 0x004000;
	DrvVidRAM3	= Next; Next += 0x002000;
	DrvScrRAM	= Next; Next += 0x000800;
	DrvSprRAM	= Next; Next += 0x000800;
	DrvPalRAM	= Next; Next += 0x001000;

	DrvZ80RAM	= Next; Next += 0x001000;

	scrollx		= (UINT16*)Next; Next += 0x000004 * sizeof(UINT16);
	scrolly		= (UINT16*)Next; Next += 0x000004 * sizeof(UINT16);
	priority	= (UINT16*)Next; Next += 0x000001 * sizeof(UINT16);

	soundlatch	= Next; Next += 0x000002;

	okibank		= Next; Next += 0x000001;

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static void DrvSetTransTab(UINT8 *gfx, INT32 tab, INT32 len, INT32 size)
{
	for (INT32 i = 0; i < len; i+= (size * size))
	{
		DrvTransTable[tab][i/(size*size)] = 1; // transparent

		for (INT32 j = 0; j < (size * size); j++) {
			if (gfx[i + j] != 0x3f) {
				DrvTransTable[tab][i/(size*size)] = 0;
				break;
			}
		}
	}
}

static void DrvGfxDecode(UINT8 *gfxsrc, UINT8 *gfxdest, INT32 len, INT32 size)
{
	INT32 Planes[6] = {
		(((len * 8) / 3) * 2) + 8, (((len * 8) / 3) * 2) + 0, 
		(((len * 8) / 3) * 1) + 8, (((len * 8) / 3) * 1) + 0,
		(((len * 8) / 3) * 0) + 8, (((len * 8) / 3) * 0) + 0
	};

	INT32 XOffs1[16] = { STEP8(0,1), STEP8(256,1) };
	INT32 XOffs2[32] = { STEP8(0,1), STEP8(512,1), STEP8(1024,1), STEP8(1536,1) };
	INT32 YOffs[32]  = { STEP16(0,16), STEP16(256,16) };

	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) {
		return;
	}

	memcpy(tmp, gfxsrc, len);

	GfxDecode(((len * 8) / 6) / (size*size), 6, size, size, Planes, (size == 32) ? XOffs2 : XOffs1, YOffs, (size*size*2), tmp, gfxdest);

	BurnFree(tmp);
}

static INT32 DrvInit(INT32 (*pInitCallback)(), INT32 lordgun)
{
	BurnAllocMemIndex();

	if (pInitCallback) {
		if (pInitCallback()) return 1;
	}

	DrvGfxDecode(DrvGfxROM[0], DrvGfxROM[0], 0x300000,  8);
	DrvGfxDecode(DrvGfxROM[2], DrvGfxROM[1], 0x600000, 16);
	DrvGfxDecode(DrvGfxROM[2], DrvGfxROM[2], 0x600000, 32);
	DrvGfxDecode(DrvGfxROM[3], DrvGfxROM[3], 0xc00000, 16);
	DrvSetTransTab(DrvGfxROM[0], 0, 0x0400000, 8);
	DrvSetTransTab(DrvGfxROM[1], 1, 0x0800000, 16);
	DrvSetTransTab(DrvGfxROM[2], 2, 0x0800000, 32);
	DrvSetTransTab(DrvGfxROM[3], 3, 0x1000000, 16);
	DrvSetTransTab(DrvGfxROM[1], 4, 0x0800000, 16/4); // for line scroll

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x1fffff, MAP_ROM);
	SekMapMemory(Drv68KRAM,		0x200000, 0x20ffff, MAP_RAM);
	SekMapMemory(DrvPriRAM,		0x210000, 0x21ffff, MAP_RAM);
	SekMapMemory(DrvVidRAM0,	0x300000, 0x30ffff, MAP_RAM);
	SekMapMemory(DrvVidRAM1,	0x310000, 0x313fff, MAP_RAM);
	SekMapMemory(DrvVidRAM2,	0x314000, 0x317fff, MAP_RAM);
	SekMapMemory(DrvVidRAM3,	0x318000, 0x319fff, MAP_RAM);
	SekMapMemory(DrvScrRAM,		0x31c000, 0x31c7ff, MAP_RAM);
	SekMapMemory(DrvSprRAM,		0x400000, 0x4007ff, MAP_RAM);
	SekMapMemory(DrvPalRAM,		0x500000, 0x500fff, MAP_RAM);
	SekSetWriteWordHandler(0,	lordgun_write_word);
	SekSetWriteByteHandler(0,	lordgun_write_byte);
	SekSetReadWordHandler(0,	lordgun_read_word);
	SekSetReadByteHandler(0,	lordgun_read_byte);
	SekClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM, 0x0000, 0xefff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM, 0xf000, 0xffff, MAP_RAM);
	ZetSetOutHandler(lordgun_sound_write_port);
	ZetSetInHandler(lordgun_sound_read_port);
	ZetClose();

	// aliencha
	BurnYMF278BInit(33868800, DrvSndROM[2], 0x200000, &DrvFMIRQHandler);
	BurnYMF278BSetAllRoutes(0.50, BURN_SND_ROUTE_BOTH);
	BurnTimerAttachZet((lordgun) ? 5000000 : 6000000);

	// lordgun
	BurnYM3812Init(1, 3579545, &DrvFMIRQHandler, &DrvSynchroniseStream, 0);
	BurnYM3812SetRoute(0, BURN_SND_YM3812_ROUTE, 1.00, BURN_SND_ROUTE_BOTH);

	MSM6295Init(0, 1000000 / 132, 1);
	MSM6295Init(1, 1000000 / 132, 1); // aliencha
	MSM6295SetRoute(0, 0.60, BURN_SND_ROUTE_BOTH);
	MSM6295SetRoute(1, 0.60, BURN_SND_ROUTE_BOTH);
	MSM6295SetBank(0, DrvSndROM[0], 0, 0x3ffff);
	MSM6295SetBank(1, DrvSndROM[1], 0, 0x3ffff);

	ppi8255_init(2);
	if (lordgun) {
		ppi8255_set_read_ports(0, lordgun_dip_read, NULL, lordgun_service_read);
		ppi8255_set_write_ports(0, NULL, lordgun_eeprom_write, NULL);
	} else {
		ppi8255_set_read_ports(0, aliencha_dip_read, NULL, aliencha_service_read);
		ppi8255_set_write_ports(0, NULL, aliencha_eeprom_write, aliencha_dip_select);
	}

	ppi8255_set_read_ports(1, lordgun_start1_read, lordgun_start2_read, lordgun_coin_read);

	EEPROMInit(&eeprom_interface_93C46);

	GenericTilesInit();

	BurnGunInit(2, true);

	DrvDoReset();

	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();

	BurnYMF278BExit(); // aliencha
	BurnYM3812Exit();
	MSM6295Exit();

	ppi8255_exit();
	BurnGunExit();

	SekExit();
	ZetExit();

	EEPROMExit();

	BurnFree(AllMem);

	return 0;
}

static INT32 lordgunLoadRoms()
{
	if (BurnLoadRom(Drv68KROM  + 0x000001,  0, 2)) return 1;
	if (BurnLoadRom(Drv68KROM  + 0x000000,  1, 2)) return 1;

	if (BurnLoadRom(DrvZ80ROM  + 0x000000,  2, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[0] + 0x000000,  3, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x100000,  4, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x200000,  5, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[2] + 0x000000,  6, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x200000,  7, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x400000,  8, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[3] + 0x000000,  9, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x200000, 10, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x200001, 11, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x300000, 12, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x300001, 13, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x400000, 14, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x600000, 15, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x600001, 16, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x700000, 17, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x700001, 18, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x800000, 19, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0xa00000, 20, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0xa00001, 21, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0xb00000, 22, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0xb00001, 23, 2)) return 1;

	if (BurnLoadRom(DrvSndROM[0] + 0x000000, 24, 1)) return 1;

	if (BurnLoadRomExt(EEPROM, 25, 1, LD_BYTESWAP)) return 1;

	UINT16 *rom = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x100000/2; i++) {
		if ((i & 0x0120) == 0x0100 || (i & 0x0a00) == 0x0800)
			rom[i] ^= BURN_ENDIAN_SWAP_INT16(0x0010);
	}

	return 0;
}

static INT32 lordgunuLoadRoms()
{
	if (BurnLoadRom(Drv68KROM  + 0x000001,  0, 2)) return 1;
	if (BurnLoadRom(Drv68KROM  + 0x000000,  1, 2)) return 1;

	if (BurnLoadRom(DrvZ80ROM  + 0x000000,  2, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[0] + 0x000000,  3, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x100000,  4, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x200000,  5, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[2] + 0x000000,  6, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x200000,  7, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x400000,  8, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[3] + 0x000000,  9, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x200000, 10, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x400000, 11, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x600000, 12, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x800000, 13, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0xa00000, 14, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[0] + 0x000000, 15, 1)) return 1;

	if (BurnLoadRomExt(EEPROM, 16, 1, LD_BYTESWAP)) return 1;

	UINT16 *rom = (UINT16*)Drv68KROM;

	for (INT32 i = 0; i < 0x100000/2; i++) {
		if ((i & 0x0120) == 0x0100 || (i & 0x0a00) == 0x0800)
			rom[i] ^= BURN_ENDIAN_SWAP_INT16(0x0010);
	}

	return 0;
}

static INT32 alienchaLoadRoms()
{
	if (BurnLoadRom(Drv68KROM  + 0x000000,  0, 1)) return 1;
	BurnByteswap(Drv68KROM, 0x200000);

	if (BurnLoadRom(DrvZ80ROM  + 0x000000,  1, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[0] + 0x000000,  2, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x100000,  3, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x200000,  4, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[2] + 0x000000,  5, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x200000,  6, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x400000,  7, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[3] + 0x000000,  8, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x400000,  9, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x800000, 10, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[0] + 0x000000, 11, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[1] + 0x000000, 12, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[2] + 0x000000, 13, 1)) return 1;

	return 0;
}

static INT32 alienchacLoadRoms()
{
/*
	if (BurnLoadRom(Drv68KROM  + 0x000000,  0, 1)) return 1;
	BurnByteswap(Drv68KROM, 0x200000);
	if (BurnLoadRom(Drv68KROM  + 0x000001,  1, 2)) return 1;
	if (BurnLoadRom(Drv68KROM  + 0x000000,  2, 2)) return 1;
*/
	if (BurnLoadRom(Drv68KROM + 0x000001, 0, 2)) return 1;
	if (BurnLoadRom(Drv68KROM + 0x000000, 1, 2)) return 1;
	if (BurnLoadRom(Drv68KROM + 0x100000, 2, 1)) return 1;
	BurnByteswap(Drv68KROM + 0x100000, 0x200000);

	if (BurnLoadRom(DrvZ80ROM  + 0x000000,  3, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[0] + 0x000000,  4, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x100000,  5, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[0] + 0x200000,  6, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[2] + 0x000000,  7, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x200000,  8, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[2] + 0x400000,  9, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM[3] + 0x000000, 10, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x400000, 11, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM[3] + 0x800000, 12, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[0] + 0x000000, 13, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[1] + 0x000000, 14, 1)) return 1;

	if (BurnLoadRom(DrvSndROM[2] + 0x000000, 15, 1)) return 1;

	return 0;
}

static void draw_layer_linescroll()
{
	UINT16 *vram = (UINT16*)DrvVidRAM1;
	UINT16 *sram = (UINT16*)DrvScrRAM;
	UINT16 *dest = draw_bitmap[1];

	for (INT32 y = 0; y < nScreenHeight; y++, dest += nScreenWidth)
	{
		INT32 yscroll = (scrolly[1] + y) & 0x1ff;

		for (INT32 x = 0; x < nScreenWidth + 16; x+=16) {

			INT32 xscroll = (scrollx[1] + sram[y * 2 + 1] + x) & 0x7ff;

			INT32 ofst = (((yscroll >> 4) << 7) | (xscroll >> 4)) << 1;

			INT32 attr = BURN_ENDIAN_SWAP_INT16(vram[ofst]);
			INT32 code = BURN_ENDIAN_SWAP_INT16(vram[ofst + 1]) & 0x7fff;
			INT32 pri  = (attr & 0x0e00) >> 9;

			INT32 flipx = attr & 0x8000;
			INT32 flipy = attr & 0x4000;

			INT32 color = ((attr & 0x0070) << 2)+0x600+pri*0x800;

			{
				UINT8 *gfx = DrvGfxROM[1] + (code << 8);

				if (flipy) gfx += ((yscroll & 0x0f)^0xf) << 4;
				else	   gfx += ((yscroll & 0x0f)    ) << 4;

				if (DrvTransTable[4][(gfx-DrvGfxROM[1])/16]) continue;

				if (flipx) flipx = 0x0f;

				for (INT32 xx = 0, sx = x - (xscroll & 0x0f); xx < 16; xx++, sx++) {
					if (sx < 0 || sx >= nScreenWidth) continue;

					INT32 pxl = gfx[xx^flipx];

					if (pxl == 0x3f) continue;

					dest[sx] = pxl | color;
				}
			}
		}
	}
}

static void draw_layer(UINT8 *ram, UINT8 *gfx, INT32 size, INT32 wide, INT32 high, INT32 color_offset, INT32 color_and, INT32 layer)
{
	INT32 twidth  = wide * size;
	INT32 theight = high * size;

	INT32 code_and = (size == 32) ? 0x1fff : ((size == 16) ? 0x7fff : 0xffff);

	UINT16 *vram = (UINT16*)ram;

	for (INT32 sy = 0, offs = 0; sy < high * size; sy+=size)
	{
		for (INT32 sx = 0; sx < wide * size; sx+=size, offs++)
		{
			INT32 code = BURN_ENDIAN_SWAP_INT16(vram[offs * 2 + 1]) & code_and;
			if (DrvTransTable[layer][code]) continue; // transparent

			INT32 attr = BURN_ENDIAN_SWAP_INT16(vram[offs * 2 + 0]);

			INT32 prio =  (attr >> 9) & 0x07;
			INT32 color= ((attr >> 4) & color_and) + color_offset / 0x40 + prio * 0x800 / 0x40;

			INT32 flipx= attr & 0x8000;
			INT32 flipy= attr & 0x4000;

			INT32 xx = sx - (scrollx[layer] % twidth);
			if (xx < (0 - (size - 1))) xx += twidth;

			INT32 yy = sy - (scrolly[layer] % theight);
			if (yy < (0 - (size - 1))) yy += theight;

			if (xx >= nScreenWidth || yy >= nScreenHeight) continue;

			if (flipy) {
				if (flipx) {
					RenderCustomTile_Mask_FlipXY_Clip(draw_bitmap[layer], size, size, code, xx, yy, color, 6, 0x3f, 0, gfx);
				} else {
					RenderCustomTile_Mask_FlipY_Clip(draw_bitmap[layer], size, size, code, xx, yy, color, 6, 0x3f, 0, gfx);
				}
			} else {
				if (flipx) {
					RenderCustomTile_Mask_FlipX_Clip(draw_bitmap[layer], size, size, code, xx, yy, color, 6, 0x3f, 0, gfx);
				} else {
					RenderCustomTile_Mask_Clip(draw_bitmap[layer], size, size, code, xx, yy, color, 6, 0x3f, 0, gfx);
				}
			}
		}
	}
}

static void draw_sprites()
{
	UINT16 *sprite = (UINT16*)DrvSprRAM;

	for (INT32 offs = 0; offs < 0x800 / 2; offs += 4)
	{
		INT32 x0, x1, dx, y0, y1, dy;

		INT32 attr = BURN_ENDIAN_SWAP_INT16(sprite[offs + 1]);
		if (attr & 0x0100) break;

		INT32 sy    = BURN_ENDIAN_SWAP_INT16(sprite[offs + 0]);
		INT32 code  = BURN_ENDIAN_SWAP_INT16(sprite[offs + 2]);
		INT32 sx    = BURN_ENDIAN_SWAP_INT16(sprite[offs + 3]);
		INT32 flipx = (attr & 0x8000);
		INT32 flipy = (attr & 0x4000);
		INT32 pri   = (attr & 0x0e00) >> 9;
		INT32 color = (attr & 0x00f0) >> 4;
		INT32 nx    = (attr & 0x000f) + 1;
		INT32 ny    = ((sy & 0xf000) >> 12) + 1;

		if (flipx) { x0 = nx - 1;	x1 = -1;	dx = -1; }
		else	   { x0 = 0;		x1 = nx;	dx = +1; }

		if (flipy) { y0 = ny - 1;	y1 = -1;	dy = -1; }
		else	   { y0 = 0;		y1 = ny;	dy = +1; }

		sx	-= 0x18;
		sy	 = (sy & 0x7ff) - (sy & 0x800);

		for (INT32 y = y0; y != y1; y += dy, code += 1 - 0x10 * nx)
		{
			for (INT32 x = x0; x != x1; x += dx, code += 0x10)
			{
				if (DrvTransTable[3][code]) continue;

				if (flipy) {
					if (flipx) {
						RenderCustomTile_Mask_FlipXY_Clip(draw_bitmap[4], 16, 16, code, sx + x * 0x10, sy + y * 0x10, color + pri * 0x800/0x40, 6, 0x3f, 0, DrvGfxROM[3]);
					} else {
						RenderCustomTile_Mask_FlipY_Clip(draw_bitmap[4], 16, 16, code, sx + x * 0x10, sy + y * 0x10, color + pri * 0x800/0x40, 6, 0x3f, 0, DrvGfxROM[3]);
					}
				} else {
					if (flipx) {
						RenderCustomTile_Mask_FlipX_Clip(draw_bitmap[4], 16, 16, code, sx + x * 0x10, sy + y * 0x10, color + pri * 0x800/0x40, 6, 0x3f, 0, DrvGfxROM[3]);
					} else {
						RenderCustomTile_Mask_Clip(draw_bitmap[4], 16, 16, code, sx + x * 0x10, sy + y * 0x10, color + pri * 0x800/0x40, 6, 0x3f, 0, DrvGfxROM[3]);
					}
				}
			}
		}
	}
}

static void copy_layers()
{
	const UINT8 pri2layer[8] = { 0, 0, 0, 4, 3, 0, 1, 2 };
	const UINT8 layerpri[5] = { 0, 1, 2, 4, 3 };

	UINT16 *dest	  = pTransDraw;
	UINT16 *source[5] = { draw_bitmap[0], draw_bitmap[1], draw_bitmap[2], draw_bitmap[3], draw_bitmap[4] };
	UINT16 *pri_ram = (UINT16*)DrvPriRAM;

	for (INT32 y = 0; y < nScreenHeight; y++)
	{
		for (INT32 x = 0; x < nScreenWidth; x++)
		{
			UINT16 pens[5];
			INT32 pri_addr = 0;

			for (INT32 l = 0; l < 5; l++) {
				pens[l] = *source[l]++;
				if (pens[l] == 0x3f) pri_addr |= 1 << layerpri[l];
			}

			pri_addr |= (pens[1] >> 11) << 5;
			pri_addr |= (pens[4] >> 11) << 8;
			pri_addr |= (pens[0] >> 11) << 11;
			pri_addr |= (pens[3] >> 11) << 14;

			*dest++ = pens[pri2layer[BURN_ENDIAN_SWAP_INT16(pri_ram[pri_addr & 0x7fff]) & 7]] & 0x7ff;
		}
	}
}

static void DrvPaletteRecalc()
{
	UINT16 *p = (UINT16*)DrvPalRAM;

	for (INT32 i = 0; i < 0x1000 / 2; i++) {
		INT32 r = (BURN_ENDIAN_SWAP_INT16(p[i]) >> 0) & 0x0f;
		INT32 g = (BURN_ENDIAN_SWAP_INT16(p[i]) >> 4) & 0x0f;
		INT32 b = (BURN_ENDIAN_SWAP_INT16(p[i]) >> 8) & 0x0f;

		r |= r << 4;
		g |= g << 4;
		b |= b << 4;

		DrvPalette[i] = BurnHighCol(r, g, b, 0);
	}

	DrvPalette[0x0800] = BurnHighCol(0xff, 0xff, 0xff, 0); // white
}

static INT32 DrvDraw()
{
	DrvPaletteRecalc();

	if (lordgun_whitescreen) {
		BurnTransferClear(0x800);

		BurnTransferCopy(DrvPalette);
		return 0;
	}

	for (INT32 o = 0; o < nScreenWidth * nScreenHeight; o++) {
		draw_bitmap[0][o] = draw_bitmap[1][o] = draw_bitmap[2][o] = draw_bitmap[3][o] = draw_bitmap[4][o] = 0x003f;
	}

	INT32 line_enable = 0;
	{
		UINT16 *rs = (UINT16*)DrvScrRAM;
		for (INT32 i = 0; i < nScreenHeight * 2; i+=2) {
			if ((BURN_ENDIAN_SWAP_INT16(rs[i + 1]) & 0x7ff) != (BURN_ENDIAN_SWAP_INT16(rs[1]) & 0x7ff)) {
				line_enable = 1;
				break;
			}
		}
	}

	draw_layer(DrvVidRAM0, DrvGfxROM[0],  8, 0x100, 0x040, 0x500, 0x03, 0);
	if (line_enable == 0) {
		draw_layer(DrvVidRAM1, DrvGfxROM[1], 16, 0x080, 0x020, 0x600, 0x07, 1);
	} else {
		draw_layer_linescroll();
	}
	draw_layer(DrvVidRAM2, DrvGfxROM[2], 32, 0x040, 0x010, 0x700, 0x03, 2);
	draw_layer(DrvVidRAM3, DrvGfxROM[0],  8, 0x040, 0x020, 0x400, 0x0f, 3);
	draw_sprites();

	copy_layers();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 lordgunDraw()
{
	DrvDraw();

	BurnGunDrawTargets();

	return 0;
}

static inline void compile_inputs()
{
	memset(DrvInputs, 0xff, 5 * sizeof(INT16));

	for (INT32 i = 0; i < 16; i++) {
		DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
		DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
		DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		DrvInputs[3] ^= (DrvJoy4[i] & 1) << i;
		DrvInputs[4] ^= (DrvJoy5[i] & 1) << i;
	}

	BurnGunMakeInputs(0, (INT16)DrvAxis[0], (INT16)DrvAxis[1]);
	BurnGunMakeInputs(1, (INT16)DrvAxis[2], (INT16)DrvAxis[3]);
		
	float x0 = ((float)((BurnGunX[0] >> 8) + 8)) / 448 * 412;
	float y0 = ((float)((BurnGunY[0] >> 8) + 8)) / 224 * 224;
	float x1 = ((float)((BurnGunX[1] >> 8) + 8)) / 448 * 412;
	float y1 = ((float)((BurnGunY[1] >> 8) + 8)) / 224 * 224;
	DrvAnalogInput[0] = (UINT16)x0 + 0x3c;
	DrvAnalogInput[2] = (UINT8)y0 + 0;
	DrvAnalogInput[1] = (UINT16)x1 + 0x3c;
	DrvAnalogInput[3] = (UINT8)y1 + 0;
}

static INT32 lordgunFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	compile_inputs();

	SekNewFrame();
	ZetNewFrame();

	INT32 nCyclesTotal[2] =  { 10000000 / 60, 5000000 / 60 };
	INT32 nCyclesDone[2] = { 0, 0 };

	INT32 nInterleave = 50;
	
	SekOpen(0);
	ZetOpen(0);

	for (INT32 i = 0; i < nInterleave; i++) {
		CPU_RUN(0, Sek);

		if (i == (nInterleave - 1)) SekSetIRQLine(4, CPU_IRQSTATUS_AUTO);

		CPU_RUN_TIMER(1);
	}

	ZetClose();
	SekClose();

	if (pBurnSoundOut) {
		BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
		MSM6295Render(pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		lordgunDraw();
	}

	return 0;
}

static INT32 alienchaFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	compile_inputs();

	SekNewFrame();
	ZetNewFrame();

	INT32 nInterleave = 100;

	INT32 nCyclesTotal[2] =  { 10000000 / 60, 6000000 / 60 };
	INT32 nCyclesDone[2] = { 0, 0 };

	SekOpen(0);
	ZetOpen(0);

	for (INT32 i = 0; i < nInterleave; i++) {
		CPU_RUN(0, Sek);

		if (i == (nInterleave - 1)) SekSetIRQLine(4, CPU_IRQSTATUS_AUTO);

		CPU_RUN_TIMER(1);
	}

	ZetClose();
	SekClose();

	if (pBurnSoundOut) {
		BurnYMF278BUpdate(nBurnSoundLen);
		MSM6295Render(pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029708;
	}

	if (nAction & ACB_VOLATILE) {
		memset(&ba, 0, sizeof(ba));
		ba.Data		= AllRam;
		ba.nLen		= RamEnd - AllRam;
		ba.szName	= "All RAM";
		BurnAcb(&ba);

		SekScan(nAction);
		ZetScan(nAction);

		BurnYMF278BScan(nAction, pnMin);
		BurnYM3812Scan(nAction, pnMin);
		MSM6295Scan(nAction, pnMin);

		ppi8255_scan();
		BurnGunScan();
		EEPROMScan(nAction, pnMin);

		SCAN_VAR(aliencha_dip_sel);
		SCAN_VAR(lordgun_whitescreen);
		SCAN_VAR(lordgun_protection_data);
		SCAN_VAR(eeprom_old);

		SCAN_VAR(lordgun_gun_hw_x);
		SCAN_VAR(lordgun_gun_hw_y);
	}

	if (nAction & ACB_WRITE) {
		set_oki_bank(okibank[0]);
	}

	return 0;
}


// Lord of Gun (World)

static struct BurnRomInfo lordgunRomDesc[] = {
	{ "lord_gun_u144-ch.u144",	0x080000, 0xea54ee18, 1 | BRF_PRG | BRF_ESS }, //  0 68k code
	{ "lord_gun_u122-ch.u122",	0x080000, 0x969a0348, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "lord_gun_160.u160",		0x010000, 0xd59b5e28, 2 | BRF_PRG | BRF_ESS }, //  2 z80 code

	{ "igs_t001.u8",			0x100000, 0x36dd96f3, 3 | BRF_GRA },           //  3 8x8 tiles
	{ "igs_t002.u18",			0x100000, 0x816a7665, 3 | BRF_GRA },           //  4
	{ "igs_t003.u19",			0x100000, 0xcbfee543, 3 | BRF_GRA },           //  5

	{ "igs_b001.u1",			0x200000, 0x3096de1c, 4 | BRF_GRA },           //  6 16x16 and 32x32 tiles
	{ "igs_b002.u2",			0x200000, 0x2234531e, 4 | BRF_GRA },           //  7
	{ "igs_b003.u9",			0x200000, 0x6cbf21ac, 4 | BRF_GRA },           //  8

	{ "igs_a001.u22",			0x200000, 0x400abe33, 5 | BRF_GRA },           //  9 Sprites
	{ "lord_gun_u24.u24",		0x080000, 0x454a5b11, 5 | BRF_GRA },           // 10
	{ "lord_gun_u23.u23",		0x080000, 0xa0d7aada, 5 | BRF_GRA },           // 11
	{ "lord_gun_u7.u7",			0x080000, 0x95ef3894, 5 | BRF_GRA },           // 12
	{ "lord_gun_u14.u14",		0x080000, 0xdc8a77a1, 5 | BRF_GRA },           // 13
	{ "igs_a002.u21",			0x200000, 0xa4810e38, 5 | BRF_GRA },           // 14
	{ "lord_gun_u5.u5",			0x080000, 0x63aa10c3, 5 | BRF_GRA },           // 15
	{ "lord_gun_u13.u13",		0x080000, 0x478e248c, 5 | BRF_GRA },           // 16
	{ "lord_gun_u4.u4",			0x080000, 0xd203c24e, 5 | BRF_GRA },           // 17
	{ "lord_gun_u11.u11",		0x080000, 0x72277dcd, 5 | BRF_GRA },           // 18
	{ "igs_a003.u20",			0x200000, 0x649e48d9, 5 | BRF_GRA },           // 19
	{ "lord_gun_u12.u12",		0x080000, 0xa2a55d65, 5 | BRF_GRA },           // 20
	{ "lord_gun_u6.u6",			0x080000, 0xfe649605, 5 | BRF_GRA },           // 21
	{ "lord_gun_u10.u10",		0x080000, 0xeea39e5e, 5 | BRF_GRA },           // 22
	{ "lord_gun_u3.u3",			0x080000, 0x233782f8, 5 | BRF_GRA },           // 23

	{ "lord_gun_u161-3.u161",	0x080000, 0xb4e0fa07, 6 | BRF_SND },           // 24 OKI #0 Samples

	{ "eeprom",					0x000080, 0x0dad0e43, 7 | BRF_PRG | BRF_ESS }, // 25 eeprom
};

STD_ROM_PICK(lordgun)
STD_ROM_FN(lordgun)

static INT32 lordgunInit()
{
	return DrvInit(lordgunLoadRoms, 1);
}

struct BurnDriver BurnDrvLordgun = {
	"lordgun", NULL, NULL, NULL, "1994",
	"Lord of Gun (World)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, lordgunRomInfo, lordgunRomName, NULL, NULL, NULL, NULL, LordgunInputInfo, LordgunDIPInfo,
	lordgunInit, DrvExit, lordgunFrame, lordgunDraw, DrvScan, &DrvRecalc, 0x800,
	448, 224, 4, 3
};


// Lord of Gun (USA)

static struct BurnRomInfo lordgunuRomDesc[] = {
	{ "lord_gun_u10.u10",	0x080000, 0xacda77ef, 1 | BRF_PRG | BRF_ESS }, //  0 68k code
	{ "lord_gun_u4.u4",		0x080000, 0xa1a61254, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "lord_gun_u90.u90",	0x010000, 0xd59b5e28, 2 | BRF_PRG | BRF_ESS }, //  2 z80 code

	{ "igs_t001.u108",		0x100000, 0x36dd96f3, 3 | BRF_GRA },           //  3 8x8 tiles
	{ "igs_t002.u114",		0x100000, 0x816a7665, 3 | BRF_GRA },           //  4
	{ "igs_t003.u119",		0x100000, 0xcbfee543, 3 | BRF_GRA },           //  5

	{ "igs_b001.u82",		0x200000, 0x3096de1c, 4 | BRF_GRA },           //  6 16x16 and 32x32 tiles
	{ "igs_b002.u91",		0x200000, 0x2234531e, 4 | BRF_GRA },           //  7
	{ "igs_b003.u97",		0x200000, 0x6cbf21ac, 4 | BRF_GRA },           //  8

	{ "igs_a001.u14",		0x200000, 0x400abe33, 5 | BRF_GRA },           //  9 Sprites
	{ "igs_a004.u13",		0x200000, 0x52687264, 5 | BRF_GRA },           // 10
	{ "igs_a002.u9",		0x200000, 0xa4810e38, 5 | BRF_GRA },           // 11
	{ "igs_a005.u8",		0x200000, 0xe32e79e3, 5 | BRF_GRA },           // 12
	{ "igs_a003.u3",		0x200000, 0x649e48d9, 5 | BRF_GRA },           // 13
	{ "igs_a006.u2",		0x200000, 0x39288eb6, 5 | BRF_GRA },           // 14

	{ "lord_gun_u100.u100",	0x080000, 0xb4e0fa07, 6 | BRF_SND },           // 15 OKI #0 Samples

	{ "eeprom",				0x000080, 0x0dad0e43, 7 | BRF_PRG | BRF_ESS }, // 16 eeprom
};

STD_ROM_PICK(lordgunu)
STD_ROM_FN(lordgunu)

static INT32 lordgunuInit()
{
	return DrvInit(lordgunuLoadRoms, 1);
}

struct BurnDriver BurnDrvLordgunu = {
	"lordgunu", "lordgun", NULL, NULL, "1994",
	"Lord of Gun (USA)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, lordgunuRomInfo, lordgunuRomName, NULL, NULL, NULL, NULL, LordgunInputInfo, LordgunDIPInfo,
	lordgunuInit, DrvExit, lordgunFrame, lordgunDraw, DrvScan, &DrvRecalc, 0x800,
	448, 224, 4, 3
};

// Alien Challenge (World)

static struct BurnRomInfo alienchaRomDesc[] = {
	{ "igsc0102.u81",	0x200000, 0xe3432be3, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code

	{ "hfh_s.u86",		0x010000, 0x5728a9ed, 2 | BRF_PRG | BRF_ESS }, //  1 z80 code

	{ "igst0101.u9",	0x100000, 0x2ce12d7b, 3 | BRF_GRA },           //  2 8x8 tiles
	{ "igst0102.u10",	0x100000, 0x542a76a0, 3 | BRF_GRA },           //  3
	{ "igst0103.u11",	0x100000, 0xadf5698a, 3 | BRF_GRA },           //  4

	{ "igsb0101.u8",	0x200000, 0x5c995f7e, 4 | BRF_GRA },           //  5 16x16 and 32x32 tiles
	{ "igsb0102.u7",	0x200000, 0xa2ae9baf, 4 | BRF_GRA },           //  6
	{ "igsb0103.u6",	0x200000, 0x11b927af, 4 | BRF_GRA },           //  7

	{ "igsa0101.u3",	0x400000, 0x374d07c4, 5 | BRF_GRA },           //  8 Sprites
	{ "igsa0102.u2",	0x400000, 0xdbeee7ac, 5 | BRF_GRA },           //  9
	{ "igsa0103.u1",	0x400000, 0xe5f19041, 5 | BRF_GRA },           // 10

	{ "hfh_g.u65",		0x040000, 0xec469b57, 6 | BRF_SND },           // 11 OKI #0 Samples

	{ "hfh_g.u66",		0x040000, 0x7cfcd98e, 7 | BRF_SND },           // 12 OKI #1 Samples

	{ "yrw801-m",		0x200000, 0x2a9d8d43, 8 | BRF_SND },           // 13 YMF278b Samples
};

STD_ROM_PICK(aliencha)
STD_ROM_FN(aliencha)

static INT32 alienchaInit()
{
	return DrvInit(alienchaLoadRoms, 0);
}

struct BurnDriver BurnDrvAliencha = {
	"aliencha", NULL, NULL, NULL, "1994",
	"Alien Challenge (World)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NULL, alienchaRomInfo, alienchaRomName, NULL, NULL, NULL, NULL, AlienchaInputInfo, AlienchaDIPInfo,
	alienchaInit, DrvExit, alienchaFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	448, 224, 4, 3
};


// Alien Challenge (China)

static struct BurnRomInfo alienchacRomDesc[] = {
	{ "hfh_p.u80",		0x080000, 0x5175ebdc, 1 | BRF_PRG | BRF_ESS }, //  0 68k code
	{ "hfh_p.u79",		0x080000, 0x42ad978c, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "igsc0101.u81",	0x200000, 0x704c48cf, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "alien_u-86.u86",	0x010000, 0x5728a9ed, 2 | BRF_PRG | BRF_ESS }, //  3 z80 code

	{ "igst0101.u9",	0x100000, 0x2ce12d7b, 3 | BRF_GRA },           //  4 8x8 tiles
	{ "igst0102.u10",	0x100000, 0x542a76a0, 3 | BRF_GRA },           //  5
	{ "igst0103.u11",	0x100000, 0xadf5698a, 3 | BRF_GRA },           //  6

	{ "igsb0101.u8",	0x200000, 0x5c995f7e, 4 | BRF_GRA },           //  7 16x16 and 32x32 tiles
	{ "igsb0102.u7",	0x200000, 0xa2ae9baf, 4 | BRF_GRA },           //  8
	{ "igsb0103.u6",	0x200000, 0x11b927af, 4 | BRF_GRA },           //  9

	{ "igsa0101.u3",	0x400000, 0x374d07c4, 5 | BRF_GRA },           // 10 Sprites
	{ "igsa0102.u2",	0x400000, 0xdbeee7ac, 5 | BRF_GRA },           // 11
	{ "igsa0103.u1",	0x400000, 0xe5f19041, 5 | BRF_GRA },           // 12

	{ "alien_u65.u65",	0x040000, 0xec469b57, 6 | BRF_SND },           // 11 OKI #0 Samples

	{ "alien_u66.u66",	0x040000, 0x7cfcd98e, 7 | BRF_SND },           // 12 OKI #1 Samples

	{ "yrw801-m",		0x200000, 0x2a9d8d43, 8 | BRF_SND },           // 13 YMF278b Samples
};

STD_ROM_PICK(alienchac)
STD_ROM_FN(alienchac)

static INT32 alienchacInit()
{
	return DrvInit(alienchacLoadRoms, 0);
}

struct BurnDriver BurnDrvAlienchac = {
	"alienchac", "aliencha", NULL, NULL, "1994",
	"Alien Challenge (China)\0", NULL, "IGS", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NULL, alienchacRomInfo, alienchacRomName, NULL, NULL, NULL, NULL, AlienchaInputInfo, AlienchacDIPInfo,
	alienchacInit, DrvExit, alienchaFrame, DrvDraw, DrvScan, &DrvRecalc, 0x800,
	448, 224, 4, 3
};
