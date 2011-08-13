/************************************************************************************

Sega Saturn SMPC - System Manager and Peripheral Control MCU simulation

The SMPC is actually a 4-bit Hitachi HD404920FS MCU, labeled with a Sega custom
315-5744 (that needs decapping)

MCU simulation by Angelo Salese & R. Belmont

TODO:
- timings;
- fix intback issue with inputs (according to the docs, it should fall in between
  VBLANK-IN and OUT, for obvious reasons);
- clean-ups;

*************************************************************************************/
/* SMPC Addresses

00
01 -w  Input Register 0 (IREG)
02
03 -w  Input Register 1
04
05 -w  Input Register 2
06
07 -w  Input Register 3
08
09 -w  Input Register 4
0a
0b -w  Input Register 5
0c
0d -w  Input Register 6
0e
0f
10
11
12
13
14
15
16
17
18
19
1a
1b
1c
1d
1e
1f -w  Command Register (COMREG)
20
21 r-  Output Register 0 (OREG)
22
23 r-  Output Register 1
24
25 r-  Output Register 2
26
27 r-  Output Register 3
28
29 r-  Output Register 4
2a
2b r-  Output Register 5
2c
2d r-  Output Register 6
2e
2f r-  Output Register 7
30
31 r-  Output Register 8
32
33 r-  Output Register 9
34
35 r-  Output Register 10
36
37 r-  Output Register 11
38
39 r-  Output Register 12
3a
3b r-  Output Register 13
3c
3d r-  Output Register 14
3e
3f r-  Output Register 15
40
41 r-  Output Register 16
42
43 r-  Output Register 17
44
45 r-  Output Register 18
46
47 r-  Output Register 19
48
49 r-  Output Register 20
4a
4b r-  Output Register 21
4c
4d r-  Output Register 22
4e
4f r-  Output Register 23
50
51 r-  Output Register 24
52
53 r-  Output Register 25
54
55 r-  Output Register 26
56
57 r-  Output Register 27
58
59 r-  Output Register 28
5a
5b r-  Output Register 29
5c
5d r-  Output Register 30
5e
5f r-  Output Register 31
60
61 r-  SR
62
63 rw  SF
64
65
66
67
68
69
6a
6b
6c
6d
6e
6f
70
71
72
73
74
75 rw PDR1
76
77 rw PDR2
78
79 -w DDR1
7a
7b -w DDR2
7c
7d -w IOSEL2/1
7e
7f -w EXLE2/1
*/

#include "emu.h"
#include "coreutil.h"
#include "includes/stv.h"
#include "machine/smpc.h"
#include "machine/eeprom.h"

#define LOG_SMPC 0
#define LOG_PAD_CMD 0

static TIMER_CALLBACK( stv_bankswitch_state )
{
	saturn_state *state = machine.driver_data<saturn_state>();
	static const char *const banknames[] = { "game0", "game1", "game2", "game3" };
	UINT8* game_region;

	if(state->m_prev_bankswitch != param)
	{
		game_region = machine.region(banknames[param])->base();

		if (game_region)
			memcpy(machine.region("abus")->base(), game_region, 0x3000000);
		else
			memset(machine.region("abus")->base(), 0x00, 0x3000000);

		state->m_prev_bankswitch = param;
	}
}

static void stv_select_game(running_machine &machine, int gameno)
{
	machine.scheduler().timer_set(attotime::zero, FUNC(stv_bankswitch_state), gameno);
}

static void smpc_master_on(running_machine &machine)
{
	saturn_state *state = machine.driver_data<saturn_state>();

	device_set_input_line(state->m_maincpu, INPUT_LINE_RESET, CLEAR_LINE);
}

static TIMER_CALLBACK( smpc_slave_enable )
{
	saturn_state *state = machine.driver_data<saturn_state>();

	device_set_input_line(state->m_slave, INPUT_LINE_RESET, param ? ASSERT_LINE : CLEAR_LINE);
	state->m_smpc.OREG[31] = param + 0x02; //read-back for last command issued
	state->m_smpc.SF = 0x00; //clear hand-shake flag
}

