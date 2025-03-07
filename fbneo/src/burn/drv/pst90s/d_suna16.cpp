// FB Alpha SunA 16-bit hardware driver module
// Based on MAME driver by Luca Elia

#include "tiles_generic.h"
#include "m68000_intf.h"
#include "z80_intf.h"
#include "burn_ym2151.h"
#include "burn_ym3526.h"
#include "dac.h"
#include "ay8910.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *Drv68KROM;
static UINT8 *DrvZ80ROM0;
static UINT8 *DrvZ80ROM1;
static UINT8 *DrvZ80ROM2;
static UINT8 *DrvGfxROM0;
static UINT32 nGfxROM0Len;
static UINT8 *DrvGfxROM1;
static UINT8 *Drv68KRAM;
static UINT8 *DrvZ80RAM0;
static UINT8 *DrvSprRAM0;
static UINT8 *DrvSprRAM1;
static UINT8 *DrvPalRAM;
static UINT8 *DrvPalRAM2;
static UINT32 *Palette;
static UINT32 *DrvPalette;

static UINT8 DrvRecalc;

static UINT8 DrvJoy1[16];
static UINT8 DrvJoy2[16];
static UINT8 DrvJoy3[16];
static UINT8 DrvJoy4[16];
static UINT8 DrvJoy5[16];
static UINT8 DrvJoy6[16];

static UINT8 DrvDips[3];
static UINT8 DrvReset;

static UINT16 DrvInputs[6];

static INT32 game_select = 0;

static UINT8 soundlatch;
static UINT8 soundlatch2;
static UINT8 soundlatch3;
static UINT8 flipscreen;
static UINT8 color_bank;

static UINT8 bestofbest_prot = 0;
static UINT8 Bssoccera = 0;

static UINT8 z80bankdata[2];

static struct BurnInputInfo BestbestInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 15,	 "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 14,	 "p1 start" },
	{"P1 Up"        , BIT_DIGITAL  , DrvJoy1 + 0,    "p1 up"    },
	{"P1 Down"      , BIT_DIGITAL  , DrvJoy1 + 1,    "p1 down"  },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 2,    "p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 3,    "p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 4,	 "p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy1 + 5,	 "p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy1 + 6,	 "p1 fire 3"},
	{"P1 Button 4"  , BIT_DIGITAL  , DrvJoy1 + 9,	 "p1 fire 4"},
	{"P1 Button 5"  , BIT_DIGITAL  , DrvJoy1 + 10,	 "p1 fire 5"},
	{"P1 Button 6"  , BIT_DIGITAL  , DrvJoy1 + 11,	 "p1 fire 6"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy2 + 15,	 "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 14,	 "p2 start" },
	{"P2 Up"        , BIT_DIGITAL  , DrvJoy2 + 0,    "p2 up"    },
	{"P2 Down"      , BIT_DIGITAL  , DrvJoy2 + 1,    "p2 down"  },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 2,    "p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 3,    "p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 4,	 "p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 5,	 "p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 6,	 "p2 fire 3"},
	{"P2 Button 4"  , BIT_DIGITAL  , DrvJoy2 + 9,	 "p2 fire 4"},
	{"P2 Button 5"  , BIT_DIGITAL  , DrvJoy2 + 10,	 "p2 fire 5"},
	{"P2 Button 6"  , BIT_DIGITAL  , DrvJoy2 + 11,	 "p2 fire 6"},

	{"Service"      , BIT_DIGITAL  , DrvJoy3 + 6,    "service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,			 "reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,		 "dip"	    },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,		 "dip"	    },
};

STDINPUTINFO(Bestbest)

static struct BurnInputInfo UballoonInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 15,	 "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 14,	 "p1 start" },
	{"P1 Up"        , BIT_DIGITAL  , DrvJoy1 + 0,    "p1 up"    },
	{"P1 Down"      , BIT_DIGITAL  , DrvJoy1 + 1,    "p1 down"  },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 2,    "p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 3,    "p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 4,	 "p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy1 + 5,	 "p1 fire 2"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy2 + 15,	 "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 14,	 "p2 start" },
	{"P2 Up"        , BIT_DIGITAL  , DrvJoy2 + 0,    "p2 up"    },
	{"P2 Down"      , BIT_DIGITAL  , DrvJoy2 + 1,    "p2 down"  },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 2,    "p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 3,    "p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 4,	 "p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 5,	 "p2 fire 2"},

	{"Service"      , BIT_DIGITAL  , DrvJoy1 + 12,   "service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,			 "reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,		 "dip"	    },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,		 "dip"	    },
	{"Dip 3",	  BIT_DIPSWITCH, DrvDips + 2,		 "dip"	    },
};

STDINPUTINFO(Uballoon)

static struct BurnInputInfo BssoccerInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy6 + 4,	 "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 7,	 "p1 start" },
	{"P1 Up"        , BIT_DIGITAL  , DrvJoy1 + 0,    "p1 up"    },
	{"P1 Down"      , BIT_DIGITAL  , DrvJoy1 + 1,    "p1 down"  },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 2,    "p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 3,    "p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 4,	 "p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy1 + 5,	 "p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy1 + 6,	 "p1 fire 3"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy6 + 5,	 "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 7,	 "p2 start" },
	{"P2 Up"        , BIT_DIGITAL  , DrvJoy2 + 0,    "p2 up"    },
	{"P2 Down"      , BIT_DIGITAL  , DrvJoy2 + 1,    "p2 down"  },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 2,    "p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 3,    "p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 4,	 "p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 5,	 "p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 6,	 "p2 fire 3"},

	{"P3 Coin"      , BIT_DIGITAL  , DrvJoy6 + 6,	 "p3 coin"  },
	{"P3 Start"     , BIT_DIGITAL  , DrvJoy3 + 7,	 "p3 start" },
	{"P3 Up"        , BIT_DIGITAL  , DrvJoy3 + 0,    "p3 up"    },
	{"P3 Down"      , BIT_DIGITAL  , DrvJoy3 + 1,    "p3 down"  },
	{"P3 Left"      , BIT_DIGITAL  , DrvJoy3 + 2,    "p3 left"  },
	{"P3 Right"     , BIT_DIGITAL  , DrvJoy3 + 3,    "p3 right" },
	{"P3 Button 1"  , BIT_DIGITAL  , DrvJoy3 + 4,	 "p3 fire 1"},
	{"P3 Button 2"  , BIT_DIGITAL  , DrvJoy3 + 5,	 "p3 fire 2"},
	{"P3 Button 3"  , BIT_DIGITAL  , DrvJoy3 + 6,	 "p3 fire 3"},

	{"P4 Coin"      , BIT_DIGITAL  , DrvJoy6 + 7,	 "p4 coin"  },
	{"P4 Start"     , BIT_DIGITAL  , DrvJoy4 + 7,	 "p4 start" },
	{"P4 Up"        , BIT_DIGITAL  , DrvJoy4 + 0,    "p4 up"    },
	{"P4 Down"      , BIT_DIGITAL  , DrvJoy4 + 1,    "p4 down"  },
	{"P4 Left"      , BIT_DIGITAL  , DrvJoy4 + 2,    "p4 left"  },
	{"P4 Right"     , BIT_DIGITAL  , DrvJoy4 + 3,    "p4 right" },
	{"P4 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 4,	 "p4 fire 1"},
	{"P4 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 5,	 "p4 fire 2"},
	{"P4 Button 3"  , BIT_DIGITAL  , DrvJoy4 + 6,	 "p4 fire 3"},

	{"Service"      , BIT_DIGITAL  , DrvJoy3 + 6,    "service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,			 "reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,		 "dip"	    },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,		 "dip"	    },
	{"Dip 3",	  BIT_DIPSWITCH, DrvDips + 2,		 "dip"	    },
};

STDINPUTINFO(Bssoccer)

static struct BurnInputInfo SunaqInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 7,	 "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 6,	 "p1 start" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 0,	 "p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy1 + 1,	 "p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy1 + 2,	 "p1 fire 3"},
	{"P1 Button 4"  , BIT_DIGITAL  , DrvJoy1 + 3,	 "p1 fire 4"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy2 + 7,	 "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 6,	 "p2 start" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 0,	 "p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 1,	 "p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 2,	 "p2 fire 3"},
	{"P2 Button 4"  , BIT_DIGITAL  , DrvJoy2 + 3,	 "p2 fire 4"},

	{"Service"      , BIT_DIGITAL  , DrvJoy3 + 6,    "service"  },

	{"Reset",	      BIT_DIGITAL  , &DrvReset,		 "reset"	},
	{"Dip 1",	      BIT_DIPSWITCH, DrvDips + 0,	 "dip"	    },
};

STDINPUTINFO(Sunaq)

static struct BurnDIPInfo bestbestDIPList[]=
{
	{0x1a, 0xff, 0xff, 0xff, NULL			},
	{0x1b, 0xff, 0xff, 0xff, NULL			},

	{0x1a, 0xfe, 0,       8, "Coinage" 		},
	{0x1a, 0x01, 0x07, 0x00, "5C 1C" 		},
	{0x1a, 0x01, 0x07, 0x01, "4C 1C" 		},
	{0x1a, 0x01, 0x07, 0x02, "3C 1C" 		},
	{0x1a, 0x01, 0x07, 0x03, "2C 1C" 		},
	{0x1a, 0x01, 0x07, 0x07, "1C 1C" 		},
	{0x1a, 0x01, 0x07, 0x06, "1C 2C" 		},
	{0x1a, 0x01, 0x07, 0x05, "1C 3C" 		},
	{0x1a, 0x01, 0x07, 0x04, "1C 4C" 		},

