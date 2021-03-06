/***************************************************************************

    Amiga keyboard controller emulation

***************************************************************************/


#include "emu.h"
#include "includes/amiga.h"
#include "amigakbd.h"


#define KEYBOARD_BUFFER_SIZE    256

const device_type AMIGAKBD = &device_creator<amigakbd_device>;

//-------------------------------------------------
//  amigakbd_device - constructor
//-------------------------------------------------

amigakbd_device::amigakbd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, AMIGAKBD, "Amiga Keyboard", tag, owner, clock, "amigakbd", __FILE__),
	m_write_kclk(*this),
	m_write_kdat(*this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void amigakbd_device::device_start()
{
	m_write_kclk.resolve_safe();
	m_write_kdat.resolve_safe();

	/* allocate a keyboard buffer */
	m_buf = auto_alloc_array(machine(), UINT8, KEYBOARD_BUFFER_SIZE);
	m_buf_pos = 0;
	m_cur_pos = 0;
	m_timer = timer_alloc(0);
	m_timer->reset();
}

void amigakbd_device::kbd_sendscancode(UINT8 scancode )
{
	int j;

	/* send over to the cia A */
	for( j = 0; j < 8; j++ )
	{
		m_write_kclk(0);    /* lower cnt */
		m_write_kdat(BIT(scancode << j, 7)); /* set the serial data */
		m_write_kclk(1);    /* raise cnt */
	}
}

//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void amigakbd_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	UINT8   scancode;

	(void)param;

	/* if we don't have pending data, bail */

	if ( m_buf_pos == m_cur_pos )
		return;

	/* fetch the next scan code and send it to the Amiga */
	scancode = m_buf[m_cur_pos++];
	m_cur_pos %= KEYBOARD_BUFFER_SIZE;
	kbd_sendscancode(scancode);

	/* if we still have more data, schedule another update */
	if ( m_buf_pos != m_cur_pos )
	{
		m_timer->adjust(machine().first_screen()->frame_period() / 4);
	}
}

INPUT_CHANGED_MEMBER(amigakbd_device::kbd_update)
{
	int index = (int)(FPTR)param, i;
	UINT32  oldvalue = oldval * field.mask(), newvalue = newval * field.mask();
	UINT32  delta = oldvalue ^ newvalue;

	/* Special case Page UP, which we will use as Action Replay button */
	if ( (index == 3) && ( delta & 0x80000000 ) && ( newvalue & 0x80000000 ) )
	{
		const amiga_machine_interface *amiga_intf = amiga_get_interface(machine());

		if ( amiga_intf != NULL && amiga_intf->nmi_callback )
		{
			(*amiga_intf->nmi_callback)(machine());
		}
	}
	else
	{
		int key_buf_was_empty = ( m_buf_pos == m_cur_pos ) ? 1 : 0;

		for( i = 0; i < 32; i++ )
		{
			if ( delta & ( 1 << i ) )
			{
				int down = ( newvalue & ( 1 << i ) ) ? 0 : 1;
				int scancode = ( ( (index*32)+i ) << 1 ) | down;
				int amigacode = ~scancode;

				/* add the keycode to the buffer */
				m_buf[m_buf_pos++] = amigacode & 0xff;
				m_buf_pos %= KEYBOARD_BUFFER_SIZE;
			}
		}

		/* if the buffer was empty and we have new data, start a timer to send the keystrokes */
		if ( key_buf_was_empty && ( m_buf_pos != m_cur_pos ) )
		{
			m_timer->adjust(machine().first_screen()->frame_period() / 4);
		}
	}
}

/*********************************************************************************************/

/* Layout is for the US A500 keyboard */

INPUT_PORTS_START( amiga_us_keyboard )
	PORT_START("amiga_keyboard_0")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')     // 00
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')         // 01
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')         // 02
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')         // 03
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')         // 04
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')         // 05
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')         // 06
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')         // 07
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')         // 08
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')         // 09
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')         // 0A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')     // 0B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')        // 0C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('\\') PORT_CHAR('\xA6')    // 0D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0)                                  // 0E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))   // 0F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')         // 10
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')         // 11
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')         // 12
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')         // 13
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')         // 14
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')         // 15
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')         // 16
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')         // 17
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')         // 18
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')         // 19
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') // 1A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')    // 1B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0)                                  // 1C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))   // 1D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))   // 1E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 0) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))   // 1F

	PORT_START("amiga_keyboard_1")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')         // 20
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')         // 21
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')         // 22
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')         // 23
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')         // 24
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')         // 25
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')         // 26
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')         // 27
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')         // 28
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')     // 29
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')        // 2A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1)                                  // 2B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1)                                  // 2C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))   // 2D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))   // 2E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))   // 2F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1)                                  // 30
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')         // 31
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')         // 32
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')         // 33
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')         // 34
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')         // 35
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')         // 36
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')         // 37
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')     // 38
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_STOP)     PORT_CHAR('.') PORT_CHAR('>')      // 39
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')     // 3A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1)                                  // 3B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))// 3C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))   // 3D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))   // 3E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 1) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))   // 3F

	PORT_START("amiga_keyboard_2")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')                        // 40
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)                  // 41
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')                     // 42
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))   // 43
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)                     // 44
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))       // 45
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))       // 46
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 47
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 48
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 49
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))   // 4A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 4B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))         // 4C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))     // 4D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))   // 4E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))     // 4F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))         // 50
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))         // 51
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))         // 52
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))         // 53
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))         // 54
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))         // 55
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))         // 56
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))         // 57
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))         // 58
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))       // 59
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 5A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2)                                  // 5B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_SLASH_PAD) PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))   // 5C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR(UCHAR_MAMEKEY(ASTERISK)) // 5D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD)) // 5E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 2) PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT)) // 5F

	PORT_START("amiga_keyboard_3")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)   // 60
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)  // 61
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK)         // 62
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))// 63
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Alt (Left)") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) // 64
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Alt (Right)") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))    // 65
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Amiga A (Left)") PORT_CODE(KEYCODE_LWIN)                // 66
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3) PORT_NAME("Amiga A (Right)")  PORT_CODE(KEYCODE_RWIN)          // 67
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 68
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 69
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 6F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 70
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 71
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 72
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 73
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 74
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 75
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 76
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 77
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 78
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 79
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 7A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 7B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 7C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 7D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)                                  // 7E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED_MEMBER(DEVICE_SELF, amigakbd_device, kbd_update, 3)    PORT_CODE(KEYCODE_PGUP)         // 7F NMI button
INPUT_PORTS_END

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor amigakbd_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( amiga_us_keyboard );
}