static TIMER_CALLBACK( smpc_sound_enable )
{
	saturn_state *state = machine.driver_data<saturn_state>();

	device_set_input_line(state->m_audiocpu, INPUT_LINE_RESET, param ? ASSERT_LINE : CLEAR_LINE);
	state->m_en_68k = param ^ 1;
	state->m_smpc.OREG[31] = param + 0x06; //read-back for last command issued
	state->m_smpc.SF = 0x00; //clear hand-shake flag
}

static void smpc_system_reset(running_machine &machine)
{
	saturn_state *state = machine.driver_data<saturn_state>();

	/*Only backup ram and SMPC ram are retained after that this command is issued.*/
	memset(state->m_scu_regs ,0x00,0x000100);
	memset(state->m_scsp_regs,0x00,0x001000);
	memset(state->m_sound_ram,0x00,0x080000);
	memset(state->m_workram_h,0x00,0x100000);
	memset(state->m_workram_l,0x00,0x100000);
	memset(state->m_vdp2_regs,0x00,0x040000);
	memset(state->m_vdp2_vram,0x00,0x100000);
	memset(state->m_vdp2_cram,0x00,0x080000);
	memset(state->m_vdp1_vram,0x00,0x100000);
	//A-Bus

	device_set_input_line(state->m_maincpu, INPUT_LINE_RESET, PULSE_LINE);
}