	{0x1a, 0xfe, 0,       4, "Difficulty" 		},
	{0x1a, 0x01, 0x18, 0x18, "Easy" 		},
	{0x1a, 0x01, 0x18, 0x10, "Normal" 		},
	{0x1a, 0x01, 0x18, 0x08, "Hard" },
	{0x1a, 0x01, 0x18, 0x00, "Hardest" 		},

	{0x1a, 0xfe, 0,       2, "Display Combos"  	},
	{0x1a, 0x01, 0x20, 0x00, "Off"			},
	{0x1a, 0x01, 0x20, 0x20, "On"			},

	{0x1a, 0xfe, 0,       2, "Demo Sounds"		},
	{0x1a, 0x01, 0x80, 0x80, "Off" 			},
	{0x1a, 0x01, 0x80, 0x00, "On" 			},

	{0x1b, 0xfe, 0,       2, "Flip Screen" 		},
	{0x1b, 0x01, 0x01, 0x01, "Off" 			},
	{0x1b, 0x01, 0x01, 0x00, "On" 			},

	{0x1b, 0xfe, 0,       4, "Play Time" 		},
	{0x1b, 0x01, 0x06, 0x06, "1:10"  		},
	{0x1b, 0x01, 0x06, 0x04, "1:20"  		},
	{0x1b, 0x01, 0x06, 0x02, "1:30"  		},
	{0x1b, 0x01, 0x06, 0x00, "1:40"  		},
};

STDDIPINFO(bestbest)

static struct BurnDIPInfo bssoccerDIPList[]=
{
	{0x26, 0xff, 0xff, 0xff, NULL },
	{0x27, 0xff, 0xff, 0xff, NULL },
	{0x28, 0xff, 0xff, 0xff, NULL },

	{0x26, 0xfe, 0,       8, "Coinage" },
	{0x26, 0x01, 0x07, 0x00, "4C 1C" },
	{0x26, 0x01, 0x07, 0x01, "3C 1C" },
	{0x26, 0x01, 0x07, 0x02, "2C 1C" },
	{0x26, 0x01, 0x07, 0x07, "1C 1C" },
	{0x26, 0x01, 0x07, 0x06, "1C 2C" },
	{0x26, 0x01, 0x07, 0x05, "1C 3C" },
	{0x26, 0x01, 0x07, 0x04, "1C 4C" },
	{0x26, 0x01, 0x07, 0x03, "1C 5C" },

	{0x26, 0xfe, 0,       4, "Difficulty" },
	{0x26, 0x01, 0x18, 0x10, "Easy" },
	{0x26, 0x01, 0x18, 0x18, "Normal" },
	{0x26, 0x01, 0x18, 0x08, "Hard" },
	{0x26, 0x01, 0x18, 0x00, "Hardest?"   },

	{0x26, 0xfe, 0,       2, "Demo Sounds" },
	{0x26, 0x01, 0x20, 0x00, "Off" },
	{0x26, 0x01, 0x20, 0x20, "On" },

	{0x26, 0xfe, 0,       2, "Flip Screen" },
	{0x26, 0x01, 0x40, 0x40, "Off" },
	{0x26, 0x01, 0x40, 0x00, "On" },

	{0x27, 0xfe, 0,       4, "Play Time P1"  },
	{0x27, 0x01, 0x03, 0x03, "1:30"  },
	{0x27, 0x01, 0x03, 0x02, "1:45"  },
	{0x27, 0x01, 0x03, 0x01, "2:00"  },
	{0x27, 0x01, 0x03, 0x00, "2:15"  },

	{0x27, 0xfe, 0,       4, "Play Time P2"  },
	{0x27, 0x01, 0x0c, 0x0c, "1:30"  },
	{0x27, 0x01, 0x0c, 0x08, "1:45"  },
	{0x27, 0x01, 0x0c, 0x04, "2:00"  },
	{0x27, 0x01, 0x0c, 0x00, "2:15"  },

	{0x27, 0xfe, 0,       4, "Play Time P3"  },
	{0x27, 0x01, 0x30, 0x30, "1:30"  },
	{0x27, 0x01, 0x30, 0x20, "1:45"  },
	{0x27, 0x01, 0x30, 0x10, "2:00"  },
	{0x27, 0x01, 0x30, 0x00, "2:15"  },

	{0x27, 0xfe, 0,       4, "Play Time P4"  },
	{0x27, 0x01, 0xc0, 0xc0, "1:30"  },
	{0x27, 0x01, 0xc0, 0x80, "1:45"  },
	{0x27, 0x01, 0xc0, 0x40, "2:00"  },
	{0x27, 0x01, 0xc0, 0x00, "2:15"  },

	{0x28, 0xfe, 0,       2, "Copyright"  },
	{0x28, 0x01, 0x01, 0x01, "Distributer Unico"  },
	{0x28, 0x01, 0x01, 0x00, "All Rights Reserved"  },
};

STDDIPINFO(bssoccer)


static struct BurnDIPInfo sunaqDIPList[]=
{
	{0x0e, 0xff, 0xff, 0xcf, NULL },

	{0x0e, 0xfe, 0,       8, "Coinage" },
	{0x0e, 0x01, 0x07, 0x00, "5C 1C" },
	{0x0e, 0x01, 0x07, 0x01, "4C 1C" },
	{0x0e, 0x01, 0x07, 0x02, "3C 1C" },
	{0x0e, 0x01, 0x07, 0x03, "2C 1C" },
	{0x0e, 0x01, 0x07, 0x07, "1C 1C" },
	{0x0e, 0x01, 0x07, 0x06, "1C 2C" },
	{0x0e, 0x01, 0x07, 0x05, "1C 3C" },
	{0x0e, 0x01, 0x07, 0x04, "1C 4C" },

	{0x0e, 0xfe, 0,       4, "Difficulty" },
	{0x0e, 0x01, 0x18, 0x00, "Easy" },
	{0x0e, 0x01, 0x18, 0x08, "Normal" },
	{0x0e, 0x01, 0x18, 0x10, "Hard" },
	{0x0e, 0x01, 0x18, 0x18, "Hardest" },

	{0x0e, 0xfe, 0,       2, "Demo Sounds" },
	{0x0e, 0x01, 0x20, 0x20, "Off" },
	{0x0e, 0x01, 0x20, 0x00, "On" },

	{0x0e, 0xfe, 0,       2, "Flip Screen" },
	{0x0e, 0x01, 0x40, 0x40, "Off" },
	{0x0e, 0x01, 0x40, 0x00, "On" },
};

STDDIPINFO(sunaq)


static struct BurnDIPInfo uballoonDIPList[]=
{
	{0x12, 0xff, 0xff, 0xff, NULL },
	{0x13, 0xff, 0xff, 0xff, NULL },
	{0x14, 0xff, 0xff, 0xff, NULL },

	{0x12, 0xfe, 0,       2, "Copyright"  },
	{0x12, 0x01, 0x30, 0x30, "Distributer Unico"  },
	{0x12, 0x01, 0x30, 0x20, "All Rights Reserved"  },
//	{0x12, 0x01, 0x30, 0x10, "Distributer Unico"  },
//	{0x12, 0x01, 0x30, 0x00, "All Rights Reserved"  },

	{0x13, 0xfe, 0,       8, "Coinage" },
	{0x13, 0x01, 0x07, 0x00, "5C 1C" },
	{0x13, 0x01, 0x07, 0x01, "4C 1C" },
	{0x13, 0x01, 0x07, 0x02, "3C 1C" },
	{0x13, 0x01, 0x07, 0x03, "2C 1C" },
	{0x13, 0x01, 0x07, 0x07, "1C 1C" },
	{0x13, 0x01, 0x07, 0x06, "1C 2C" },
	{0x13, 0x01, 0x07, 0x05, "1C 3C" },
	{0x13, 0x01, 0x07, 0x04, "1C 4C" },

	{0x13, 0xfe, 0,       4, "Lives" },
	{0x13, 0x01, 0x18, 0x10, "2"  },
	{0x13, 0x01, 0x18, 0x18, "3"  },
	{0x13, 0x01, 0x18, 0x08, "4"  },
	{0x13, 0x01, 0x18, 0x00, "5"  },

	{0x13, 0xfe, 0,       4, "Difficulty" },
	{0x13, 0x01, 0x60, 0x40, "Easy" },
	{0x13, 0x01, 0x60, 0x60, "Normal" },
	{0x13, 0x01, 0x60, 0x20, "Hard" },
	{0x13, 0x01, 0x60, 0x00, "Hardest" },

	{0x14, 0xfe, 0,       2, "Flip Screen" },
	{0x14, 0x01, 0x01, 0x01, "Off" },
	{0x14, 0x01, 0x01, 0x00, "On" },

	{0x14, 0xfe, 0,       2, "Cabinet" },
	{0x14, 0x01, 0x02, 0x02, "Upright" },
	{0x14, 0x01, 0x02, 0x00, "Cocktail" },

