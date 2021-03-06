/**********************************************************************

    Motorola MC6845 and compatible CRT controller emulation

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __MC6845__
#define __MC6845__

#include "emu.h"


#define MC6845_INTERFACE(name) \
	const mc6845_interface (name) =

#define MCFG_MC6845_ADD(_tag, _variant, _screen_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, _variant, _clock) \
	MCFG_VIDEO_SET_SCREEN(_screen_tag) \
	MCFG_DEVICE_CONFIG(_config)

#define MCFG_MOS8563_ADD(_tag, _screen_tag, _clock, _config, _map) \
	MCFG_DEVICE_ADD(_tag, MOS8563, _clock) \
	MCFG_VIDEO_SET_SCREEN(_screen_tag) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(60) \
	MCFG_SCREEN_SIZE(640, 200) \
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos8563_device, screen_update)

#define MCFG_MOS8568_ADD(_tag, _screen_tag, _clock, _config, _map) \
	MCFG_DEVICE_ADD(_tag, MOS8568, _clock) \
	MCFG_VIDEO_SET_SCREEN(_screen_tag) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(60) \
	MCFG_SCREEN_SIZE(640, 200) \
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos8568_device, screen_update)


class mc6845_device;

/* callback definitions */
typedef void * (*mc6845_begin_update_func)(mc6845_device *device, bitmap_rgb32 &bitmap, const rectangle &cliprect);
#define MC6845_BEGIN_UPDATE(name)   void *name(mc6845_device *device, bitmap_rgb32 &bitmap, const rectangle &cliprect)


typedef void (*mc6845_update_row_func)(mc6845_device *device, bitmap_rgb32 &bitmap,
										const rectangle &cliprect, UINT16 ma, UINT8 ra,
										UINT16 y, UINT8 x_count, INT8 cursor_x, int de, int hbp, int vbp, void *param);


#define MC6845_UPDATE_ROW(name)     void name(mc6845_device *device, bitmap_rgb32 &bitmap,  \
												const rectangle &cliprect, UINT16 ma, UINT8 ra, \
												UINT16 y, UINT8 x_count, INT8 cursor_x, int de, int hbp, int vbp, void *param)


typedef void (*mc6845_end_update_func)(mc6845_device *device, bitmap_rgb32 &bitmap, const rectangle &cliprect, void *param);
#define MC6845_END_UPDATE(name)     void name(mc6845_device *device, bitmap_rgb32 &bitmap, const rectangle &cliprect, void *param)


typedef void (*mc6845_on_update_addr_changed_func)(mc6845_device *device, int address, int strobe);
#define MC6845_ON_UPDATE_ADDR_CHANGED(name) void name(mc6845_device *device, int address, int strobe)


/* interface */
struct mc6845_interface
{
	bool m_show_border_area;        /* visible screen area (false) active display (true) active display + blanking */

	/* visible screen area adjustment */
	int m_visarea_adjust_min_x;
	int m_visarea_adjust_max_x;
	int m_visarea_adjust_min_y;
	int m_visarea_adjust_max_y;

	int m_hpixels_per_column;       /* number of pixels per video memory address */

	/* if specified, this gets called before any pixel update,
	   optionally return a pointer that will be passed to the
	   update and tear down callbacks */
	mc6845_begin_update_func        m_begin_update;

	/* this gets called for every row, the driver must output
	   x_count * hpixels_per_column pixels.
	   cursor_x indicates the character position where the cursor is, or -1
	   if there is no cursor on this row */
	mc6845_update_row_func          m_update_row;

	/* if specified, this gets called after all row updating is complete */
	mc6845_end_update_func          m_end_update;

	/* if specified, this gets called for every change of the disply enable pin (pin 18) */
	devcb_write_line            m_out_de_func;

	/* if specified, this gets called for every change of the cursor pin (pin 19) */
	devcb_write_line            m_out_cur_func;

	/* if specified, this gets called for every change of the HSYNC pin (pin 39) */
	devcb_write_line            m_out_hsync_func;

	/* if specified, this gets called for every change of the VSYNC pin (pin 40) */
	devcb_write_line            m_out_vsync_func;

	/* Called whenever the update address changes
	 * For vblank/hblank timing strobe indicates the physical update.
	 * vblank/hblank timing not supported yet! */

	mc6845_on_update_addr_changed_func  m_on_update_addr_changed;
};