static void smpc_change_clock(running_machine &machine, UINT8 cmd)
{
	saturn_state *state = machine.driver_data<saturn_state>();
	UINT32 xtal;

	xtal = cmd ? MASTER_CLOCK_320 : MASTER_CLOCK_352;

	machine.device("maincpu")->set_unscaled_clock(xtal/2);
	machine.device("slave")->set_unscaled_clock(xtal/2);

	state->m_vdp2.dotsel = cmd ^ 1;
	stv_vdp2_dynamic_res_change(machine);

	device_set_input_line(state->m_maincpu, INPUT_LINE_NMI, PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
	device_set_input_line(state->m_slave, INPUT_LINE_RESET, ASSERT_LINE); // command also asserts slave cpu
	/* TODO: VDP1 / VDP2 / SCU / SCSP default power ON values */
}

static TIMER_CALLBACK( stv_smpc_intback )
{
	saturn_state *state = machine.driver_data<saturn_state>();
	int i;

	state->m_smpc.OREG[0] = (0x80) | ((state->m_NMI_reset & 1) << 6);

	for(i=0;i<7;i++)
		state->m_smpc.OREG[1+i] = state->m_smpc.rtc_data[i];

	state->m_smpc.OREG[8]=0x00;  // CTG0 / CTG1?

	state->m_smpc.OREG[9]=0x00;  // TODO: system region on Saturn

	state->m_smpc.OREG[10]= 0 << 7 |
	                         state->m_vdp2.dotsel << 6 |
	                         1 << 5 |
	                         1 << 4 |
	                         0 << 3 | //MSHNMI
	                         1 << 2 |
	                         0 << 1 | //SYSRES
	                         0 << 0;  //SOUNDRES
	state->m_smpc.OREG[11]= 0 << 6; //CDRES

	for(i=0;i<4;i++)
		state->m_smpc.OREG[12+i]=state->m_smpc.SMEM[i];

	for(i=0;i<15;i++)
		state->m_smpc.OREG[16+i]=0xff; // undefined

	//  /*This is for RTC,cartridge code and similar stuff...*/
	//if(LOG_SMPC) printf ("Interrupt: System Manager (SMPC) at scanline %04x, Vector 0x47 Level 0x08\n",scanline);
	if(!(state->m_scu.ism & IRQ_SMPC))
		device_set_input_line_and_vector(state->m_maincpu, 8, HOLD_LINE, 0x47);
	else
		state->m_scu.ist |= (IRQ_SMPC);

	/* put issued command in OREG31 */
	state->m_smpc.OREG[31] = 0x10; // TODO: doc says 0?
	/* clear hand-shake flag */
	state->m_smpc.SF = 0x00;
}

/*
	[0] port status:
		0x04 Sega-tap
		0x16 Multi-tap
		0x2x clock serial peripheral
		0xf0 peripheral isn't connected
	    0xf1 peripheral is connected
	[1] Peripheral ID (note: lowest four bits determines the size of the input packet)
		0x02 digital pad
		0x25 (tested by Game Basic?)
		0x34 keyboard
*/

static void smpc_digital_pad(running_machine &machine, UINT8 pad_num, UINT8 offset)
{
	saturn_state *state = machine.driver_data<saturn_state>();
	static const char *const padnames[] = { "JOY1", "JOY2" };
	UINT16 pad_data;

	pad_data = input_port_read(machine, padnames[pad_num]);
	state->m_smpc.OREG[0+pad_num*offset] = 0xf1;
	state->m_smpc.OREG[1+pad_num*offset] = 0x02;
	state->m_smpc.OREG[2+pad_num*offset] = pad_data>>8;
	state->m_smpc.OREG[3+pad_num*offset] = pad_data & 0xff;
}

static void smpc_keyboard(running_machine &machine, UINT8 pad_num, UINT8 offset)
{
	saturn_state *state = machine.driver_data<saturn_state>();
	UINT16 game_key;

	game_key = 0xffff;

	game_key ^= ((input_port_read(machine, "KEYS_1") & 0x80) << 8); // right
	game_key ^= ((input_port_read(machine, "KEYS_1") & 0x40) << 8); // left
	game_key ^= ((input_port_read(machine, "KEYS_1") & 0x20) << 8); // down
	game_key ^= ((input_port_read(machine, "KEYS_1") & 0x10) << 8); // up
	game_key ^= ((input_port_read(machine, "KEYF") & 0x80) << 4); // ESC -> START
	game_key ^= ((input_port_read(machine, "KEY3") & 0x04) << 8); // Z / A trigger
	game_key ^= ((input_port_read(machine, "KEY4") & 0x02) << 8); // C / C trigger
	game_key ^= ((input_port_read(machine, "KEY6") & 0x04) << 6); // X / B trigger
	game_key ^= ((input_port_read(machine, "KEY2") & 0x20) << 2); // Q / R trigger
	game_key ^= ((input_port_read(machine, "KEY3") & 0x10) << 2); // A / X trigger
	game_key ^= ((input_port_read(machine, "KEY3") & 0x08) << 2); // S / Y trigger
	game_key ^= ((input_port_read(machine, "KEY4") & 0x08) << 1); // D / Z trigger
	game_key ^= ((input_port_read(machine, "KEY4") & 0x10) >> 1); // E / L trigger

	state->m_smpc.OREG[0+pad_num*offset] = 0xf1;
	state->m_smpc.OREG[1+pad_num*offset] = 0x34;
	state->m_smpc.OREG[2+pad_num*offset] = game_key>>8; // game buttons, TODO
	state->m_smpc.OREG[3+pad_num*offset] = game_key & 0xff;
	/*
		x--- ---- 0
		-x-- ---- caps lock
		--x- ---- num lock
		---x ---- scroll lock
		---- x--- data ok
		---- -x-- 1
		---- --x- 1
		---- ---x Break key
	*/
	state->m_smpc.OREG[4+pad_num*offset] = state->m_keyb.status | 6;
	state->m_smpc.OREG[5+pad_num*offset] = state->m_keyb.data;
}

static TIMER_CALLBACK( intback_peripheral )
{
	saturn_state *state = machine.driver_data<saturn_state>();
	int pad_num;
	static const UINT8 peri_id[10] = { 0x02, 0x13, 0x15, 0x23, 0x23, 0x34, 0xe1, 0xe2, 0xe3, 0xff };
	UINT8 read_id[2];
	UINT8 offset;

//  if (LOG_SMPC) logerror("SMPC: providing PAD data for intback, pad %d\n", intback_stage-2);

	read_id[0] = (input_port_read(machine, "INPUT_TYPE")) & 0x0f;
	read_id[1] = (input_port_read(machine, "INPUT_TYPE")) >> 4;

	/* doesn't work? */
	//pad_num = state->m_smpc.intback_stage - 1;

	if(LOG_PAD_CMD) printf("%d\n",state->m_smpc.intback_stage - 1);

	offset = 0;

	for(pad_num=0;pad_num<2;pad_num++)
	{
		switch(read_id[pad_num])
		{
			case 0: smpc_digital_pad(machine,pad_num,offset); break;
			case 5: smpc_keyboard(machine,pad_num,offset); break;
		}

		offset += (peri_id[read_id[pad_num]] & 0xf) + 2; /* offset for pad 2 */
	}

	if (state->m_smpc.intback_stage == 2)
	{
		state->m_smpc.SR = (0x80 | state->m_smpc.pmode);	// pad 2, no more data, echo back pad mode set by intback
		state->m_smpc.intback_stage = 0;
	}
	else
	{
		state->m_smpc.SR = (0xc0 | state->m_smpc.pmode);	// pad 1, more data, echo back pad mode set by intback
		state->m_smpc.intback_stage ++;
	}

	if(!(state->m_scu.ism & IRQ_SMPC))
		device_set_input_line_and_vector(state->m_maincpu, 8, HOLD_LINE, 0x47);
	else
		state->m_scu.ist |= (IRQ_SMPC);

	state->m_smpc.OREG[31] = 0x10; /* callback for last command issued */
	state->m_smpc.SF = 0x00;	/* clear hand-shake flag */
}

static TIMER_CALLBACK( saturn_smpc_intback )
{
	saturn_state *state = machine.driver_data<saturn_state>();

	if(state->m_smpc.IREG[0] != 0)
	{
		{
			int i;

			state->m_smpc.OREG[0] = (0x80) | ((state->m_NMI_reset & 1) << 6); // bit 7: SETTIME (RTC isn't setted up properly)

			for(i=0;i<7;i++)
				state->m_smpc.OREG[1+i] = state->m_smpc.rtc_data[i];

			state->m_smpc.OREG[8]=0x00;  //Cartridge code?

			state->m_smpc.OREG[9] = state->m_saturn_region;

			state->m_smpc.OREG[10]= 0 << 7 |
			                         state->m_vdp2.dotsel << 6 |
			                         1 << 5 |
			                         1 << 4 |
			                         0 << 3 | //MSHNMI
			                         1 << 2 |
			                         0 << 1 | //SYSRES
			                         0 << 0;  //SOUNDRES
			state->m_smpc.OREG[11]= 0 << 6; //CDRES

			for(i=0;i<4;i++)
				state->m_smpc.OREG[12+i]=state->m_smpc.SMEM[i];

			for(i=0;i<15;i++)
				state->m_smpc.OREG[16+i]=0xff; // undefined
		}

		state->m_smpc.intback_stage = (state->m_smpc.IREG[1] & 8) >> 3; // first peripheral
		state->m_smpc.SR = 0x40 | state->m_smpc.intback_stage << 5;
		state->m_smpc.pmode = state->m_smpc.IREG[0]>>4;

		if(!(state->m_scu.ism & IRQ_SMPC))
			device_set_input_line_and_vector(state->m_maincpu, 8, HOLD_LINE, 0x47);
		else
			state->m_scu.ist |= (IRQ_SMPC);

		/* put issued command in OREG31 */
		state->m_smpc.OREG[31] = 0x10;
		/* clear hand-shake flag */
		state->m_smpc.SF = 0x00;
	}
	else if(state->m_smpc.IREG[1] & 8)
	{
		state->m_smpc.intback_stage = (state->m_smpc.IREG[1] & 8) >> 3; // first peripheral
		state->m_smpc.SR = 0x40;
		state->m_smpc.OREG[31] = 0x10;
		machine.scheduler().timer_set(attotime::from_usec(0), FUNC(intback_peripheral),0);
	}
	else
	{
		printf("SMPC intback bogus behaviour called %02x %02x\n",state->m_smpc.IREG[0],state->m_smpc.IREG[1]);
	}

}

static void smpc_rtc_write(running_machine &machine)
{
	saturn_state *state = machine.driver_data<saturn_state>();
	int i;

	for(i=0;i<7;i++)
		state->m_smpc.rtc_data[i] = state->m_smpc.IREG[i];
}

static void smpc_memory_setting(running_machine &machine)
{
	saturn_state *state = machine.driver_data<saturn_state>();
	int i;

	for(i=0;i<4;i++)
		state->m_smpc.SMEM[i] = state->m_smpc.IREG[i];
}

static void smpc_nmi_req(running_machine &machine)
{
	saturn_state *state = machine.driver_data<saturn_state>();

	/*NMI is unconditionally requested?*/
	device_set_input_line(state->m_maincpu, INPUT_LINE_NMI, PULSE_LINE);
}

static void smpc_nmi_set(running_machine &machine,UINT8 cmd)
{
	saturn_state *state = machine.driver_data<saturn_state>();

	state->m_NMI_reset = cmd ^ 1;
	state->m_smpc.OREG[0] = (0x80) | ((state->m_NMI_reset & 1) << 6);
}

/********************************************
 *
 * ST-V handlers
 *
 *******************************************/

READ8_HANDLER( stv_SMPC_r )
{
	saturn_state *state = space->machine().driver_data<saturn_state>();
	int return_data = 0;

	if(!(offset & 1))
		return 0;

	if(offset >= 0x21 && offset <= 0x5f)
		return_data = state->m_smpc.OREG[(offset-0x21) >> 1];

	if (offset == 0x61) // TODO: SR
		return_data = 0x20 ^ 0xff;

	if (offset == 0x63)
		return_data = state->m_smpc.SF;

	if (offset == 0x75)//PDR1 read
		return_data = input_port_read(space->machine(), "DSW1");

	if (offset == 0x77)//PDR2 read
		return_data = (0xfe | space->machine().device<eeprom_device>("eeprom")->read_bit());

	return return_data;
}

static void stv_comreg_exec(address_space *space,UINT8 data)
{
	switch (data)
	{
		case 0x00:
			if(LOG_SMPC) printf ("SMPC: Master ON\n");
			smpc_master_on(space->machine());
			break;
		//in theory 0x01 is for Master OFF,but obviously is not used.
		case 0x02:
		case 0x03:
			if(LOG_SMPC) printf ("SMPC: Slave %s\n",(data & 1) ? "off" : "on");
			space->machine().scheduler().timer_set(attotime::from_usec(100), FUNC(smpc_slave_enable),data & 1);
			break;
		case 0x06:
		case 0x07:
			if(LOG_SMPC) printf ("SMPC: Sound %s, ignored\n",(data & 1) ? "off" : "on");
			break;
		/*CD (SH-1) ON/OFF,guess that this is needed for Sports Fishing games...*/
		//case 0x08:
		//case 0x09:
		case 0x0d:
			if(LOG_SMPC) printf ("SMPC: System Reset\n");
			smpc_system_reset(space->machine());
			break;
		case 0x0e:
		case 0x0f:
			if(LOG_SMPC) printf ("SMPC: Change Clock to %s\n",data & 1 ? "320" : "352");
			smpc_change_clock(space->machine(),data & 1);
			break;
		/*"Interrupt Back"*/
		case 0x10:
			if(LOG_SMPC) printf ("SMPC: Status Acquire\n");
			space->machine().scheduler().timer_set(attotime::from_msec(16), FUNC(stv_smpc_intback),0); //TODO: variable time
			break;
		/* RTC write*/
		case 0x16:
			if(LOG_SMPC) printf("SMPC: RTC write\n");
			smpc_rtc_write(space->machine());
			break;
		/* SMPC memory setting*/
		case 0x17:
			if(LOG_SMPC) printf ("SMPC: memory setting\n");
			//smpc_memory_setting(space->machine());
			break;
		case 0x18:
			if(LOG_SMPC) printf ("SMPC: NMI request\n");
			smpc_nmi_req(space->machine());
			break;
		case 0x19:
		case 0x1a:
			if(LOG_SMPC) printf ("SMPC: NMI %sable\n",data & 1 ? "Dis" : "En");
			smpc_nmi_set(space->machine(),data & 1);
			break;
		default:
			printf ("cpu '%s' (PC=%08X) SMPC: undocumented Command %02x\n", space->device().tag(), cpu_get_pc(&space->device()), data);
	}
}

WRITE8_HANDLER( stv_SMPC_w )
{
	saturn_state *state = space->machine().driver_data<saturn_state>();

	if (!(offset & 1)) // avoid writing to even bytes
		return;

//  if(LOG_SMPC) printf ("8-bit SMPC Write to Offset %02x with Data %02x\n", offset, data);

	if(offset >= 1 && offset <= 0xd)
		state->m_smpc.IREG[offset >> 1] = data;

	if (offset == 0x1f) // COMREG
	{
		stv_comreg_exec(space,data);

		// we've processed the command, clear status flag
		if(data != 0x10 && data != 0x02 && data != 0x03)
		{
			state->m_smpc.OREG[31] = data; //read-back command
			state->m_smpc.SF = 0x00;
		}
		/*TODO:emulate the timing of each command...*/
	}

	if(offset == 0x63)
		state->m_smpc.SF = data & 1;

	if(offset == 0x75)
	{
		/*
        -xx- ---- PDR1
        ---x ---- EEPROM write bit
        ---- x--- EEPROM CLOCK line
        ---- -x-- EEPROM CS line
        ---- --xx A-Bus bank bits
        */
		eeprom_device *eeprom = space->machine().device<eeprom_device>("eeprom");
		eeprom->set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
		eeprom->write_bit(data & 0x10);
		eeprom->set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
		state->m_stv_multi_bank = data & 3;

		stv_select_game(space->machine(), state->m_stv_multi_bank);

		state->m_smpc.PDR1 = (data & 0x60);
	}

	if(offset == 0x77)
	{
		/*
            -xx- ---- PDR2
            ---x ---- Enable Sound System (ACTIVE LOW)
        */
		//popmessage("PDR2 = %02x",state->m_smpc_ram[0x77]);

		if(LOG_SMPC) printf("SMPC: M68k %s\n",(data & 0x10) ? "off" : "on");
		//space->machine().scheduler().timer_set(attotime::from_usec(100), FUNC(smpc_sound_enable),(state->m_smpc_ram[0x77] & 0x10) >> 4);
		device_set_input_line(state->m_audiocpu, INPUT_LINE_RESET, (data & 0x10) ? ASSERT_LINE : CLEAR_LINE);
		state->m_en_68k = ((data & 0x10) >> 4) ^ 1;

		//if(LOG_SMPC) printf("SMPC: ram [0x77] = %02x\n",data);
		state->m_smpc.PDR2 = (data & 0x60);
	}

	if(offset == 0x7d)
	{
		/*
        ---- --x- IOSEL2 direct (1) / control mode (0) port select
        ---- ---x IOSEL1 direct (1) / control mode (0) port select
        */
		state->m_smpc.IOSEL1 = (data & 1) >> 0;
		state->m_smpc.IOSEL2 = (data & 2) >> 1;
	}

	if(offset == 0x7f)
	{
		//enable PAD irq & VDP2 external latch for port 1/2
		state->m_smpc.EXLE1 = (data & 1) >> 0;
		state->m_smpc.EXLE2 = (data & 2) >> 1;
	}
}

/********************************************
 *
 * Saturn handlers
 *
 *******************************************/

READ8_HANDLER( saturn_SMPC_r )
{
	saturn_state *state = space->machine().driver_data<saturn_state>();
	UINT8 return_data = 0;

	if (!(offset & 1)) // avoid reading to even bytes (TODO: is it 0s or 1s?)
		return 0x00;

	if(offset >= 0x21 && offset <= 0x5f)
		return_data = state->m_smpc.OREG[(offset-0x21) >> 1];

	if (offset == 0x61)
		return_data = state->m_smpc.SR;

	if (offset == 0x63)
		return_data = state->m_smpc.SF;

	if (offset == 0x75 || offset == 0x77)//PDR1/2 read
	{
		if ((state->m_smpc.IOSEL1 && offset == 0x75) || (state->m_smpc.IOSEL2 && offset == 0x77))
		{
			int hshake;
			const int shift_bit[4] = { 4, 12, 8, 0 };
			const char *const padnames[] = { "JOY1", "JOY2" };

			if(input_port_read(space->machine(), "INPUT_TYPE") && !(space->debugger_access()))
			{
				popmessage("Warning: read with SH-2 direct mode with a non-pad device");
				return 0;
			}

			if(offset == 0x75)
				hshake = (state->m_smpc.PDR1>>5) & 3;
			else
				hshake = (state->m_smpc.PDR2>>5) & 3;

			if (LOG_SMPC) logerror("SMPC: SH-2 direct mode, returning data for phase %d\n", hshake);

			return_data = 0x80 | 0x10 | ((input_port_read(space->machine(), padnames[offset == 0x77])>>shift_bit[hshake]) & 0xf);
		}
	}

	if (LOG_SMPC) logerror ("cpu %s (PC=%08X) SMPC: Read from Byte Offset %02x (%d) Returns %02x\n", space->device().tag(), cpu_get_pc(&space->device()), offset, offset>>1, return_data);


	return return_data;
}

static void saturn_comreg_exec(address_space *space,UINT8 data)
{
	saturn_state *state = space->machine().driver_data<saturn_state>();

	switch (data)
	{
		case 0x00:
			if(LOG_SMPC) printf ("SMPC: Master ON\n");
			smpc_master_on(space->machine());
			break;
		//in theory 0x01 is for Master OFF
		case 0x02:
		case 0x03:
			if(LOG_SMPC) printf ("SMPC: Slave %s\n",(data & 1) ? "off" : "on");
			space->machine().scheduler().timer_set(attotime::from_usec(100), FUNC(smpc_slave_enable),data & 1);
			break;
		case 0x06:
		case 0x07:
			if(LOG_SMPC) printf ("SMPC: Sound %s\n",(data & 1) ? "off" : "on");
			space->machine().scheduler().timer_set(attotime::from_usec(100), FUNC(smpc_sound_enable),data & 1);
			break;
		/*CD (SH-1) ON/OFF,guess that this is needed for Sports Fishing games...*/
		//case 0x08:
		//case 0x09:
		case 0x0d:
			if(LOG_SMPC) printf ("SMPC: System Reset\n");
			smpc_system_reset(space->machine());
			break;
		case 0x0e:
		case 0x0f:
			if(LOG_SMPC) printf ("SMPC: Change Clock to %s\n",data & 1 ? "320" : "352");
			smpc_change_clock(space->machine(),data & 1);
			break;
		/*"Interrupt Back"*/
		case 0x10:
			if(LOG_SMPC) printf ("SMPC: Status Acquire (IntBack)\n");
			int timing;

			timing = 100;

			if(state->m_smpc.IREG[0] != 0) // non-peripheral data
				timing = 200;

			if(state->m_smpc.IREG[1] & 8) // peripheral data
				timing = 15000;

			/* TODO: check if IREG[2] is setted to 0xf0 */

			if(LOG_PAD_CMD) printf("INTBACK %02x %02x\n",state->m_smpc.IREG[0],state->m_smpc.IREG[1]);
			space->machine().scheduler().timer_set(attotime::from_usec(timing), FUNC(saturn_smpc_intback),0); //TODO: is variable time correct?
			break;
		/* RTC write*/
		case 0x16:
			if(LOG_SMPC) printf("SMPC: RTC write\n");
			smpc_rtc_write(space->machine());
			break;
		/* SMPC memory setting*/
		case 0x17:
			if(LOG_SMPC) printf ("SMPC: memory setting\n");
			smpc_memory_setting(space->machine());
			break;
		case 0x18:
			if(LOG_SMPC) printf ("SMPC: NMI request\n");
			smpc_nmi_req(space->machine());
			break;
		case 0x19:
		case 0x1a:
			if(LOG_SMPC) printf ("SMPC: NMI %sable\n",data & 1 ? "Dis" : "En");
			smpc_nmi_set(space->machine(),data & 1);
			break;
		default:
			printf ("cpu %s (PC=%08X) SMPC: undocumented Command %02x\n", space->device().tag(), cpu_get_pc(&space->device()), data);
	}
}

WRITE8_HANDLER( saturn_SMPC_w )
{
	saturn_state *state = space->machine().driver_data<saturn_state>();

	if (LOG_SMPC) logerror ("8-bit SMPC Write to Offset %02x (reg %d) with Data %02x\n", offset, offset>>1, data);

	if (!(offset & 1)) // avoid writing to even bytes
		return;

	if(offset >= 1 && offset <= 0xd)
		state->m_smpc.IREG[offset >> 1] = data;

	if(offset == 1) //IREG0, check if a BREAK / CONTINUE request for INTBACK command
	{
		if(state->m_smpc.intback_stage)
		{
			if(data & 0x40)
			{
				if(LOG_PAD_CMD) printf("SMPC: BREAK request\n");
				state->m_smpc.SR &= 0x0f;
				state->m_smpc.intback_stage = 0;
			}
			else if(data & 0x80)
			{
				if(LOG_PAD_CMD) printf("SMPC: CONTINUE request\n");
				space->machine().scheduler().timer_set(attotime::from_usec(200), FUNC(intback_peripheral),0); /* TODO: is timing correct? */
				state->m_smpc.OREG[31] = 0x10;
				//state->m_smpc.SF = 0x01; //TODO: set hand-shake flag?
			}
		}
	}

	if (offset == 0x1f)
	{
		saturn_comreg_exec(space,data);

		// we've processed the command, clear status flag
		if(data != 0x10 && data != 2 && data != 3 && data != 6 && data != 7)
		{
			state->m_smpc.OREG[31] = data; //read-back for last command issued
			state->m_smpc.SF = 0x00; //clear hand-shake flag
		}
		/*TODO:emulate the timing of each command...*/
	}

	if (offset == 0x63)
		state->m_smpc.SF = data & 1; // hand-shake flag

	if(offset == 0x75)	// PDR1
		state->m_smpc.PDR1 = (data & state->m_smpc.DDR1);

	if(offset == 0x77)	// PDR2
		state->m_smpc.PDR2 = (data & state->m_smpc.DDR2);

	if(offset == 0x79)
		state->m_smpc.DDR1 = data & 0x7f;

	if(offset == 0x7b)
		state->m_smpc.DDR2 = data & 0x7f;

	if(offset == 0x7d)
	{
		state->m_smpc.IOSEL1 = data & 1;
		state->m_smpc.IOSEL2 = (data & 2) >> 1;
	}

	if(offset == 0x7f)
	{
		//enable PAD irq & VDP2 external latch for port 1/2
		state->m_smpc.EXLE1 = (data & 1) >> 0;
		state->m_smpc.EXLE2 = (data & 2) >> 1;
	}
}