	{0x14, 0xfe, 0,       8, "Bonus Life" },
	{0x14, 0x01, 0x1c, 0x1c, "200K"  },
	{0x14, 0x01, 0x1c, 0x10, "300K, 1000K"  },
	{0x14, 0x01, 0x1c, 0x18, "400K"  },
	{0x14, 0x01, 0x1c, 0x0c, "500K, 1500K"  },
	{0x14, 0x01, 0x1c, 0x08, "500K, 2000K"  },
	{0x14, 0x01, 0x1c, 0x04, "500K, 3000K"  },
	{0x14, 0x01, 0x1c, 0x14, "600K"  },
	{0x14, 0x01, 0x1c, 0x00, "None" },

	{0x14, 0xfe, 0,       2, "Demo Sounds" },
	{0x14, 0x01, 0x80, 0x80, "Off" },
	{0x14, 0x01, 0x80, 0x00, "On" },
};

STDDIPINFO(uballoon)


//-------------------------------------------------------------------------------------------------
// Generic functions

static void suna_palette_write(INT32 offset)
{
	UINT8 r, b, g;
	UINT16 data = BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM + offset)));

	r = (data >>  0) & 0x1f;
	r = (r << 3) | (r >> 2);

	g = (data >>  5) & 0x1f;
	g = (g << 3) | (g >> 2);

	b = (data >> 10) & 0x1f;
	b = (b << 3) | (b >> 2);

	Palette[offset>>1] = (r << 16) | (g << 8) | b;
	DrvPalette[offset>>1] = BurnHighCol(r, g, b, 0);

	return;
}

//-------------------------------------------------------------------------------------------------
// Memory handlers


//----------------------------------------------------------------
// Best of Best


//------------------
// 68k

static UINT16 __fastcall bestbest_read_word(UINT32 address)
{
	switch (address & ~1)
	{
		case 0x500000:
			return DrvInputs[0];

		case 0x500002:
			return DrvInputs[1];

		case 0x500004:
			return DrvInputs[2];
	}

	return 0;
}

static UINT8 __fastcall bestbest_read_byte(UINT32 address)
{
	switch (address)
	{
		case 0x500000:
		case 0x500001:
			return DrvInputs[0] >> ((~address & 1) << 3);

		case 0x500002:
		case 0x500003:
			return DrvInputs[1] >> ((~address & 1) << 3);

		case 0x500004:
		case 0x500005:
			return DrvInputs[2] >> ((~address & 1) << 3);

		case 0x500019:
			return bestofbest_prot;
	}

	return 0;
}

static void __fastcall bestbest_write_word(UINT32 address, UINT16 data)
{
	if ((address & 0xfff000) == 0x540000) {
		*((UINT16*)(DrvPalRAM + (address & 0x0fff))) = BURN_ENDIAN_SWAP_INT16(data);
		suna_palette_write(address & 0xffe);
		return;
	}

	switch (address & ~1)
	{
		case 0x500000:
			soundlatch = data;
		return;

		case 0x500002:
			flipscreen = data & 0x10;
		return;
	}

	return;
}

static void __fastcall bestbest_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xfff000) == 0x540000) {
		DrvPalRAM[address & 0xfff] = data;
		suna_palette_write(address & 0xffe);
		return;
	}

	switch (address)
	{
		case 0x500000:
		case 0x500001:
			soundlatch = data;
		return;

		case 0x500002:
		case 0x500003:
			flipscreen = data & 0x10;
		return;

		case 0x500008:
		case 0x500009:
			switch (data & 0xff) {
				case 0x00:	bestofbest_prot ^= 0x09;	break;
				case 0x08:	bestofbest_prot ^= 0x02;	break;
				case 0x0c:	bestofbest_prot ^= 0x03;	break;
			}
		return;
	}

	return;
}

//------------------
// Z80 #0

static void __fastcall bestbest_sound0_write(UINT16 address, UINT8 data)
{
	switch (address)
	{
		case 0xc000:
		case 0xc001:
			BurnYM3526Write(address & 1, data);
		return;

		case 0xc002:
		case 0xc003:
			AY8910Write(0, address & 1, data);
		return;

		case 0xf000:
			soundlatch2 = data;
		return;
	}

	return;
}

static UINT8 __fastcall bestbest_sound0_read(UINT16 address)
{
	switch (address)
	{
		case 0xf800:
			return soundlatch;
	}

	return 0;
}

//------------------
// Z80 #1

static void __fastcall bestbest_sound1_out(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x00: { DACSignedWrite(0, (data & 0xf) * 0x11); return; }
		case 0x01: { DACSignedWrite(1, (data & 0xf) * 0x11); return; }

		case 0x02: { DACSignedWrite(2, (data & 0xf) * 0x11); return; }
		case 0x03: { DACSignedWrite(3, (data & 0xf) * 0x11); return; }
	}

	return;
}

static UINT8 __fastcall bestbest_sound1_in(UINT16 port)
{
	switch (port & 0xff)
	{
		case 0x00:
			return soundlatch2;
	}

	return 0;
}


//----------------------------------------------------------------
// SunA Quiz


//------------------
// 68k

static UINT16 __fastcall sunaq_read_word(UINT32 address)
{
	if ((address & 0xfff000) == 0x540000) {
		if (address & 0x200) {
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM2 + (address & 0xffe))));
		} else {
			address += color_bank << 9;
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM + (address & 0xffe))));
		}
	}

	switch (address & ~1)
	{
		case 0x500000:
			return DrvInputs[0];

		case 0x500002:
			return DrvInputs[1];

		case 0x500004:
			return DrvInputs[2];

		case 0x500006:
			return DrvInputs[3];
	}

	return 0;
}

static UINT8 __fastcall sunaq_read_byte(UINT32 address)
{
	if ((address & 0xfff000) == 0x540000) {
		if (address & 0x200) {
			return DrvPalRAM2[address & 0xffe];
		} else {
			address += color_bank << 9;
			return DrvPalRAM[address & 0xffe];
		}
	}

	switch (address)
	{
		case 0x500000:
		case 0x500001:
			return DrvInputs[0] >> ((~address & 1) << 3);

		case 0x500002:
		case 0x500003:
			return DrvInputs[1] >> ((~address & 1) << 3);

		case 0x500004:
		case 0x500005:
			return DrvInputs[2] >> ((~address & 1) << 3);

		case 0x500006:
		case 0x500007:
			return DrvInputs[3] >> ((~address & 1) << 3);
	}

	return 0;
}

static void __fastcall sunaq_write_word(UINT32 address, UINT16 data)
{
	if ((address & 0xfff000) == 0x540000) {
		if (address & 0x200) {
			*((UINT16*)(DrvPalRAM2 + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
		} else {
			address += color_bank << 9;
			*((UINT16*)(DrvPalRAM + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
			suna_palette_write(address & 0xffff);
		}
		return;
	}

	switch (address & ~1)
	{
		case 0x500000:
			soundlatch = data;
		return;

		case 0x500002:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;

		case 0x500004:
			// coin counter
		return;
	}
	return;
}

static void __fastcall sunaq_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xfff000) == 0x540000) {
		if (address & 0x200) {
			DrvPalRAM2[address & 0xfff] = data;
		} else {
			address += color_bank << 9;
			DrvPalRAM[address & 0xfff] = data;
			suna_palette_write(address & 0xffe);
		}

		return;
	}

	switch (address)
	{
		case 0x500000:
		case 0x500001:
			soundlatch = data;
		return;

		case 0x500002:
		case 0x500003:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;

		case 0x500004:
		case 0x500005:
			// coin counter
		return;
	}
	return;
}

//------------------
// Z80 #0

static void __fastcall sunaq_sound0_write(UINT16 address, UINT8 data)
{
	switch (address)
	{
		case 0xf800:
			BurnYM2151SelectRegister(data);
		return;

		case 0xf801:
			BurnYM2151WriteRegister(data);
		return;

		case 0xfc00:
			soundlatch2 = data;
		return;
	}

	return;
}


//----------------------------------------------------------------
// Ultra Balloon


//------------------
// 68k

static UINT8 uballoon_prot_read(UINT16 offset)
{
	UINT8 ret = 0;

	switch (offset)
	{
		case 0x0011:
			ret = ((bestofbest_prot & 0x03) == 0x03) ? 2 : 0;
			ret |= ((bestofbest_prot & 0x30) == 0x30) ? 1 : 0;
		break;

		case 0x0311:
			ret = 0x03;
		break;

		default:
	//		bprintf (0, _T("uballoon_prot_read %04X\n"), offset);
		break;
	}

	return ret;
}

static void uballoon_prot_write(UINT16 offset, UINT8 data)
{
	switch (offset)
	{
		case 0x0001:
			bestofbest_prot = data;
		break;

		default:
	//		bprintf (0, _T("uballoon_prot_write %04X=%02X\n"), offset, data);
		break;
	}
}

static UINT16 __fastcall uballoon_read_word(UINT32 address)
{
	if ((address & 0xfff000) == 0x200000) {
		if (address & 0x200) {
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM2 + (address & 0xffe))));
		} else {
			address += color_bank << 9;
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM + (address & 0xffe))));
		}
	}

	switch (address & ~1)
	{
		case 0x600000:
			return DrvInputs[0];

		case 0x600002:
			return DrvInputs[1];

		case 0x600004:
			return DrvInputs[2];

		case 0x600006:
			return DrvInputs[3];
	}

	return 0;
}

