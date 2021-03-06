/***************************************************************************

                            -= Kaneko 16 Bit Games =-

                    driver by   Luca Elia (l.elia@tin.it)

**************************************************************************/

#include "emu.h"
#include "includes/kaneko16.h"
#include "kan_pand.h"



WRITE16_MEMBER(kaneko16_state::kaneko16_display_enable)
{
	COMBINE_DATA(&m_disp_enable);
}

VIDEO_START_MEMBER(kaneko16_state,kaneko16)
{
	m_disp_enable = 1;  // default enabled for games not using it
}



/* Fill the bitmap with a single colour. This is wrong, but will work most of
   the times. To do it right, each pixel should be drawn with pen 0
   of the bottomost tile that covers it (which is pretty tricky to do) */
template<class _BitmapClass>
void kaneko16_state::kaneko16_fill_bitmap(palette_device* palette, _BitmapClass &bitmap, const rectangle &cliprect)
{
	int pen = 0;

	if (m_kaneko_spr)
	{
		if (m_kaneko_spr->get_sprite_type() == 1)
		{
			pen = 0x7f00;
		}
	}


	typename _BitmapClass::pixel_t *dest;
	(void)dest; // shut up Visual Studio
	if (sizeof(*dest) == 2)
	{
		bitmap.fill(pen, cliprect);
	}
	else
	{
		const pen_t *pal = palette->pens();
		bitmap.fill(pal[pen], cliprect);
	}
}




template<class _BitmapClass>
UINT32 kaneko16_state::screen_update_common(screen_device &screen, _BitmapClass &bitmap, const rectangle &cliprect)
{
	int i;

	screen.priority().fill(0, cliprect);

	if (m_view2_0) m_view2_0->kaneko16_prepare(bitmap, cliprect);
	if (m_view2_1) m_view2_1->kaneko16_prepare(bitmap, cliprect);

	for ( i = 0; i < 8; i++ )
	{
		if (m_view2_0) m_view2_0->render_tilemap_chip(screen,bitmap,cliprect,i);
		if (m_view2_1) m_view2_1->render_tilemap_chip_alt(screen,bitmap,cliprect,i, VIEW2_2_pri);
	}

	return 0;
}





UINT32 kaneko16_state::screen_update_kaneko16(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	kaneko16_fill_bitmap(m_palette, bitmap,cliprect);

	// if the display is disabled, do nothing?
	if (!m_disp_enable) return 0;

	screen_update_common(screen, bitmap, cliprect);
	m_kaneko_spr->kaneko16_render_sprites(machine(),bitmap,cliprect, screen.priority(), m_spriteram, m_spriteram.bytes());
	return 0;
}






/* Berlwall and Gals Panic have an additional hi-color layers */

PALETTE_INIT_MEMBER(kaneko16_berlwall_state,berlwall)
{
	int i;

	/* first 2048 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
		palette.set_pen_color(i,pal5bit(i >> 5),pal5bit(i >> 10),pal5bit(i >> 0));
}

VIDEO_START_MEMBER(kaneko16_berlwall_state,berlwall)
{
	int sx, x,y;
	UINT8 *RAM  =   memregion("gfx3")->base();

	/* Render the hi-color static backgrounds held in the ROMs */

	m_bg15_bitmap.allocate(256 * 32, 256 * 1);

/*
    8aba is used as background color
    8aba/2 = 455d = 10001 01010 11101 = $11 $0a $1d
*/

	for (sx = 0 ; sx < 32 ; sx++)   // horizontal screens
		for (x = 0 ; x < 256 ; x++) // horizontal pixels
		for (y = 0 ; y < 256 ; y++) // vertical pixels
		{
			int addr  = sx * (256 * 256) + x + y * 256;
			int data = RAM[addr * 2 + 0] * 256 + RAM[addr * 2 + 1];
			int r,g,b;

			r = (data & 0x07c0) >>  6;
			g = (data & 0xf800) >> 11;
			b = (data & 0x003e) >>  1;

			/* apply a simple decryption */
			r ^= 0x09;

			if (~g & 0x08) g ^= 0x10;
			g = (g - 1) & 0x1f;     /* decrease with wraparound */

			b ^= 0x03;
			if (~b & 0x08) b ^= 0x10;
			b = (b + 2) & 0x1f;     /* increase with wraparound */

			/* kludge to fix the rollercoaster picture */
			if ((r & 0x10) && (b & 0x10))
				g = (g - 1) & 0x1f;     /* decrease with wraparound */

			m_bg15_bitmap.pix16(y, sx * 256 + x) = ((g << 10) | (r << 5) | b);
		}

	VIDEO_START_CALL_MEMBER(kaneko16);
}







/* Select the high color background image (out of 32 in the ROMs) */
READ16_MEMBER(kaneko16_berlwall_state::kaneko16_bg15_select_r)
{
	return m_bg15_select[0];
}
WRITE16_MEMBER(kaneko16_berlwall_state::kaneko16_bg15_select_w)
{
	COMBINE_DATA(&m_bg15_select[0]);
}

/* ? */
READ16_MEMBER(kaneko16_berlwall_state::kaneko16_bg15_reg_r)
{
	return m_bg15_reg[0];
}
WRITE16_MEMBER(kaneko16_berlwall_state::kaneko16_bg15_reg_w)
{
	COMBINE_DATA(&m_bg15_reg[0]);
//  printf("kaneko16_bg15_reg_w %04x\n", m_bg15_reg[0]);
	double brt1 = data & 0xff;
	brt1 = brt1 / 255.0;

	for (int i = 0; i < 0x8000;i++)
		m_bgpalette->set_pen_contrast(i, brt1);
}


void kaneko16_berlwall_state::kaneko16_render_15bpp_bitmap(bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	if (m_bg15_bitmap.valid())
	{
		int select  =   m_bg15_select[ 0 ];
//      int reg     =   m_bg15_reg[ 0 ];
		int flip    =   select & 0x20;
		int sx;//, sy;

	//  if (flip)   select ^= 0x1f;

		sx      =   (select & 0x1f) * 256;
	//  sy      =   0;

		const pen_t *pal = m_bgpalette->pens();
		UINT16* srcbitmap;
		UINT32* dstbitmap;

		for (int y = cliprect.min_y; y <= cliprect.max_y; y++)
		{
			if (!flip) srcbitmap = &m_bg15_bitmap.pix16(y);
			else srcbitmap =  &m_bg15_bitmap.pix16(255-y);
			dstbitmap = &bitmap.pix32(y);

			for (int x = cliprect.min_x; x <= cliprect.max_x; x++)
			{
				UINT16 pix = srcbitmap[x + sx];
				dstbitmap[x] = pal[pix&0x7fff];
			}
		}

//      flag = 0;
	}
}
UINT32 kaneko16_berlwall_state::screen_update_berlwall(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	// berlwall uses a 15bpp bitmap as a bg, not a solid fill
	kaneko16_render_15bpp_bitmap(bitmap,cliprect);

	// if the display is disabled, do nothing?
	if (!m_disp_enable) return 0;

	screen_update_common(screen, bitmap, cliprect);
	m_kaneko_spr->kaneko16_render_sprites(machine(),bitmap,cliprect, screen.priority(), m_spriteram, m_spriteram.bytes()/2);
	return 0;
}