class mc6845_device :   public device_t,
						public device_video_interface,
						public mc6845_interface
{
	friend class mc6845_1_device;
	friend class r6545_1_device;
	friend class c6545_1_device;
	friend class h46505_device;
	friend class hd6845_device;
	friend class sy6545_1_device;
	friend class sy6845e_device;
	friend class hd6345_device;
	friend class ams40041_device;

public:
	// construction/destruction
	mc6845_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	mc6845_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);

	/* select one of the registers for reading or writing */
	DECLARE_WRITE8_MEMBER( address_w );

	/* read from the status register */
	DECLARE_READ8_MEMBER( status_r );

	/* read from the currently selected register */
	DECLARE_READ8_MEMBER( register_r );

	/* write to the currently selected register */
	DECLARE_WRITE8_MEMBER( register_w );

	// read display enable line state
	DECLARE_READ_LINE_MEMBER( de_r );

	// read cursor line state
	DECLARE_READ_LINE_MEMBER( cursor_r );

	// read horizontal sync line state
	DECLARE_READ_LINE_MEMBER( hsync_r );

	// read vertical sync line state
	DECLARE_READ_LINE_MEMBER( vsync_r );

	/* return the current value on the MA0-MA13 pins */
	UINT16 get_ma();

	/* return the current value on the RA0-RA4 pins */
	UINT8 get_ra();

	/* simulates the LO->HI clocking of the light pen pin (pin 3) */
	void assert_light_pen_input();

	/* set the clock (pin 21) of the chip */
	void set_clock(int clock);

	/* set number of pixels per video memory address */
	void set_hpixels_per_column(int hpixels_per_column);

	/* updates the screen -- this will call begin_update(),
	   followed by update_row() reapeatedly and after all row
	   updating is complete, end_update() */
	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
	virtual void device_post_load();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	bool m_supports_disp_start_addr_r;
	bool m_supports_vert_sync_width;
	bool m_supports_status_reg_d5;
	bool m_supports_status_reg_d6;
	bool m_supports_status_reg_d7;
	bool m_supports_transparent;

	devcb_resolved_write_line   m_res_out_de_func;
	devcb_resolved_write_line   m_res_out_cur_func;
	devcb_resolved_write_line   m_res_out_hsync_func;
	devcb_resolved_write_line   m_res_out_vsync_func;

	/* register file */
	UINT8   m_horiz_char_total;     /* 0x00 */
	UINT8   m_horiz_disp;           /* 0x01 */
	UINT8   m_horiz_sync_pos;       /* 0x02 */
	UINT8   m_sync_width;           /* 0x03 */
	UINT8   m_vert_char_total;      /* 0x04 */
	UINT8   m_vert_total_adj;       /* 0x05 */
	UINT8   m_vert_disp;            /* 0x06 */
	UINT8   m_vert_sync_pos;        /* 0x07 */
	UINT8   m_mode_control;         /* 0x08 */
	UINT8   m_max_ras_addr;         /* 0x09 */
	UINT8   m_cursor_start_ras;     /* 0x0a */
	UINT8   m_cursor_end_ras;       /* 0x0b */
	UINT16  m_disp_start_addr;      /* 0x0c/0x0d */
	UINT16  m_cursor_addr;          /* 0x0e/0x0f */
	UINT16  m_light_pen_addr;       /* 0x10/0x11 */
	UINT16  m_update_addr;          /* 0x12/0x13 */

	/* other internal state */
	UINT8   m_register_address_latch;
	bool    m_cursor_state;
	UINT8   m_cursor_blink_count;
	bool    m_update_ready_bit;
	/* output signals */
	int     m_cur;
	int     m_hsync;
	int     m_vsync;
	int     m_de;

	/* internal counters */
	UINT8   m_character_counter;        /* Not used yet */
	UINT8   m_hsync_width_counter;  /* Not used yet */
	UINT8   m_line_counter;
	UINT8   m_raster_counter;
	UINT8   m_adjust_counter;
	UINT8   m_vsync_width_counter;

	bool    m_line_enable_ff;       /* Internal flip flop which is set when the line_counter is reset and reset when vert_disp is reached */
	UINT8   m_vsync_ff;
	UINT8   m_adjust_active;
	UINT16  m_line_address;
	INT16   m_cursor_x;

	/* timers */
	static const device_timer_id TIMER_LINE = 0;
	static const device_timer_id TIMER_DE_OFF = 1;
	static const device_timer_id TIMER_CUR_ON = 2;
	static const device_timer_id TIMER_CUR_OFF = 3;
	static const device_timer_id TIMER_HSYNC_ON = 4;
	static const device_timer_id TIMER_HSYNC_OFF = 5;
	static const device_timer_id TIMER_LIGHT_PEN_LATCH = 6;
	static const device_timer_id TIMER_UPD_ADR = 7;
	static const device_timer_id TIMER_UPD_TRANS = 8;

	emu_timer *m_line_timer;
	emu_timer *m_de_off_timer;
	emu_timer *m_cur_on_timer;
	emu_timer *m_cur_off_timer;
	emu_timer *m_hsync_on_timer;
	emu_timer *m_hsync_off_timer;
	emu_timer *m_light_pen_latch_timer;
	emu_timer *m_upd_adr_timer;
	emu_timer *m_upd_trans_timer;

	/* computed values - do NOT state save these! */
	/* These computed are used to define the screen parameters for a driver */
	UINT16  m_horiz_pix_total;
	UINT16  m_vert_pix_total;
	UINT16  m_max_visible_x;
	UINT16  m_max_visible_y;
	UINT16  m_hsync_on_pos;
	UINT16  m_hsync_off_pos;
	UINT16  m_vsync_on_pos;
	UINT16  m_vsync_off_pos;
	bool    m_has_valid_parameters;

	UINT16   m_current_disp_addr;   /* the display address currently drawn (used only in mc6845_update) */

	bool     m_light_pen_latched;
	attotime m_upd_time;

	void update_upd_adr_timer();
	void call_on_update_address(int strobe);
	void recompute_parameters(bool postload);
	void update_counters();
	void set_de(int state);
	void set_hsync(int state);
	void set_vsync(int state);
	void set_cur(int state);
	void handle_line_timer();
	virtual void update_cursor_state();
	virtual UINT8 draw_scanline(int y, bitmap_rgb32 &bitmap, const rectangle &cliprect, void *param);
};