static UINT8 __fastcall uballoon_read_byte(UINT32 address)
{
	if ((address & 0xff0000) == 0xa00000) {
		return uballoon_prot_read(address);
	}

	if ((address & 0xfff000) == 0x200000) {
		if (address & 0x200) {
			return DrvPalRAM2[address & 0xffe];
		} else {
			address += color_bank << 9;
			return DrvPalRAM[address & 0xffe];
		}
	}

	switch (address)
	{
		case 0x600000:
		case 0x600001:
			return DrvInputs[0] >> ((~address & 1) << 3);

		case 0x600002:
		case 0x600003:
			return DrvInputs[1] >> ((~address & 1) << 3);

		case 0x600004:
		case 0x600005:
			return DrvInputs[2] >> ((~address & 1) << 3);

		case 0x600006:
		case 0x600007:
			return DrvInputs[3] >> ((~address & 1) << 3);
	}

	return 0;
}

static void __fastcall uballoon_write_word(UINT32 address, UINT16 data)
{
	if ((address & 0xfff000) == 0x200000) {
		if (address & 0x200) {
			*((UINT16*)(DrvPalRAM2 + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
		} else {
			address += color_bank << 9;
			*((UINT16*)(DrvPalRAM + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
			suna_palette_write(address & 0xffff);
		}
		return;
	}

	switch (address & ~1)
	{
		case 0x600000:
			soundlatch = data;
		return;

		case 0x600004:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;
	}
	return;
}

static void __fastcall uballoon_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xfff000) == 0x200000) {
		if (address & 0x200) {
			DrvPalRAM2[address & 0xfff] = data;
		} else {
			address += color_bank << 9;
			DrvPalRAM[address & 0xfff] = data;
			suna_palette_write(address & 0xffe);
		}
		return;
	}

	if ((address & 0xff0000) == 0xa00000) {
		uballoon_prot_write(address, data);
		return;
	}

	switch (address)
	{
		case 0x600000:
		case 0x600001:
			soundlatch = data;
		return;

		case 0x600004:
		case 0x600005:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;
	}
	return;
}

//------------------
// Z80 #1

static void uballoon_bankswitch(INT32 data)
{
	z80bankdata[0] = data;

	INT32 bank = ((data & 1) << 16) | 0x400;

	ZetMapMemory(DrvZ80ROM1 + bank, 0x0400, 0xffff, MAP_ROM);
}

static void __fastcall uballoon_sound1_out(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x00: { DACSignedWrite(0, (data & 0xf) * 0x11); return; }
		case 0x01: { DACSignedWrite(1, (data & 0xf) * 0x11); return; }

		case 0x03:
			uballoon_bankswitch(data);
		return;
	}

	return;
}

static UINT8 __fastcall uballoon_sound1_in(UINT16 port)
{
	switch (port & 0xff)
	{
		case 0x00:
			return soundlatch2;
	}

	return 0;
}


//----------------------------------------------------------------
// Back Street Soccer


//------------------
// 68k

static UINT16 __fastcall bssoccer_read_word(UINT32 address)
{
	if ((address & 0xfff000) == 0x400000) {
		if (address & 0x200) {
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM2 + (address & 0xffe))));
		} else {
			address += color_bank << 9;
			return BURN_ENDIAN_SWAP_INT16(*((UINT16*)(DrvPalRAM + (address & 0xffe))));
		}
	}

	switch (address & ~1)
	{
		case 0xa00000:
			return DrvInputs[0];

		case 0xa00002:
			return DrvInputs[1];

		case 0xa00004:
			return DrvInputs[2];

		case 0xa00006:
			return DrvInputs[3];

		case 0xa00008:
			return DrvInputs[4];

		case 0xa0000a:
			return DrvInputs[5];

	}

	return 0;
}

static UINT8 __fastcall bssoccer_read_byte(UINT32 address)
{
	if ((address & 0xfff000) == 0x400000) {
		if (address & 0x200) {
			return DrvPalRAM2[address & 0xffe];
		} else {
			address += color_bank << 9;
			return DrvPalRAM[address & 0xffe];
		}
	}

	switch (address)
	{
		case 0xa00000:
		case 0xa00001:
			return DrvInputs[0] >> ((~address & 1) << 3);

		case 0xa00002:
		case 0xa00003:
			return DrvInputs[1] >> ((~address & 1) << 3);

		case 0xa00004:
		case 0xa00005:
			return DrvInputs[2] >> ((~address & 1) << 3);

		case 0xa00006:
		case 0xa00007:
			return DrvInputs[3] >> ((~address & 1) << 3);

		case 0xa00008:
		case 0xa00009:
			return DrvInputs[4] >> ((~address & 1) << 3);

		case 0xa0000a:
		case 0xa0000b:
			return DrvInputs[5] >> ((~address & 1) << 3);
	}

	return 0;
}

static void __fastcall bssoccer_write_word(UINT32 address, UINT16 data)
{
	if ((address & 0xfff000) == 0x400000) {
		if (address & 0x200) {
			*((UINT16*)(DrvPalRAM2 + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
		} else {
			address += color_bank << 9;
			*((UINT16*)(DrvPalRAM + (address & 0xffff))) = BURN_ENDIAN_SWAP_INT16(data);
			suna_palette_write(address & 0xffff);
		}
		return;
	}

	switch (address & ~1)
	{
		case 0xa00000:
			soundlatch = data;
		return;

		case 0xa00002:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;
	}
	return;
}

static void __fastcall bssoccer_write_byte(UINT32 address, UINT8 data)
{
	if ((address & 0xfff000) == 0x400000) {
		if (address & 0x200) {
			DrvPalRAM[address & 0xfff] = data;
		} else {
			address += color_bank << 9;
			DrvPalRAM[address & 0xfff] = data;
			suna_palette_write(address & 0xffe);
		}
		return;
	}

	switch (address)
	{
		case 0xa00000:
		case 0xa00001:
			soundlatch = data;
		return;

		case 0xa00002:
		case 0xa00003:
			flipscreen = data & 0x01;
			color_bank = (data >> 2) & 1;
		return;
	}
	return;
}

//------------------
// Z80 #0

static void bssoccer_bankswitch_w(UINT8 *z80data, INT32 p, INT32 data)
{
	z80bankdata[p] = data;

	INT32 bank = ((data & 7) << 16) | 0x1000;

	ZetMapMemory(z80data + bank, 0x1000, 0xffff, MAP_ROM);
}

static void __fastcall bssoccer_sound0_write(UINT16 address, UINT8 data)
{
	switch (address)
	{
		case 0xf800:
			BurnYM2151SelectRegister(data);
		return;

		case 0xf801:
			BurnYM2151WriteRegister(data);
		return;

		case 0xfd00:
			soundlatch2 = data;
		return;

		case 0xfe00:
			soundlatch3 = data;
		return;
	}

	return;
}

static UINT8 __fastcall bssoccer_sound0_read(UINT16 address)
{
	switch (address)
	{
		case 0xf801:
			return BurnYM2151Read();

		case 0xfc00:
			return soundlatch;
	}

	return 0;
}

//------------------
// Z80 #1

static void __fastcall bssoccer_sound1_out(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x00: { DACSignedWrite(0, (data & 0xf) * 0x11); return; }
		case 0x01: { DACSignedWrite(1, (data & 0xf) * 0x11); return; }

		case 0x03:
			bssoccer_bankswitch_w(DrvZ80ROM1, 0, data);
		return;
	}

	return;
}

static UINT8 __fastcall bssoccer_sound1_in(UINT16 port)
{
	switch (port & 0xff)
	{
		case 0x00:
			return soundlatch2;
	}

	return 0;
}

//------------------
// Z80 #2

static void __fastcall bssoccer_sound2_out(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x00: { DACSignedWrite(2, (data & 0xf) * 0x11); return; }
		case 0x01: { DACSignedWrite(3, (data & 0xf) * 0x11); return; }
		return;

		case 0x03:
			bssoccer_bankswitch_w(DrvZ80ROM2, 1, data);
		return;
	}

	return;
}

static UINT8 __fastcall bssoccer_sound2_in(UINT16 port)
{
	switch (port & 0xff)
	{
		case 0x00:
			return soundlatch3;
	}

	return 0;
}


//-------------------------------------------------------------------------------------------------
// Initialization routines

static INT32 DrvDoReset()
{
	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	for (INT32 j = 0; j < 2; j++) {
		ZetOpen(j);
		ZetReset();
		ZetClose();
	}

	if (game_select == 3) {
		ZetOpen(2);
		ZetReset();
		ZetClose();
	}

	soundlatch = 0;
	soundlatch2 = 0;
	soundlatch3 = 0;
	color_bank = 0;
	bestofbest_prot = 0;
	flipscreen = 0;

	z80bankdata[0] = z80bankdata[1] = 0;

	if (game_select == 3) {
		ZetOpen(1);
		bssoccer_bankswitch_w(DrvZ80ROM1, 0, z80bankdata[0]);
		ZetClose();
		ZetOpen(2);
		bssoccer_bankswitch_w(DrvZ80ROM2, 1, z80bankdata[1]);
		ZetClose();
	}

	if (game_select == 2) {
		ZetOpen(1);
		uballoon_bankswitch(z80bankdata[0]);
		ZetClose();
	}

	if (game_select == 1) {
		ZetOpen(1);
		bssoccer_bankswitch_w(DrvZ80ROM1, 0, z80bankdata[0]);
		ZetClose();
	}

	if (game_select) {
		BurnYM2151Reset();
	} else {
		BurnYM3526Reset();
		AY8910Reset(0);
	}

	DACReset();

	HiscoreReset();

	return 0;
}


static INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x0200000;
	DrvZ80ROM0	= Next; Next += 0x0010000;
	DrvZ80ROM1	= Next; Next += 0x0080000;
	DrvZ80ROM2	= Next; Next += 0x0080000;

	DrvGfxROM0	= Next; Next += 0x0600000;
	if (game_select == 0) {
		DrvGfxROM1	= Next; Next += 0x0800000;
	}

	DrvPalette	= (UINT32*)Next; Next += 0x01000 * sizeof(UINT32);

	AllRam		= Next;

	Drv68KRAM	= Next; Next += 0x0010000;

//	DrvZ80RAM0	= Next; Next += 0x0000800;
	DrvZ80RAM0	= Next; Next += 0x0001000;

	DrvSprRAM0	= Next; Next += 0x0020000;
	DrvSprRAM1	= Next; Next += 0x0020000;

	DrvPalRAM	= Next; Next += 0x0001000;
	DrvPalRAM2	= Next; Next += 0x0010000;

	Palette		= (UINT32*)Next; Next += 0x01000 * sizeof(UINT32);

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static INT32 DrvGfxDecode(UINT8 *gfx_base, INT32 len)
{
	INT32 Plane[4] = {(len << 2) + 0, (len << 2) + 4, 0, 4 };
	INT32 XOffs[8] = {  3,  2,  1,  0, 11, 10,  9,  8 };
	INT32 YOffs[8] = {  0, 16, 32, 48, 64, 80, 96, 112 };

	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) {
		return 1;
	}

	for (INT32 i = 0; i < len; i++) tmp[i] = gfx_base[i] ^ 0xff; // copy & invert

	GfxDecode(((len * 8) / 4) / 64, 4, 8, 8, Plane, XOffs, YOffs, 0x80, tmp, gfx_base);

	BurnFree (tmp);

	return 0;
}

static INT32 DrvLoadRoms()
{
	char* pRomName;
	struct BurnRomInfo ri;

	UINT8 *Load68K = Drv68KROM;
	UINT8 *Loadz0  = DrvZ80ROM0;
	UINT8 *Loadz1  = DrvZ80ROM1;
	UINT8 *Loadz2  = DrvZ80ROM2;
	UINT8 *Loadg0  = DrvGfxROM0;
	UINT8 *Loadg1  = DrvGfxROM1;

	INT32 gfx0_len = 0;
	INT32 gfx1_len = 0;

	for (INT32 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {

		BurnDrvGetRomInfo(&ri, i);

		if ((ri.nType & 7) == 1) {

			if (Bssoccera) {
				if (BurnLoadRom(Load68K + 0, i + 0, 0)) return 1;

				Load68K += 0x200000;
			} else {
				if (BurnLoadRom(Load68K + 1, i + 0, 2)) return 1;
				if (BurnLoadRom(Load68K + 0, i + 1, 2)) return 1;

				Load68K += 0x100000;
			}

			i++;

			continue;
		}

		if ((ri.nType & 7) == 2) {
			if (BurnLoadRom(Loadz0, i, 1)) return 1;
			continue;
		}

		if ((ri.nType & 7) == 3) {
			if (BurnLoadRom(Loadz1, i, 1)) return 1;
			continue;
		}

		if ((ri.nType & 7) == 4) {
			if (BurnLoadRom(Loadz2, i, 1)) return 1;
			continue;
		}

		if ((ri.nType & 7) == 5) {
			if (BurnLoadRom(Loadg0, i, 1)) return 1;

			Loadg0 += ri.nLen;
			gfx0_len += ri.nLen;

			continue;
		}

		if ((ri.nType & 7) == 6) {
			if (BurnLoadRom(Loadg1, i, 1)) return 1;

			Loadg1 += ri.nLen;
			gfx1_len += ri.nLen;

			continue;
		}
	}

	if (Bssoccera) {
		UINT8* Temp = (UINT8*)BurnMalloc(0x300000);
		for (INT32 i = 0; i < 0x180000; i++) {
			Temp[i] = DrvGfxROM0[i*2];
		}
		for (INT32 i = 0x180000; i < 0x300000; i++) {
			Temp[i] = DrvGfxROM0[((i-0x180000)*2)+1];
		}
		memcpy (DrvGfxROM0, Temp, 0x300000);
		BurnFree(Temp);
	}

	nGfxROM0Len = gfx0_len >> 5;

	if (gfx0_len) DrvGfxDecode(DrvGfxROM0, gfx0_len);
	if (gfx1_len) DrvGfxDecode(DrvGfxROM1, gfx1_len);

	return 0;
}

static void bestbest_ay8910_write_a(UINT32,UINT32)
{
}

static void bestbestFMIRQHandler(INT32, INT32 nStatus)
{
	ZetSetIRQLine(0, (nStatus) ? CPU_IRQSTATUS_ACK : CPU_IRQSTATUS_NONE);
}

static INT32 BestbestInit()
{
	game_select = 0;

	BurnAllocMemIndex();

	if (DrvLoadRoms()) return 1;

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		   0x000000, 0x03ffff, MAP_ROM);
	SekMapMemory(Drv68KROM,		   0x040000, 0x07ffff, MAP_ROM);
	SekMapMemory(Drv68KROM,		   0x080000, 0x0bffff, MAP_ROM);
	SekMapMemory(Drv68KROM,		   0x0c0000, 0x0fffff, MAP_ROM);
	SekMapMemory(Drv68KROM + 0x100000, 0x200000, 0x2fffff, MAP_ROM);
	SekMapMemory(DrvPalRAM, 	   0x540000, 0x540fff, MAP_ROM);
	SekMapMemory(DrvPalRAM2,	   0x541000, 0x54ffff, MAP_RAM);
	SekMapMemory(Drv68KRAM,		   0x580000, 0x58ffff, MAP_RAM);
	SekMapMemory(DrvSprRAM0,	   0x5c0000, 0x5dffff, MAP_RAM);
	SekMapMemory(DrvSprRAM1,	   0x5e0000, 0x5fffff, MAP_RAM);
	SekSetWriteByteHandler(0,	   bestbest_write_byte);
	SekSetWriteWordHandler(0,	   bestbest_write_word);
	SekSetReadByteHandler(0,	   bestbest_read_byte);
	SekSetReadWordHandler(0,	   bestbest_read_word);
	SekClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM0, 0x0000, 0xbfff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM0, 0xe000, 0xe7ff, MAP_RAM);
	ZetSetReadHandler(bestbest_sound0_read);
	ZetSetWriteHandler(bestbest_sound0_write);
	ZetClose();

	ZetInit(1);
	ZetOpen(1);
	ZetMapMemory(DrvZ80ROM1, 0x0000, 0xffff, MAP_ROM);
	ZetSetInHandler(bestbest_sound1_in);
	ZetSetOutHandler(bestbest_sound1_out);
	ZetClose();

	BurnYM3526Init(3000000, &bestbestFMIRQHandler, 1);
	BurnTimerAttach(&ZetConfig, 6000000);
	BurnYM3526SetRoute(BURN_SND_YM3526_ROUTE, 1.00, BURN_SND_ROUTE_BOTH);

	AY8910Init(0, 1500000, 0);
	AY8910SetPorts(0, NULL, NULL, bestbest_ay8910_write_a, NULL);
	AY8910SetRoute(0, BURN_SND_AY8910_ROUTE_1, 1.00, BURN_SND_ROUTE_LEFT);
	AY8910SetRoute(0, BURN_SND_AY8910_ROUTE_2, 1.00, BURN_SND_ROUTE_RIGHT);
	AY8910SetRoute(0, BURN_SND_AY8910_ROUTE_3, 0.00, BURN_SND_ROUTE_BOTH); // suppressed?

	DACInit(0, 0, 1, ZetTotalCycles, 6000000);
	DACInit(1, 0, 1, ZetTotalCycles, 6000000);
	DACInit(2, 0, 1, ZetTotalCycles, 6000000);
	DACInit(3, 0, 1, ZetTotalCycles, 6000000);
	DACSetRoute(0, 0.40, BURN_SND_ROUTE_LEFT);
	DACSetRoute(1, 0.40, BURN_SND_ROUTE_RIGHT);
	DACSetRoute(2, 0.40, BURN_SND_ROUTE_LEFT);
	DACSetRoute(3, 0.40, BURN_SND_ROUTE_RIGHT);

	DrvDoReset();

	GenericTilesInit();

	return 0;
}

