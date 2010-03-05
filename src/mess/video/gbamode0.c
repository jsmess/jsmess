 /***************************************************************************

  gbamode0.c

  Handles GBA mode 0 screen rendering

  By R. Belmont & Harmony

***************************************************************************/

static void draw_mode0_scanline(running_machine *machine, gba_state *state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)state->gba_pram)[0] | 0x30000000;

	draw_bg_scanline(state, line0, y, DISPCNT_BG0_EN, state->BG0CNT, state->BG0HOFS, state->BG0VOFS);
	draw_bg_scanline(state, line1, y, DISPCNT_BG1_EN, state->BG1CNT, state->BG1HOFS, state->BG1VOFS);
	draw_bg_scanline(state, line2, y, DISPCNT_BG2_EN, state->BG2CNT, state->BG2HOFS, state->BG2VOFS);
	draw_bg_scanline(state, line3, y, DISPCNT_BG3_EN, state->BG3CNT, state->BG3HOFS, state->BG3VOFS);
	draw_gba_oam(state, machine, lineOBJ, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;

		if(line0[x] < color)
		{
			color = line0[x];
			top = 0x01;
		}

		if((UINT8)(line1[x] >> 24) < (UINT8)(color >> 24))
		{
			color = line1[x];
			top = 0x02;
		}

		if((UINT8)(line2[x] >> 24) < (UINT8)(color >> 24))
		{
			color = line2[x];
			top = 0x04;
		}

		if((UINT8)(line3[x] >> 24) < (UINT8)(color >> 24))
		{
			color = line3[x];
			top = 0x08;
		}

		if((UINT8)(lineOBJ[x] >> 24) < (UINT8)(color >> 24))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if(top == 0x10 && (color & 0x00010000) != 0)
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if((UINT8)(line0[x] >> 24) < (UINT8)(back >> 24))
			{
				back = line0[x];
				top2 = 0x01;
			}

			if((UINT8)(line1[x] >> 24) < (UINT8)(back >> 24))
			{
				back = line1[x];
				top2 = 0x02;
			}

			if((UINT8)(line2[x] >> 24) < (UINT8)(back >> 24))
			{
				back = line2[x];
				top2 = 0x04;
			}

			if((UINT8)(line3[x] >> 24) < (UINT8)(back >> 24))
			{
				back = line3[x];
				top2 = 0x08;
			}

			if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & state->BLDCNT)
						{
							color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}

		lineMix[x] = color;
	}
}

static void draw_mode0_scanline_nowindow(running_machine *machine, gba_state *state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)state->gba_pram)[0] | 0x30000000;
	int effect = state->BLDCNT & BLDCNT_SFX;

	draw_bg_scanline(state, line0, y, DISPCNT_BG0_EN, state->BG0CNT, state->BG0HOFS, state->BG0VOFS);
	draw_bg_scanline(state, line1, y, DISPCNT_BG1_EN, state->BG1CNT, state->BG1HOFS, state->BG1VOFS);
	draw_bg_scanline(state, line2, y, DISPCNT_BG2_EN, state->BG2CNT, state->BG2HOFS, state->BG2VOFS);
	draw_bg_scanline(state, line3, y, DISPCNT_BG3_EN, state->BG3CNT, state->BG3HOFS, state->BG3VOFS);
	draw_gba_oam(state, machine, lineOBJ, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;

		if(line0[x] < color)
		{
			color = line0[x];
			top = 0x01;
		}

		if(line1[x] < (color & 0xff000000))
		{
			color = line1[x];
			top = 0x02;
		}

		if(line2[x] < (color & 0xff000000))
		{
			color = line2[x];
			top = 0x04;
		}

		if(line3[x] < (color & 0xff000000))
		{
			color = line3[x];
			top = 0x08;
		}

		if(lineOBJ[x] < (color & 0xff000000))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if((color & 0x00010000) == 0)
		{
			switch(effect)
			{
				case BLDCNT_SFX_NONE:
					break;
				case BLDCNT_SFX_ALPHA:
					if(state->BLDCNT & top)
					{
						UINT32 back = backdrop;
						UINT8 top2 = 0x20;

						if(line0[x] < back)
						{
							if(top != 0x01)
							{
								back = line0[x];
								top2 = 0x01;
							}
						}

						if(line1[x] < (back & 0xff000000))
						{
							if(top != 0x02)
							{
								back = line1[x];
								top2 = 0x02;
							}
						}

						if(line2[x] < (back & 0xff000000))
						{
							if(top != 0x04)
							{
								back = line2[x];
								top2 = 0x04;
							}
						}

						if(line3[x] < (back & 0xff000000))
						{
							if(top != 0x08)
							{
								back = line3[x];
								top2 = 0x08;
							}
						}

						if(lineOBJ[x] < (back & 0xff000000))
						{
							if(top != 0x10)
							{
								back = lineOBJ[x];
								top2 = 0x10;
							}
						}

						if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
						{
							color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
						}
					}
					break;
				case BLDCNT_SFX_LIGHTEN:
					if(top & state->BLDCNT)
					{
						color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
					}
					break;
				case BLDCNT_SFX_DARKEN:
					if(top & state->BLDCNT)
					{
						color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
					}
					break;
			}
		}
		else
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if(line0[x] < back)
			{
				back = line0[x];
				top2 = 0x01;
			}

			if(line1[x] < (back & 0xff000000))
			{
				back = line1[x];
				top2 = 0x02;
			}

			if(line2[x] < (back & 0xff000000))
			{
				back = line2[x];
				top2 = 0x04;
			}

			if(line3[x] < (back & 0xff000000))
			{
				back = line3[x];
				top2 = 0x08;
			}

			if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & state->BLDCNT)
						{
							color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}
		lineMix[x] = color;
	}
}

