/*************************************************************************

    Irem M92 hardware

*************************************************************************/

typedef struct _pf_layer_info pf_layer_info;
struct _pf_layer_info
{
	tilemap_t *		tmap;
	tilemap_t *		wide_tmap;
	UINT16			vram_base;
	UINT16			control[4];
};

class m92_state : public driver_device
{
public:
	m92_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_irqvector;
	UINT16 m_sound_status;
	UINT32 m_bankaddress;
	emu_timer *m_scanline_timer;
	UINT8 m_irq_vectorbase;
	UINT32 m_raster_irq_position;
	UINT16 *m_vram_data;
	UINT16 *m_spritecontrol;
	UINT8 m_sprite_buffer_busy;
	UINT8 m_game_kludge;
	pf_layer_info m_pf_layer[3];
	UINT16 m_pf_master_control[4];
	INT32 m_sprite_list;
	int m_palette_bank;
};


/*----------- defined in drivers/m92.c -----------*/

extern void m92_sprite_interrupt(running_machine &machine);


/*----------- defined in video/m92.c -----------*/

WRITE16_HANDLER( m92_spritecontrol_w );
WRITE16_HANDLER( m92_videocontrol_w );
READ16_HANDLER( m92_paletteram_r );
WRITE16_HANDLER( m92_paletteram_w );
WRITE16_HANDLER( m92_vram_w );
WRITE16_HANDLER( m92_pf1_control_w );
WRITE16_HANDLER( m92_pf2_control_w );
WRITE16_HANDLER( m92_pf3_control_w );
WRITE16_HANDLER( m92_master_control_w );

VIDEO_START( m92 );
SCREEN_UPDATE( m92 );