static INT32 SunaqInit()
{
	game_select = 1;

	BurnAllocMemIndex();

	if (DrvLoadRoms()) return 1;

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		   0x000000, 0x0fffff, MAP_ROM);
	SekMapMemory(DrvPalRAM2,	   0x540400, 0x540fff, MAP_RAM);
	SekMapMemory(Drv68KRAM,		   0x580000, 0x583fff, MAP_RAM);
	SekMapMemory(DrvSprRAM0,	   0x5c0000, 0x5dffff, MAP_RAM);
	SekSetWriteByteHandler(0,	   sunaq_write_byte);
	SekSetWriteWordHandler(0,	   sunaq_write_word);
	SekSetReadByteHandler(0,	   sunaq_read_byte);
	SekSetReadWordHandler(0,	   sunaq_read_word);
	SekClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM0, 0x0000, 0xefff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM0, 0xf000, 0xf7ff, MAP_RAM);
	ZetSetWriteHandler(sunaq_sound0_write);
	ZetSetReadHandler(bssoccer_sound0_read);
	ZetClose();

	ZetInit(1);
	ZetOpen(1);
	ZetMapMemory(DrvZ80ROM1, 0x0000, 0xffff, MAP_ROM);
	ZetSetInHandler(bssoccer_sound1_in);
	ZetSetOutHandler(bssoccer_sound1_out);
	ZetClose();

	BurnYM2151InitBuffered(3579545, 1, NULL, 0);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_1, 0.50, BURN_SND_ROUTE_LEFT);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_2, 0.50, BURN_SND_ROUTE_RIGHT);
	BurnTimerAttachZet(3579500);

	DACInit(0, 0, 2, ZetTotalCycles, 6000000);
	DACInit(1, 0, 2, ZetTotalCycles, 6000000);
	DACSetRoute(0, 0.50, BURN_SND_ROUTE_LEFT);
	DACSetRoute(1, 0.50, BURN_SND_ROUTE_RIGHT);

	DrvDoReset();

	GenericTilesInit();

	return 0;
}


static INT32 UballoonInit()
{
	game_select = 2;

	BurnAllocMemIndex();

	if (DrvLoadRoms()) return 1;

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		   0x000000, 0x0fffff, MAP_ROM);
	SekMapMemory(DrvPalRAM2,	   0x200400, 0x200fff, MAP_RAM);
	SekMapMemory(DrvSprRAM0,	   0x400000, 0x41ffff, MAP_RAM);
	SekMapMemory(DrvSprRAM0,	   0x5c0000, 0x5dffff, MAP_RAM);
	SekMapMemory(Drv68KRAM,	   	   0x800000, 0x803fff, MAP_RAM);
	SekSetWriteByteHandler(0,	   uballoon_write_byte);
	SekSetWriteWordHandler(0,	   uballoon_write_word);
	SekSetReadByteHandler(0,	   uballoon_read_byte);
	SekSetReadWordHandler(0,	   uballoon_read_word);
	SekClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM0, 0x0000, 0xefff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM0, 0xf000, 0xf7ff, MAP_RAM);
	ZetSetWriteHandler(sunaq_sound0_write);
	ZetSetReadHandler(bssoccer_sound0_read);
	ZetClose();

	ZetInit(1);
	ZetOpen(1);
	ZetMapMemory(DrvZ80ROM1, 0x0000, 0xffff, MAP_ROM);
	ZetSetInHandler(uballoon_sound1_in);
	ZetSetOutHandler(uballoon_sound1_out);
	ZetClose();

	BurnYM2151InitBuffered(3579545, 1, NULL, 0);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_1, 0.50, BURN_SND_ROUTE_LEFT);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_2, 0.50, BURN_SND_ROUTE_RIGHT);
	BurnTimerAttachZet(3579500);

	DACInit(0, 0, 1, ZetTotalCycles, 5333333);
	DACInit(1, 0, 1, ZetTotalCycles, 5333333);
	DACSetRoute(0, 0.50, BURN_SND_ROUTE_LEFT);
	DACSetRoute(1, 0.50, BURN_SND_ROUTE_RIGHT);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static INT32 BssoccerInit()
{
	game_select = 3;

	BurnAllocMemIndex();

	if (DrvLoadRoms()) return 1;

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		   0x000000, 0x1fffff, MAP_ROM);
	SekMapMemory(Drv68KRAM,		   0x200000, 0x203fff, MAP_RAM);
	SekMapMemory(DrvPalRAM2,	   0x400400, 0x400fff, MAP_RAM);
	SekMapMemory(DrvSprRAM0,	   0x600000, 0x61ffff, MAP_RAM);
	SekSetWriteByteHandler(0,	   bssoccer_write_byte);
	SekSetWriteWordHandler(0,	   bssoccer_write_word);
	SekSetReadByteHandler(0,	   bssoccer_read_byte);
	SekSetReadWordHandler(0,	   bssoccer_read_word);
	SekClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapMemory(DrvZ80ROM0, 0x0000, 0x7fff, MAP_ROM);
	ZetMapMemory(DrvZ80RAM0, 0xf000, 0xf7ff, MAP_RAM);
	ZetSetWriteHandler(bssoccer_sound0_write);
	ZetSetReadHandler(bssoccer_sound0_read);
	ZetClose();

	ZetInit(1);
	ZetOpen(1);
	ZetMapMemory(DrvZ80ROM1, 0x0000, 0xffff, MAP_ROM);
    ZetSetInHandler(bssoccer_sound1_in);
	ZetSetOutHandler(bssoccer_sound1_out);
	ZetClose();

	ZetInit(2);
	ZetOpen(2);
	ZetMapMemory(DrvZ80ROM2, 0x0000, 0xffff, MAP_ROM);
	ZetSetInHandler(bssoccer_sound2_in);
	ZetSetOutHandler(bssoccer_sound2_out);
	ZetClose();

	BurnYM2151InitBuffered(3579545, 1, NULL, 0);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_1, 0.20, BURN_SND_ROUTE_LEFT);
	BurnYM2151SetRoute(BURN_SND_YM2151_YM2151_ROUTE_2, 0.20, BURN_SND_ROUTE_RIGHT);
	BurnTimerAttachZet(3579500);

	DACInit(0, 0, 1, ZetTotalCycles, 5333333);
	DACInit(1, 0, 1, ZetTotalCycles, 5333333);
	DACInit(2, 0, 1, ZetTotalCycles, 5333333);
	DACInit(3, 0, 1, ZetTotalCycles, 5333333);
	DACSetRoute(0, 0.40, BURN_SND_ROUTE_BOTH);
	DACSetRoute(1, 0.40, BURN_SND_ROUTE_BOTH);
	DACSetRoute(2, 0.40, BURN_SND_ROUTE_LEFT);
	DACSetRoute(3, 0.40, BURN_SND_ROUTE_RIGHT);

	DrvDoReset();

	GenericTilesInit();

	return 0;
}

static INT32 DrvExit()
{
	BurnFreeMemIndex();

	if (game_select) {
		BurnYM2151Exit();
	} else {
		AY8910Exit(0);
		BurnYM3526Exit();
	}

	DACExit();

	SekExit();
	ZetExit();

	GenericTilesExit();

	Bssoccera = 0;

	return 0;
}


//-------------------------------------------------------------------------------------------------
// Drawing & CPU/sound emulation routines


static void draw_sprites(UINT16 *sprites, UINT8 *gfx_base, INT32 max_tile)
{
	INT32 offs;

	INT32 max_x = (256 - 0) - 8;
	INT32 max_y = (256 - 0) - 8;

	for (offs = 0xfc00/2; offs < 0x10000/2 ; offs += 4/2)
	{
		INT32 srcpg, srcx,srcy, dimx,dimy;
		INT32 tile_x, tile_xinc, tile_xstart;
		INT32 tile_y, tile_yinc;
		INT32 dx, dy;
		INT32 flipx, y0;

		INT32 y		=	BURN_ENDIAN_SWAP_INT16(sprites[ offs + 0 + 0x00000 / 2 ]);
		INT32 x		=	BURN_ENDIAN_SWAP_INT16(sprites[ offs + 1 + 0x00000 / 2 ]);
		INT32 dim 	=	BURN_ENDIAN_SWAP_INT16(sprites[ offs + 0 + 0x10000 / 2 ]);

		INT32 bank	=	(x >> 12) & 0xf;

		srcpg	=	((y & 0xf000) >> 12) + ((x & 0x0200) >> 5);
		srcx	=	((y   >> 8) & 0xf) * 2;
		srcy	=	((dim >> 0) & 0xf) * 2;

		switch ( (dim >> 4) & 0xc )
		{
			case 0x0:	dimx = 2;	dimy =	2;	y0 = 0x100; break;
			case 0x4:	dimx = 4;	dimy =	4;	y0 = 0x100; break;
			case 0x8:	dimx = 2;	dimy = 32;	y0 = 0x130; break;
			default:
			case 0xc:	dimx = 4;	dimy = 32;	y0 = 0x120; break;
		}

		if (dimx==4)	{ flipx = srcx & 2; 	srcx &= ~2; }
		else		{ flipx = 0; }

		x = (x & 0xff) - (x & 0x100);
		y = (y0 - (y & 0xff) - dimy*8 ) & 0xff;

		if (flipx)	{ tile_xstart = dimx-1; tile_xinc = -1; }
		else		{ tile_xstart = 0;		tile_xinc = +1; }

		tile_y = 0; 	tile_yinc = +1;

		for (dy = 0; dy < dimy * 8; dy += 8)
		{
			tile_x = tile_xstart;

			for (dx = 0; dx < dimx * 8; dx += 8)
			{
				INT32 addr	=	(srcpg * 0x20 * 0x20) +
								((srcx + tile_x) & 0x1f) * 0x20 +
								((srcy + tile_y) & 0x1f);

				INT32 tile	=	BURN_ENDIAN_SWAP_INT16(sprites[ addr + 0x00000 / 2 ]);
				INT32 color	=	BURN_ENDIAN_SWAP_INT16(sprites[ addr + 0x10000 / 2 ]);

				INT32 sx		=	x + dx;
				INT32 sy		=	(y + dy) & 0xff;

				INT32 tile_flipx	=	tile & 0x4000;
				INT32 tile_flipy	=	tile & 0x8000;

				if (flipx)	tile_flipx ^= 0x4000;

				if (flipscreen)
				{
					sx = max_x - sx;
					sy = max_y - sy;
					tile_flipx ^= 0x4000;
					tile_flipy ^= 0x8000;
				}

				tile   = (tile & 0x3fff) | (bank << 14);
				color += (color_bank << 4);
				color &= 0x7f;
				tile  %= max_tile;

				sy -= 16;

				tile_x += tile_xinc;

				if (sy < -15 || sy > (nScreenHeight - 1) || sx < -15 || sx > (nScreenWidth - 1)) {
					continue;
				}

				Draw8x8MaskTile(pTransDraw, tile, sx, sy, tile_flipx, tile_flipy, color, 4, 0x0f, 0, gfx_base);
			}

			tile_y += tile_yinc;
		}

	}

}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		for (INT32 i = 0; i < 0x1000; i++) {
			INT32 rgb = Palette[i];
			DrvPalette[i] = BurnHighCol((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0);
		}
	}

	BurnTransferClear(0xff);

	draw_sprites((UINT16*)DrvSprRAM0, DrvGfxROM0, nGfxROM0Len);
	if (!game_select) {
		draw_sprites((UINT16*)DrvSprRAM1, DrvGfxROM1, 0x20000);
	}

	BurnTransferCopy(DrvPalette);

	return 0;
}