class mc6845_1_device : public mc6845_device
{
public:
	mc6845_1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class r6545_1_device : public mc6845_device
{
public:
	r6545_1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class c6545_1_device : public mc6845_device
{
public:
	c6545_1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class h46505_device : public mc6845_device
{
public:
	h46505_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class hd6845_device : public mc6845_device
{
public:
	hd6845_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class sy6545_1_device : public mc6845_device
{
public:
	sy6545_1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class sy6845e_device : public mc6845_device
{
public:
	sy6845e_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class hd6345_device : public mc6845_device
{
public:
	hd6345_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class ams40041_device : public mc6845_device
{
public:
	ams40041_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	virtual void device_start();
	virtual void device_reset();
};

class mos8563_device : public mc6845_device,
						public device_memory_interface
{
public:
	mos8563_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);
	mos8563_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

	DECLARE_PALETTE_INIT(mos8563);

	DECLARE_WRITE8_MEMBER( address_w );
	DECLARE_READ8_MEMBER( status_r );
	DECLARE_READ8_MEMBER( register_r );
	DECLARE_WRITE8_MEMBER( register_w );

	inline UINT8 read_videoram(offs_t offset);
	inline void write_videoram(offs_t offset, UINT8 data);

	void update_row(bitmap_rgb32 &bitmap, const rectangle &cliprect, UINT16 ma, UINT8 ra, UINT16 y, UINT8 x_count, INT8 cursor_x, int de, int hbp, int vbp, void *param);
	static MC6845_UPDATE_ROW( vdc_update_row );

protected:
	// device-level overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	const address_space_config      m_videoram_space_config;
	required_device<palette_device> m_palette;

	UINT8 m_char_buffer[80];
	UINT8 m_attr_buffer[80];

	bool    m_char_blink_state;
	UINT8   m_char_blink_count;

	/* register file */
	UINT16  m_attribute_addr;       /* 0x14/0x15 */
	UINT8   m_horiz_char;           /* 0x16 */
	UINT8   m_vert_char_disp;       /* 0x17 */
	UINT8   m_vert_scroll;          /* 0x18 */
	UINT8   m_horiz_scroll;         /* 0x19 */
	UINT8   m_color;                /* 0x1a */
	UINT8   m_row_addr_incr;        /* 0x1b */
	UINT8   m_char_base_addr;       /* 0x1c */
	UINT8   m_underline_ras;        /* 0x1d */
	UINT8   m_word_count;           /* 0x1e */
	UINT8   m_data;                 /* 0x1f */
	UINT16  m_block_addr;           /* 0x20/0x21 */
	UINT16  m_de_begin;             /* 0x22/0x23 */
	UINT8   m_dram_refresh;         /* 0x24 */
	UINT8   m_sync_polarity;        /* 0x25 */

	int m_revision;

	virtual void update_cursor_state();
	virtual UINT8 draw_scanline(int y, bitmap_rgb32 &bitmap, const rectangle &cliprect, void *param);

	static const device_timer_id TIMER_BLOCK_COPY = 9;

	emu_timer *m_block_copy_timer;
};

class mos8568_device : public mos8563_device
{
public:
	mos8568_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
};


extern const device_type MC6845;
extern const device_type MC6845_1;
extern const device_type R6545_1;
extern const device_type C6545_1;
extern const device_type H46505;
extern const device_type HD6845;
extern const device_type SY6545_1;
extern const device_type SY6845E;
extern const device_type HD6345;
extern const device_type AMS40041;
extern const device_type MOS8563;
extern const device_type MOS8568;



#endif
