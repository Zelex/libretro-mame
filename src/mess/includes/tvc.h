// license:BSD-3-Clause
// copyright-holders:Sandro Ronco
/*****************************************************************************
 *
 *   includes/tvc.h
 *
 ****************************************************************************/

#pragma once

#ifndef TVC_H_
#define TVC_H_

#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/ram.h"
#include "bus/centronics/ctronics.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "imagedev/snapquik.h"
#include "formats/tvc_cas.h"

#include "bus/tvc/tvc.h"
#include "bus/tvc/hbf.h"

#define     TVC_RAM_BANK    1
#define     TVC_ROM_BANK    2

#define CENTRONICS_TAG  "centronics"

#define MCFG_TVC_SOUND_SNDINT_CALLBACK(_write) \
	devcb = &tvc_sound_device::set_sndint_wr_callback(*device, DEVCB2_##_write);

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> tvc_sound_device

class tvc_sound_device : public device_t,
							public device_sound_interface
{
public:
	// construction/destruction
	tvc_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _Object> static devcb2_base &set_sndint_wr_callback(device_t &device, _Object object) { return downcast<tvc_sound_device &>(device).m_write_sndint.set_callback(object); }

	DECLARE_WRITE8_MEMBER(write);
	void reset_divider();

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

private:
	static const device_timer_id TIMER_SNDINT = 0;

	sound_stream *  m_stream;
	int             m_freq;
	int             m_enabled;
	int             m_volume;
	int             m_incr;
	int             m_signal;
	UINT8           m_ports[3];
	emu_timer *     m_sndint_timer;
	devcb2_write_line   m_write_sndint;
};


// ======================> tvc_state

class tvc_state : public driver_device
{
public:
	tvc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu"),
			m_ram(*this, RAM_TAG),
			m_sound(*this, "custom"),
			m_cassette(*this, "cassette"),
			m_centronics(*this, CENTRONICS_TAG),
			m_palette(*this, "palette")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_ram;
	required_device<tvc_sound_device> m_sound;
	required_device<cassette_image_device> m_cassette;
	required_device<centronics_device> m_centronics;
	required_device<palette_device> m_palette;

	void machine_start();
	void machine_reset();

	void tvc_set_mem_page(UINT8 data);
	DECLARE_WRITE8_MEMBER(tvc_bank_w);
	DECLARE_WRITE8_MEMBER(tvc_vram_bank_w);
	DECLARE_WRITE8_MEMBER(tvc_palette_w);
	DECLARE_WRITE8_MEMBER(tvc_keyboard_w);
	DECLARE_READ8_MEMBER(tvc_keyboard_r);
	DECLARE_READ8_MEMBER(tvc_int_state_r);
	DECLARE_WRITE8_MEMBER(tvc_flipflop_w);
	DECLARE_WRITE8_MEMBER(tvc_border_color_w);
	DECLARE_WRITE8_MEMBER(tvc_sound_w);
	DECLARE_WRITE8_MEMBER(tvc_cassette_w);
	DECLARE_READ8_MEMBER(tvc_5b_r);
	DECLARE_WRITE_LINE_MEMBER(tvc_int_ff_set);
	DECLARE_WRITE_LINE_MEMBER(tvc_centronics_ack);

	// expansions
	DECLARE_WRITE8_MEMBER(tvc_expansion_w);
	DECLARE_READ8_MEMBER(tvc_expansion_r);
	DECLARE_READ8_MEMBER(tvc_exp_id_r);
	DECLARE_WRITE8_MEMBER(tvc_expint_ack_w);

	DECLARE_QUICKLOAD_LOAD_MEMBER( tvc64);

	tvcexp_slot_device * m_expansions[4];
	UINT8       m_video_mode;
	UINT8       m_keyline;
	UINT8       m_active_slot;
	UINT8       m_int_flipflop;
	UINT8       m_col[4];
	UINT8       m_bank_type[4];
	UINT8       m_bank;
	UINT8       m_vram_bank;
	UINT8       m_cassette_ff;
	UINT8       m_centronics_ff;
	DECLARE_PALETTE_INIT(tvc);
};


// device type definition
extern const device_type TVC_SOUND;

#endif /* TVC_H_ */