static void draw_mode0_scanline_all(running_machine *machine, gba_state *state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)state->gba_pram)[0] | 0x30000000;
	int inWindow0 = 0;
	int inWindow1 = 0;
	UINT8 inWin0Mask = state->WININ & 0x00ff;
	UINT8 inWin1Mask = state->WININ >> 8;
	UINT8 outMask = state->WINOUT & 0x00ff;

	if(state->DISPCNT & DISPCNT_WIN0_EN)
	{
		UINT8 v0 = state->WIN0V >> 8;
		UINT8 v1 = state->WIN0V & 0x00ff;
		inWindow0 = ((v0 == v1) && (v0 >= 0xe8)) ? 1 : 0;
		if(v1 >= v0)
		{
			inWindow0 |= (y >= v0 && y < v1) ? 1 : 0;
		}
		else
		{
			inWindow0 |= (y >= v0 || y < v1) ? 1 : 0;
		}
	}

	if(state->DISPCNT & DISPCNT_WIN1_EN)
	{
		UINT8 v0 = state->WIN1V >> 8;
		UINT8 v1 = state->WIN1V & 0x00ff;
		inWindow1 = ((v0 == v1) && (v0 >= 0xe8)) ? 1 : 0;
		if(v1 >= v0)
		{
			inWindow1 |= (y >= v0 && y < v1) ? 1 : 0;
		}
		else
		{
			inWindow1 |= (y >= v0 || y < v1) ? 1 : 0;
		}
	}

	draw_bg_scanline(state, line0, y, DISPCNT_BG0_EN, state->BG0CNT, state->BG0HOFS, state->BG0VOFS);
	draw_bg_scanline(state, line1, y, DISPCNT_BG1_EN, state->BG1CNT, state->BG1HOFS, state->BG1VOFS);
	draw_bg_scanline(state, line2, y, DISPCNT_BG2_EN, state->BG2CNT, state->BG2HOFS, state->BG2VOFS);
	draw_bg_scanline(state, line3, y, DISPCNT_BG3_EN, state->BG3CNT, state->BG3HOFS, state->BG3VOFS);
	draw_gba_oam(state, machine, lineOBJ, y);
	draw_gba_oam_window(state, machine, lineOBJWin, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;
		UINT8 mask = outMask;

		if((lineOBJWin[x] & 0x80000000) == 0)
		{
			mask = state->WINOUT >> 8;
		}

		if(inWindow1)
		{
			if(is_in_window(state, x, 1))
			{
				mask = inWin1Mask;
			}
		}

		if(inWindow0)
		{
			if(is_in_window(state, x, 0))
			{
				mask = inWin0Mask;
			}
		}

		if((mask & 0x01) != 0 && (line0[x] < color))
		{
			color = line0[x];
			top = 0x01;
		}

		if((mask & 0x02) != 0 && ((UINT8)(line1[x] >> 24) < (UINT8)(color >> 24)))
		{
			color = line1[x];
			top = 0x02;
		}

		if((mask & 0x04) != 0 && ((UINT8)(line2[x] >> 24) < (UINT8)(color >> 24)))
		{
			color = line2[x];
			top = 0x04;
		}

		if((mask & 0x08) != 0 && ((UINT8)(line3[x] >> 24) < (UINT8)(color >> 24)))
		{
			color = line3[x];
			top = 0x08;
		}

		if((mask & 0x10) != 0 && ((UINT8)(lineOBJ[x] >> 24) < (UINT8)(color >> 24)))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if((mask & 0x20) != 0)
		{
			if((color & 0x00010000) == 0)
			{
				switch(state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_NONE:
						break;
					case BLDCNT_SFX_ALPHA:
					{
						if(top & state->BLDCNT)
						{
							UINT32 back = backdrop;
							UINT8 top2 = 0x20;
							if((mask & 0x01) != 0 && ((UINT8)(line0[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x01)
								{
									back = line0[x];
									top2 = 0x01;
								}
							}

							if((mask & 0x02) != 0 && ((UINT8)(line1[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x02)
								{
									back = line1[x];
									top2 = 0x02;
								}
							}

							if((mask & 0x04) != 0 && ((UINT8)(line2[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x04)
								{
									back = line2[x];
									top2 = 0x04;
								}
							}

							if((mask & 0x08) != 0 && ((UINT8)(line3[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x08)
								{
									back = line3[x];
									top2 = 0x08;
								}
							}

							if((mask & 0x10) != 0 && ((UINT8)(lineOBJ[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x10)
								{
									back = lineOBJ[x];
									top2 = 0x10;
								}
							}

							if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
							{
								color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
							}
						}
						break;
					}
					case BLDCNT_SFX_LIGHTEN:
						if(top & state->BLDCNT)
						{
							color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
				}
			}
			else
			{
				UINT32 back = backdrop;
				UINT8 top2 = 0x20;

				if((mask & 0x01) != 0 && ((UINT8)(line0[x] >> 24) < (UINT8)(back >> 24)))
				{
					back = line0[x];
					top2 = 0x01;
				}

				if((mask & 0x02) != 0 && ((UINT8)(line1[x] >> 24) < (UINT8)(back >> 24)))
				{
					back = line1[x];
					top2 = 0x02;
				}

				if((mask & 0x04) != 0 && ((UINT8)(line2[x] >> 24) < (UINT8)(back >> 24)))
				{
					back = line2[x];
					top2 = 0x04;
				}

				if((mask & 0x08) != 0 && ((UINT8)(line3[x] >> 24) < (UINT8)(back >> 24)))
				{
					back = line3[x];
					top2 = 0x08;
				}

				if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
				{
					color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
				}
				else
				{
					switch(state->BLDCNT & BLDCNT_SFX)
					{
						case BLDCNT_SFX_LIGHTEN:
							if(top & state->BLDCNT)
							{
								color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
							}
							break;
						case BLDCNT_SFX_DARKEN:
							if(top & state->BLDCNT)
							{
								color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
							}
							break;
					}
				}
			}
		}
		else if(color & 0x00010000)
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if((mask & 0x01) != 0 && ((UINT8)(line0[x] >> 24) < (UINT8)(back >> 24)))
			{
				back = line0[x];
				top2 = 0x01;
			}

			if((mask & 0x02) != 0 && ((UINT8)(line1[x] >> 24) < (UINT8)(back >> 24)))
			{
				back = line1[x];
				top2 = 0x02;
			}

			if((mask & 0x04) != 0 && ((UINT8)(line2[x] >> 24) < (UINT8)(back >> 24)))
			{
				back = line2[x];
				top2 = 0x04;
			}

			if((mask & 0x08) != 0 && ((UINT8)(line3[x] >> 24) < (UINT8)(back >> 24)))
			{
				back = line3[x];
				top2 = 0x08;
			}

			if(top2 & (state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[state->BLDALPHA & 0x1f], coeff[(state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & state->BLDCNT)
						{
							color = increase_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}
		lineMix[x] = color;
	}
}
