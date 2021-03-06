/* Hot Blocks */
/*
driver by David Haywood
*/

/*
HotBlock board

Tetris with naughty bits

        ||||||||||||||||
+-------++++++++++++++++-------+
|                              |
|  YM2149 TESTSW               |
|                              |
|    62256 62256   6116 6116   |
|                              |
|    24mhz  TPC1020AFN 24c04a  |
|                              |
|                     PAL      |
| P8088-1 IC4 IC5 62256 62256  |
|                              |
+------------------------------+

330ohm resistor packs for colours


--

there are a variety of test modes which can be obtained
by resetting while holding down player 2 buttons

eeprom / backup data not hooked up ( 24c04a on port4 )

most sources say this is a game by Nics but I believe Nics
to be a company from Korea, this game is quite clearly a
Spanish game, we know for a fact that NIX are from Spain
so it could be by them instead



*/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "sound/ay8910.h"

class hotblock_state : public driver_device
{
public:
	hotblock_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_vram(*this, "vram"),
		m_maincpu(*this, "maincpu"),
		m_palette(*this, "palette")  { }

	/* memory pointers */
	required_shared_ptr<UINT8> m_vram;

	/* misc */
	int      m_port0;
	int      m_port4;

	/* memory */
	UINT8    m_pal[0x10000];
	DECLARE_READ8_MEMBER(hotblock_video_read);
	DECLARE_READ8_MEMBER(hotblock_port4_r);
	DECLARE_WRITE8_MEMBER(hotblock_port4_w);
	DECLARE_WRITE8_MEMBER(hotblock_port0_w);
	DECLARE_WRITE8_MEMBER(hotblock_video_write);
	virtual void video_start();
	UINT32 screen_update_hotblock(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(hotblocks_irq);
	required_device<cpu_device> m_maincpu;
	required_device<palette_device> m_palette;
};



READ8_MEMBER(hotblock_state::hotblock_video_read)
{
	/* right?, anything else?? */
	if (m_port0 & 0x20) // port 0 = a8 e8 -- palette
	{
		return m_pal[offset];
	}
	else // port 0 = 88 c8
	{
		return m_vram[offset];
	}
}

/* port 4 is some kind of eeprom / storage .. used to store the scores */
READ8_MEMBER(hotblock_state::hotblock_port4_r)
{
//  mame_printf_debug("port4_r\n");
	return 0x00;
}


WRITE8_MEMBER(hotblock_state::hotblock_port4_w)
{
//  mame_printf_debug("port4_w: pc = %06x : data %04x\n", space.device().safe_pc(), data);
//  popmessage("port4_w: pc = %06x : data %04x", space.device().safe_pc(), data);

	m_port4 = data;
}



WRITE8_MEMBER(hotblock_state::hotblock_port0_w)
{
//  popmessage("port4_w: pc = %06x : data %04x", space.device().safe_pc(), data);

	m_port0 = data;
}

WRITE8_MEMBER(hotblock_state::hotblock_video_write)
{
	/* right?, anything else?? */
	if (m_port0 & 0x20) // port 0 = a8 e8 -- palette
	{
		m_pal[offset] = data;
	}
	else // port 0 = 88 c8
	{
		m_vram[offset] = data;
	}
}

static ADDRESS_MAP_START( hotblock_map, AS_PROGRAM, 8, hotblock_state )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x10000, 0x1ffff) AM_READWRITE(hotblock_video_read, hotblock_video_write) AM_SHARE("vram")
	AM_RANGE(0x20000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hotblock_io, AS_IO, 8, hotblock_state )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(hotblock_port0_w)
	AM_RANGE(0x0004, 0x0004) AM_READWRITE(hotblock_port4_r, hotblock_port4_w)
	AM_RANGE(0x8000, 0x8001) AM_DEVWRITE("aysnd", ay8910_device, address_data_w)
	AM_RANGE(0x8001, 0x8001) AM_DEVREAD("aysnd", ay8910_device, data_r)
ADDRESS_MAP_END



void hotblock_state::video_start()
{
	save_item(NAME(m_pal));
}

UINT32 hotblock_state::screen_update_hotblock(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int y, x, count;
	int i;
	static const int xxx = 320, yyy = 204;

	bitmap.fill(m_palette->black_pen());

	for (i = 0; i < 256; i++)
	{
		int dat = (m_pal[i * 2 + 1] << 8) | m_pal[i * 2];
		m_palette->set_pen_color(i, pal5bit(dat >> 0), pal5bit(dat >> 5), pal5bit(dat >> 10));
	}

	count = 0;
	for (y = 0; y < yyy; y++)
	{
		for(x = 0; x < xxx; x++)
		{
			if (m_port0 & 0x40)
				bitmap.pix16(y, x) = m_vram[count];
			count++;
		}
	}

	return 0;
}


static INPUT_PORTS_START( hotblock )
	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // unused?

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) // used to get test mode
INPUT_PORTS_END


INTERRUPT_GEN_MEMBER(hotblock_state::hotblocks_irq)/* right? */
{
	device.execute().set_input_line(INPUT_LINE_NMI, PULSE_LINE);
}

static const ay8910_interface ay8910_config =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_INPUT_PORT("P1"),
	DEVCB_INPUT_PORT("P2"),
	DEVCB_NULL,
	DEVCB_NULL
};


static MACHINE_CONFIG_START( hotblock, hotblock_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8088, 10000000)
	MCFG_CPU_PROGRAM_MAP(hotblock_map)
	MCFG_CPU_IO_MAP(hotblock_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", hotblock_state,  hotblocks_irq)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(1024,1024)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MCFG_SCREEN_UPDATE_DRIVER(hotblock_state, screen_update_hotblock)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 256)


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("aysnd", AY8910, 1000000)
	MCFG_SOUND_CONFIG(ay8910_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( hotblock )
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "hotblk5.ic4", 0x000000, 0x080000, CRC(5f90f776) SHA1(5ca74714a7d264b4fafaad07dc11e57308828d30) )
	ROM_LOAD( "hotblk6.ic5", 0x080000, 0x080000, CRC(3176d231) SHA1(ac22fd0e9820c6714f51a3d8315eb5d43ef91eeb) )
ROM_END

GAME( 1993, hotblock, 0,        hotblock, hotblock, driver_device, 0, ROT0,  "NIX?", "Hot Blocks - Tetrix II", 0 )