static inline void AssembleInputs()
{
	memset (DrvInputs, 0xff, 6 * sizeof(UINT16));

	for (INT32 i = 0; i < 16; i++) {
		DrvInputs[0] ^= DrvJoy1[i] << i;
		DrvInputs[1] ^= DrvJoy2[i] << i;
		DrvInputs[2] ^= DrvJoy3[i] << i;
		DrvInputs[3] ^= DrvJoy4[i] << i;
		DrvInputs[4] ^= DrvJoy5[i] << i;
		DrvInputs[5] ^= DrvJoy6[i] << i;
	}

	switch (game_select)
	{
		case 0: // bestbest
		{
			DrvInputs[2] = (DrvDips[1] << 8) | DrvDips[0];
		}
		return;

		case 1: // sunaq
		{
			DrvInputs[2] = DrvDips[0];
		}
		return;

		case 2: // uballoon
		{
			DrvInputs[1] = (DrvInputs[1] & ~0x3000) | ((DrvDips[0] << 8) & 0x3000);
			DrvInputs[2] = DrvDips[1];
			DrvInputs[3] = DrvDips[2];
		}
		return;

		case 3: // bssoccer
		{
			DrvInputs[4] = (DrvDips[1] << 8) | DrvDips[0];
			DrvInputs[5] = ((DrvInputs[5] & 0xfe) | (DrvDips[2] & 0x01)) | 0xff00;
		}
		return;
	}

	return;
}

static INT32 BestbestFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	AssembleInputs();

	SekNewFrame();
	ZetNewFrame();

	INT32 nInterleave = 50;
	INT32 nCyclesTotal[3] = { 6000000 / 60, 6000000 / 60, 6000000 / 60 };
	INT32 nCyclesDone[3] = { 0, 0, 0 };

	SekOpen(0);
	for (INT32 i = 0; i < nInterleave; i++) {

		CPU_RUN(0, Sek);
		if (i == (nInterleave / 2)-1) SekSetIRQLine(1, CPU_IRQSTATUS_AUTO);
		if (i == (nInterleave    )-1) SekSetIRQLine(2, CPU_IRQSTATUS_AUTO);

		ZetOpen(0);
		CPU_RUN_TIMER(1);
		ZetClose();

		ZetOpen(1);
		CPU_RUN(1, Zet);
		ZetClose();
	}
	SekClose();

	if (pBurnSoundOut) {
		AY8910Render(pBurnSoundOut, nBurnSoundLen);
		BurnYM3526Update(pBurnSoundOut, nBurnSoundLen);
		DACUpdate(pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 SunaqFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	AssembleInputs();

	SekNewFrame();
	ZetNewFrame();

	INT32 nInterleave = 50;
	INT32 nCyclesTotal[3] = { 6000000 / 60, 3579500 / 60, 6000000 / 60 };
	INT32 nCyclesDone[3] = { 0, 0, 0 };

	SekOpen(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		CPU_RUN(0, Sek);
		if (i == nInterleave-1) SekSetIRQLine(1, CPU_IRQSTATUS_AUTO);

		ZetOpen(0);
		CPU_RUN_TIMER(1);
		ZetClose();

		ZetOpen(1);
		CPU_RUN(2, Zet);
		ZetClose();
	}

	if (pBurnSoundOut) {
		BurnYM2151Render(pBurnSoundOut, nBurnSoundLen);
		DACUpdate(pBurnSoundOut, nBurnSoundLen);
	}

	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 UballoonFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	AssembleInputs();

	SekNewFrame();
	ZetNewFrame();

	INT32 nInterleave = 50;
	INT32 nCyclesTotal[3] = { 8000000 / 60, 3579500 / 60, 5333333 / 60 };
	INT32 nCyclesDone[3] = { 0, 0, 0 };

	SekOpen(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		CPU_RUN(0, Sek);
		if (i == nInterleave-1) SekSetIRQLine(1, CPU_IRQSTATUS_AUTO);

		ZetOpen(0);
		CPU_RUN_TIMER(1);
		ZetClose();

		ZetOpen(1);
		CPU_RUN(2, Zet);
		ZetClose();
	}

	if (pBurnSoundOut) {
		BurnYM2151Render(pBurnSoundOut, nBurnSoundLen);
		DACUpdate(pBurnSoundOut, nBurnSoundLen);
	}

	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}


static INT32 BssoccerFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	AssembleInputs();

	INT32 nInterleave = 50;
	INT32 nCyclesTotal[4] = { 8000000 / 60, 3579500 / 60, 5333333 / 60, 5333333 / 60 };
	INT32 nCyclesDone[4] = { 0, 0, 0, 0 };

	SekNewFrame();
	ZetNewFrame();

	SekOpen(0);

	for (INT32 i = 0; i < nInterleave; i++)
	{
		CPU_RUN(0, Sek);
		if (i == (nInterleave / 2)-1) SekSetIRQLine(2, CPU_IRQSTATUS_AUTO);
		if (i == (nInterleave    )-1) SekSetIRQLine(1, CPU_IRQSTATUS_AUTO);

		ZetOpen(0);
		CPU_RUN_TIMER(1);
		ZetClose();

		ZetOpen(1);
		CPU_RUN(2, Zet);
		ZetClose();

		ZetOpen(2);
		CPU_RUN(3, Zet);
		ZetClose();
	}

	if (pBurnSoundOut) {
		BurnYM2151Render(pBurnSoundOut, nBurnSoundLen);
		DACUpdate(pBurnSoundOut, nBurnSoundLen);
	}

	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}


static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029692;
	}

	if (nAction & ACB_VOLATILE) {
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		SekScan(nAction);
		ZetScan(nAction);

		DACScan(nAction, pnMin);

		if (game_select) {
			BurnYM2151Scan(nAction, pnMin);
		} else {
			BurnYM3526Scan(nAction, pnMin);
			AY8910Scan(nAction, pnMin);
		}

		SCAN_VAR(soundlatch);
		SCAN_VAR(soundlatch2);
		SCAN_VAR(soundlatch3);
		SCAN_VAR(flipscreen);
		SCAN_VAR(color_bank);
		SCAN_VAR(bestofbest_prot);

		SCAN_VAR(z80bankdata);
	}

	if (game_select == 3) {
		ZetOpen(1);
		bssoccer_bankswitch_w(DrvZ80ROM1, 0, z80bankdata[0]);
		ZetClose();
		ZetOpen(2);
		bssoccer_bankswitch_w(DrvZ80ROM2, 1, z80bankdata[1]);
		ZetClose();
	}

	if (game_select == 2) {
		ZetOpen(1);
		uballoon_bankswitch(z80bankdata[0]);
		ZetClose();
	}

	return 0;
}



//-------------------------------------------------------------------------------------------------
// Drivers


// Best Of Best

static struct BurnRomInfo bestbestRomDesc[] = {
	{ "4.bin",	0x20000, 0x06741994, 1 | BRF_ESS | BRF_PRG },   //  0 - 68K Code
	{ "2.bin",	0x20000, 0x42843dec, 1 | BRF_ESS | BRF_PRG },   //  1
	{ "3.bin",	0x80000, 0xe2bb8f26, 1 | BRF_ESS | BRF_PRG },   //  2
	{ "1.bin",	0x80000, 0xd365e20a, 1 | BRF_ESS | BRF_PRG },   //  3

	{ "5.bin",	0x10000, 0xbb9265e6, 2 | BRF_ESS | BRF_PRG },   //  4 - Z80 #0 Code

	{ "6.bin",	0x10000, 0xdd445f6b, 3 | BRF_ESS | BRF_PRG },   //  5 - Z80 #1 Code

	{ "9.bin",	0x80000, 0xb11994ea, 5 | BRF_GRA },		//  6 - Sprites (Chip 0)
	{ "10.bin",	0x80000, 0x37b41ef5, 5 | BRF_GRA },		//  7
	{ "7.bin",	0x80000, 0x16188b73, 5 | BRF_GRA },		//  8
	{ "8.bin",	0x80000, 0x765ce06b, 5 | BRF_GRA },		//  9

	{ "16.bin",	0x80000, 0xdc46cdea, 6 | BRF_GRA },		// 10 - Sprites (Chip 1)
	{ "17.bin",	0x80000, 0xc6fadd57, 6 | BRF_GRA },		// 11
	{ "13.bin",	0x80000, 0x23283ac4, 6 | BRF_GRA },		// 12
	{ "18.bin",	0x80000, 0x674c4609, 6 | BRF_GRA },		// 13
	{ "14.bin",	0x80000, 0xc210fb53, 6 | BRF_GRA },		// 14
	{ "15.bin",	0x80000, 0x3b1166c7, 6 | BRF_GRA },		// 15
	{ "11.bin",	0x80000, 0x323eebc3, 6 | BRF_GRA },		// 16
	{ "12.bin",	0x80000, 0xca7c8176, 6 | BRF_GRA },		// 17

	{ "82s129.5",	0x00100, 0x10bfcebb, 0 | BRF_OPT },		// 18 - PROMs (not used)
	{ "82s129.6",	0x00100, 0x10bfcebb, 0 | BRF_OPT },		// 19
};

STD_ROM_PICK(bestbest)
STD_ROM_FN(bestbest)

struct BurnDriver BurnDrvBestbest = {
	"bestbest", NULL, NULL, NULL, "1994",
	"Best Of Best\0", NULL, "SunA", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_VSFIGHT, 0,
	NULL, bestbestRomInfo, bestbestRomName, NULL, NULL, NULL, NULL, BestbestInputInfo, bestbestDIPInfo,
	BestbestInit, DrvExit, BestbestFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// SunA Quiz 6000 Academy (940620-6)

static struct BurnRomInfo sunaqRomDesc[] = {
	{ "prog2.bin", 	0x80000, 0xa92bce45, 1 | BRF_ESS | BRF_PRG },   //  0 - 68K Code
	{ "prog1.bin",	0x80000, 0xff690e7e, 1 | BRF_ESS | BRF_PRG },   //  1

	{ "audio1.bin",	0x10000, 0x3df42f82, 2 | BRF_ESS | BRF_PRG },   //  2 - Z80 #0 Code

	{ "audio2.bin",	0x80000, 0xcac85ba9, 3 | BRF_ESS | BRF_PRG },   //  3 - Z80 #1 Code

	{ "gfx1.bin",	0x80000, 0x0bde5acf, 5 | BRF_GRA },		//  4 - Sprites
	{ "gfx2.bin",	0x80000, 0x24b74826, 5 | BRF_GRA },		//  5
};

STD_ROM_PICK(sunaq)
STD_ROM_FN(sunaq)

struct BurnDriver BurnDrvSunaq = {
	"sunaq", NULL, NULL, NULL, "1994",
	"SunA Quiz 6000 Academy (940620-6)\0", NULL, "SunA", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_QUIZ, 0,
	NULL, sunaqRomInfo, sunaqRomName, NULL, NULL, NULL, NULL, SunaqInputInfo, sunaqDIPInfo,
	SunaqInit, DrvExit, SunaqFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Ultra Balloon

static struct BurnRomInfo uballoonRomDesc[] = {
	{ "prg2.rom2",	0x80000, 0x72ab80ea, 1 | BRF_ESS | BRF_PRG },   //  0 - 68K Code
	{ "prg1.rom1",	0x80000, 0x27a04f55, 1 | BRF_ESS | BRF_PRG },   //  1

	{ "audio1.rom7",	0x10000, 0xc771f2b4, 2 | BRF_ESS | BRF_PRG },   //  2 - Z80 #0 Code

	{ "audio2.rom8",	0x20000, 0xc7f75347, 3 | BRF_ESS | BRF_PRG },   //  3 - Z80 #1 Code

	{ "gfx3.rom3",	0x80000, 0xfd2ec297, 5 | BRF_GRA },		//  4 - Sprites
	{ "gfx5.rom5",	0x80000, 0x6307aa60, 5 | BRF_GRA },		//  5
	{ "gfx4.rom4",	0x80000, 0x718f3150, 5 | BRF_GRA },		//  6
	{ "gfx6.rom6",	0x80000, 0xaf7e057e, 5 | BRF_GRA },		//  7
};

STD_ROM_PICK(uballoon)
STD_ROM_FN(uballoon)

struct BurnDriver BurnDrvUballoon = {
	"uballoon", NULL, NULL, NULL, "1996",
	"Ultra Balloon\0", NULL, "SunA (Unico license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_HISCORE_SUPPORTED, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM, 0,
	NULL, uballoonRomInfo, uballoonRomName, NULL, NULL, NULL, NULL, UballoonInputInfo, uballoonDIPInfo,
	UballoonInit, DrvExit, UballoonFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Back Street Soccer (KRB-0031 PCB)

static struct BurnRomInfo bssoccerRomDesc[] = {
	{ "02",		0x080000, 0x32871005, 1 | BRF_ESS | BRF_PRG },   //  0 - 68K Code
	{ "01",		0x080000, 0xace00db6, 1 | BRF_ESS | BRF_PRG },   //  1
	{ "04",		0x080000, 0x25ee404d, 1 | BRF_ESS | BRF_PRG },   //  2
	{ "03",		0x080000, 0x1a131014, 1 | BRF_ESS | BRF_PRG },   //  3

	{ "11",		0x010000, 0xdf7ae9bc, 2 | BRF_ESS | BRF_PRG },   //  4 - Z80 #0 Code

	{ "13",		0x080000, 0x2b273dca, 3 | BRF_ESS | BRF_PRG },   //  5 - Z80 #1 Code

	{ "12",		0x080000, 0x6b73b87b, 4 | BRF_ESS | BRF_PRG },   //  6 - Z80 #0 Code

	{ "05",		0x080000, 0xa5245bd4, 5 | BRF_GRA },		 //  7 - Sprites
	{ "07",		0x080000, 0xfdb765c2, 5 | BRF_GRA },		 //  8
	{ "09",		0x080000, 0x0e82277f, 5 | BRF_GRA },		 //  9
	{ "06",		0x080000, 0xd42ce84b, 5 | BRF_GRA },		 // 10
	{ "08",		0x080000, 0x96cd2136, 5 | BRF_GRA },		 // 11
	{ "10",		0x080000, 0x1ca94d21, 5 | BRF_GRA },		 // 12
};

STD_ROM_PICK(bssoccer)
STD_ROM_FN(bssoccer)

struct BurnDriver BurnDrvBssoccer = {
	"bssoccer", NULL, NULL, NULL, "1996",
	"Back Street Soccer (KRB-0031 PCB)\0", NULL, "SunA (Unico license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_HISCORE_SUPPORTED, 4, HARDWARE_MISC_POST90S, GBF_SPORTSFOOTBALL, 0,
	NULL, bssoccerRomInfo, bssoccerRomName, NULL, NULL, NULL, NULL, BssoccerInputInfo, bssoccerDIPInfo,
	BssoccerInit, DrvExit, BssoccerFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Back Street Soccer (KRB-0032A PCB)

static struct BurnRomInfo bssocceraRomDesc[] = {
	{ "uc16001",	0x200000, 0x82fa613a, 1 | BRF_ESS | BRF_PRG },   //  0 - 68K Code

	{ "unico5",		0x010000, 0xdf7ae9bc, 2 | BRF_ESS | BRF_PRG },   //  1 - Z80 #0 Code

	{ "uc04005",	0x080000, 0x2b273dca, 3 | BRF_ESS | BRF_PRG },   //  2 - Z80 #1 Code

	{ "uc04004",	0x080000, 0x6b73b87b, 4 | BRF_ESS | BRF_PRG },   //  3 - Z80 #0 Code

	{ "uc16002",	0x200000, 0x884f3ecf, 5 | BRF_GRA },		 //  4 - Sprites
	{ "uc08003",	0x100000, 0xd17c23f5, 5 | BRF_GRA },		 //  5
};

STD_ROM_PICK(bssoccera)
STD_ROM_FN(bssoccera)

static INT32 BssocceraInit()
{
	Bssoccera = 1;

	return BssoccerInit();
}

struct BurnDriver BurnDrvBssoccera = {
	"bssoccera", "bssoccer", NULL, NULL, "1996",
	"Back Street Soccer (KRB-0032A PCB)\0", NULL, "SunA (Unico license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 4, HARDWARE_MISC_POST90S, GBF_SPORTSFOOTBALL, 0,
	NULL, bssocceraRomInfo, bssocceraRomName, NULL, NULL, NULL, NULL, BssoccerInputInfo, bssoccerDIPInfo,
	BssocceraInit, DrvExit, BssoccerFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};
