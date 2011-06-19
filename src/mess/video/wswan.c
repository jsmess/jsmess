/***************************************************************************

  wswan.c

  File to handle video emulation of the Bandai WonderSwan.

  Anthony Kruize
  Wilbert Pol

***************************************************************************/

#include "emu.h"
#include "includes/wswan.h"


INLINE void wswan_plot_pixel(running_machine &machine,int x, int y, int pen) {
	wswan_state *state = machine.driver_data<wswan_state>();
	y += 5*8;
	if ( state->m_vdp.display_vertical ) {
		*BITMAP_ADDR16(machine.generic.tmpbitmap, 28*8 - 1 - x, y) = pen;
	} else {
		*BITMAP_ADDR16(machine.generic.tmpbitmap, y, x) = pen;
	}
}

INLINE void wswan_fillbitmap( running_machine &machine,int pen, rectangle *rec ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	/* Fill empty area above line 0 when filling line 0 */
	if ( state->m_vdp.current_line != 0 ) {
		rec->min_y = rec->min_y + 5*8;
	}

	/* Fill empty area below line 143 when filling line 143 */
	if ( state->m_vdp.current_line != 143 ) {
		rec->max_y = rec->max_y + 5*8;
	} else {
		rec->max_y = 28*8;
	}

	if ( state->m_vdp.display_vertical ) {
		rectangle rec2;
		rec2.min_y = 28*8 - 1 - rec->max_x;
		rec2.max_y = 28*8 - 1 - rec->min_x;
		rec2.min_x = rec->min_y;
		rec2.max_x = rec->max_y;
		bitmap_fill( machine.generic.tmpbitmap, &rec2 , pen);
	} else {
		bitmap_fill( machine.generic.tmpbitmap, rec , pen);
	}
}


static void wswan_setup_palettes(wswan_state *state) {
	int i,j;

	if ( state->m_vdp.color_mode ) {
		for( i = 0; i < 16; i++ ) {
			for( j = 0; j < 16; j++ ) {
				state->m_pal[i][j] = ( ( state->m_vdp.palette_vram[ ( i << 5 ) + j*2 + 1 ] << 8 ) | state->m_vdp.palette_vram[ ( i << 5 ) + j*2 ] ) & 0x0FFF;
			}
		}
	} else {
		for( i = 0; i < 16; i++ ) {
			state->m_pal[i][0] = state->m_ws_portram[ 0x20 + ( i << 1 ) ] & 0x07;
			state->m_pal[i][1] = ( state->m_ws_portram[ 0x20 + ( i << 1 ) ] >> 4 ) & 0x07;
			state->m_pal[i][2] = state->m_ws_portram[ 0x21 + ( i << 1 ) ] & 0x07;
			state->m_pal[i][3] = ( state->m_ws_portram[ 0x21 + ( i << 1 ) ] >> 4 ) & 0x07;
		}
	}
}

static void wswan_draw_background( running_machine &machine ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	UINT16	map_addr;
	UINT8	start_column;
	int	column;

	map_addr = state->m_vdp.layer_bg_address + ( ( ( state->m_vdp.current_line + state->m_vdp.layer_bg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( state->m_vdp.layer_bg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		int	tile_data, tile_number, tile_palette, tile_line, tile_address;
		UINT32	plane0=0, plane1=0, plane2=0, plane3=0;
		int	x, x_offset;

		tile_data = ( state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
		            | state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		tile_number = tile_data & 0x01FF;
		tile_palette = ( tile_data >> 9 ) & 0x0F;

		tile_line = ( state->m_vdp.current_line + state->m_vdp.layer_bg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			tile_line = 7 - tile_line;
		}

		if ( state->m_vdp.colors_16 ) {
			tile_address = ( ( tile_data & 0x2000 ) ? 0x8000 : 0x4000 ) + ( tile_number * 32 ) + ( tile_line << 2 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 24 ) | ( state->m_vdp.vram[ tile_address + 1 ] << 16 ) | ( state->m_vdp.vram[ tile_address + 2 ] << 8 ) | state->m_vdp.vram[ tile_address + 3 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = state->m_vdp.vram[ tile_address + 2 ] << 2;
				plane3 = state->m_vdp.vram[ tile_address + 3 ] << 3;
			}
		} else {
			tile_address = 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 8 ) | state->m_vdp.vram[ tile_address + 1 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = 0;
				plane3 = 0;
			}
		}

		for( x = 0; x < 8; x++ ) {
			int col;
			if ( state->m_vdp.tile_packed ) {
				if ( state->m_vdp.colors_16 ) {
					col = plane0 & 0x0F;
					plane0 = plane0 >> 4;
				} else {
					col = plane0 & 0x03;
					plane0 = plane0 >> 2;
				}
			} else {
				col = ( plane3 & 8 ) | ( plane2 & 4 ) | ( plane1 & 2 ) | ( plane0 & 1 );
				plane3 = plane3 >> 1;
				plane2 = plane2 >> 1;
				plane1 = plane1 >> 1;
				plane0 = plane0 >> 1;
			}
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( state->m_vdp.layer_bg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( state->m_vdp.layer_bg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
				if ( state->m_vdp.colors_16 ) {
					if ( col ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							/* Hmmmm, what should we do here... Is this correct?? */
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						}
					}
				} else {
					if ( col || !(tile_palette & 4 ) ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_vdp.main_palette[state->m_pal[tile_palette][col]] );
						}
					}
				}
			}
		}
	}
}

static void wswan_draw_foreground_0( running_machine &machine ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = state->m_vdp.layer_fg_address + ( ( ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( state->m_vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		UINT32	plane0 = 0, plane1 = 0, plane2 = 0, plane3 = 0;
		int	x, x_offset, tile_line, tile_address;
		int	tile_data = ( state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
                                    | state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		tile_line = ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			tile_line = 7 - tile_line;
		}

		if ( state->m_vdp.colors_16 ) {
			tile_address = ( ( tile_data & 0x2000 ) ? 0x8000 : 0x4000 ) + ( tile_number * 32 ) + ( tile_line << 2 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 24 ) | ( state->m_vdp.vram[ tile_address + 1 ] << 16 ) | ( state->m_vdp.vram[ tile_address + 2 ] << 8 ) | state->m_vdp.vram[ tile_address + 3 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = state->m_vdp.vram[ tile_address + 2 ] << 2;
				plane3 = state->m_vdp.vram[ tile_address + 3 ] << 3;
			}
		} else {
			tile_address = 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 8 ) | state->m_vdp.vram[ tile_address + 1 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = 0;
				plane3 = 0;
			}
		}

		for( x = 0; x < 8; x++ ) {
			int	col;
			if ( state->m_vdp.tile_packed ) {
				if ( state->m_vdp.colors_16 ) {
					col = plane0 & 0x0F;
					plane0 = plane0 >> 4;
				} else {
					col = plane0 & 0x03;
					plane0 = plane0 >> 2;
				}
			} else {
				col = ( plane3 & 8 ) | ( plane2 & 4 ) | ( plane1 & 2 ) | ( plane0 & 1 );
				plane3 = plane3 >> 1;
				plane2 = plane2 >> 1;
				plane1 = plane1 >> 1;
				plane0 = plane0 >> 1;
			}
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
				if ( state->m_vdp.colors_16 ) {
					if ( col ) {
//                      if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
//                      } else {
//                          /* Hmmmm, what should we do here... Is this correct?? */
//                          wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
//                      }
					}
				} else {
					if ( col || !(tile_palette & 4 ) ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_vdp.main_palette[state->m_pal[tile_palette][col]] );
						}
					}
				}
			}
		}
	}
}

static void wswan_draw_foreground_2( running_machine &machine ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = state->m_vdp.layer_fg_address + ( ( ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( state->m_vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		UINT32	plane0 = 0, plane1 = 0, plane2 = 0, plane3 = 0;
		int	x, x_offset, tile_line, tile_address;
		int	tile_data = ( state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
			            | state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		tile_line = ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			tile_line = 7 - tile_line;
		}

		if ( state->m_vdp.colors_16 ) {
			tile_address = ( ( tile_data & 0x2000 ) ? 0x8000 : 0x4000 ) + ( tile_number * 32 ) + ( tile_line << 2 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 24 ) | ( state->m_vdp.vram[ tile_address + 1 ] << 16 ) | ( state->m_vdp.vram[ tile_address + 2 ] << 8 ) | state->m_vdp.vram[ tile_address + 3 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = state->m_vdp.vram[ tile_address + 2 ] << 2;
				plane3 = state->m_vdp.vram[ tile_address + 3 ] << 3;
			}
		} else {
			tile_address = 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 8 ) | state->m_vdp.vram[ tile_address + 1 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = 0;
				plane3 = 0;
			}
		}

		for( x = 0; x < 8; x++ ) {
			int	col;
			if ( state->m_vdp.tile_packed ) {
				if ( state->m_vdp.colors_16 ) {
					col = plane0 & 0x0F;
					plane0 = plane0 >> 4;
				} else {
					col = plane0 & 0x03;
					plane0 = plane0 >> 2;
				}
			} else {
				col = ( plane3 & 8 ) | ( plane2 & 4 ) | ( plane1 & 2 ) | ( plane0 & 1 );
				plane3 = plane3 >> 1;
				plane2 = plane2 >> 1;
				plane1 = plane1 >> 1;
				plane0 = plane0 >> 1;
			}
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset >= state->m_vdp.window_fg_left && x_offset < state->m_vdp.window_fg_right && x_offset < WSWAN_X_PIXELS ) {
				if ( state->m_vdp.colors_16 ) {
					if ( col ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							/* Hmmmm, what should we do here... Is this correct?? */
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						}
					}
				} else {
					if ( col || !(tile_palette & 4 ) ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_vdp.main_palette[state->m_pal[tile_palette][col]] );
						}
					}
				}
			}
		}
	}
}

static void wswan_draw_foreground_3( running_machine &machine ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = state->m_vdp.layer_fg_address + ( ( ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( state->m_vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		UINT32	plane0 = 0, plane1 = 0, plane2 = 0, plane3 = 0;
		int	x, x_offset, tile_line, tile_address;
		int	tile_data = ( state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
			            | state->m_vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		tile_line = ( state->m_vdp.current_line + state->m_vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			tile_line = 7 - tile_line;
		}

		if ( state->m_vdp.colors_16 ) {
			tile_address = ( ( tile_data & 0x2000 ) ? 0x8000 : 0x4000 ) + ( tile_number * 32 ) + ( tile_line << 2 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 24 ) | ( state->m_vdp.vram[ tile_address + 1 ] << 16 ) | ( state->m_vdp.vram[ tile_address + 2 ] << 8 ) | state->m_vdp.vram[ tile_address + 3 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = state->m_vdp.vram[ tile_address + 2 ] << 2;
				plane3 = state->m_vdp.vram[ tile_address + 3 ] << 3;
			}
		} else {
			tile_address = 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 );
			if ( state->m_vdp.tile_packed ) {
				plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 8 ) | state->m_vdp.vram[ tile_address + 1 ];
			} else {
				plane0 = state->m_vdp.vram[ tile_address + 0 ];
				plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
				plane2 = 0;
				plane3 = 0;
			}
		}

		for( x = 0; x < 8; x++ ) {
			int	col;
			if ( state->m_vdp.tile_packed ) {
				if ( state->m_vdp.colors_16 ) {
					col = plane0 & 0x0F;
					plane0 = plane0 >> 4;
				} else {
					col = plane0 & 0x03;
					plane0 = plane0 >> 2;
				}
			} else {
				col = ( plane3 & 8 ) | ( plane2 & 4 ) | ( plane1 & 2 ) | ( plane0 & 1 );
				plane3 = plane3 >> 1;
				plane2 = plane2 >> 1;
				plane1 = plane1 >> 1;
				plane0 = plane0 >> 1;
			}
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( state->m_vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( ( x_offset >= 0 && x_offset < state->m_vdp.window_fg_left ) || ( x_offset >= state->m_vdp.window_fg_right && x_offset < WSWAN_X_PIXELS ) ) {
				if ( state->m_vdp.colors_16 ) {
					if ( col ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							/* Hmmmm, what should we do here... Is this correct?? */
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						}
					}
				} else {
					if ( col || !(tile_palette & 4 ) ) {
						if ( state->m_vdp.color_mode ) {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
						} else {
							wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_vdp.main_palette[state->m_pal[tile_palette][col]] );
						}
					}
				}
			}
		}
	}
}

static void wswan_handle_sprites( running_machine &machine, int mask ) {
	wswan_state *state = machine.driver_data<wswan_state>();
	int	i;
	if ( state->m_vdp.sprite_count == 0 )
		return;
	for( i = state->m_vdp.sprite_first + state->m_vdp.sprite_count - 1; i >= state->m_vdp.sprite_first; i-- ) {
		UINT8	x, y;
		UINT16	tile_data;
		int	tile_line;

		tile_data = ( state->m_vdp.vram[ state->m_vdp.sprite_table_address + i * 4 + 1 ] << 8 )
		            | state->m_vdp.vram[ state->m_vdp.sprite_table_address + i * 4 ];
		y = state->m_vdp.vram[ state->m_vdp.sprite_table_address + i * 4 + 2 ];
		x = state->m_vdp.vram[ state->m_vdp.sprite_table_address + i * 4 + 3 ];
		tile_line = state->m_vdp.current_line - y;
		tile_line = tile_line & 0xFF;
		if ( ( tile_line >= 0 ) && ( tile_line < 8 ) && ( ( tile_data & mask ) == mask ) ) {
			UINT32	plane0 = 0, plane1 = 0, plane2 = 0, plane3 = 0;
			int	j, x_offset, tile_address;
			int	tile_number = tile_data & 0x01FF;
			int	tile_palette = 8 + ( ( tile_data >> 9 ) & 0x07 );
			int	check_clip = 0;
			if ( tile_data & 0x8000 ) {
				tile_line = 7 - tile_line;
			}

			if ( state->m_vdp.colors_16 ) {
				tile_address = 0x4000 + ( tile_number * 32 ) + ( tile_line << 2 );
				if ( state->m_vdp.tile_packed ) {
					plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 24 ) | ( state->m_vdp.vram[ tile_address + 1 ] << 16 ) | ( state->m_vdp.vram[ tile_address + 2 ] << 8 ) | state->m_vdp.vram[ tile_address + 3 ];
				} else {
					plane0 = state->m_vdp.vram[ tile_address + 0 ];
					plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
					plane2 = state->m_vdp.vram[ tile_address + 2 ] << 2;
					plane3 = state->m_vdp.vram[ tile_address + 3 ] << 3;
				}
			} else {
				tile_address = 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 );
				if ( state->m_vdp.tile_packed ) {
					plane0 = ( state->m_vdp.vram[ tile_address + 0 ] << 8 ) | state->m_vdp.vram[ tile_address + 1 ];
				} else {
					plane0 = state->m_vdp.vram[ tile_address + 0 ];
					plane1 = state->m_vdp.vram[ tile_address + 1 ] << 1;
					plane2 = 0;
					plane3 = 0;
				}
			}

			if ( state->m_vdp.window_sprites_enable ) {
				if ( tile_data & 0x1000 ) {
					if ( state->m_vdp.current_line >= state->m_vdp.window_sprites_top && state->m_vdp.current_line <= state->m_vdp.window_sprites_bottom ) {
						check_clip = 1;
					}
				} else {
					if ( state->m_vdp.current_line < state->m_vdp.window_sprites_top || state->m_vdp.current_line > state->m_vdp.window_sprites_bottom ) {
						continue;
					}
				}
			}

			for ( j = 0; j < 8; j++ ) {
				int col;
				if ( state->m_vdp.tile_packed ) {
					if ( state->m_vdp.colors_16 ) {
						col = plane0 & 0x0F;
						plane0 = plane0 >> 4;
					} else {
						col = plane0 & 0x03;
						plane0 = plane0 >> 2;
					}
				} else {
					col = ( plane3 & 8 ) | ( plane2 & 4 ) | ( plane1 & 2 ) | ( plane0 & 1 );
					plane3 = plane3 >> 1;
					plane2 = plane2 >> 1;
					plane1 = plane1 >> 1;
					plane0 = plane0 >> 1;
				}
				if ( tile_data & 0x4000 ) {
					x_offset = x + j;
				} else {
					x_offset = x + 7 - j;
				}
				x_offset = x_offset & 0xFF;
				if ( state->m_vdp.window_sprites_enable ) {
					if ( tile_data & 0x1000 && check_clip ) {
						if ( x_offset >= state->m_vdp.window_sprites_left && x_offset <= state->m_vdp.window_sprites_right ) {
							continue;
						}
					} else {
						if ( x_offset < state->m_vdp.window_sprites_left || x_offset > state->m_vdp.window_sprites_right ) {
//                          continue;
						}
					}
				}
				if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
					if ( state->m_vdp.colors_16 ) {
						if ( col ) {
							if ( state->m_vdp.color_mode ) {
								wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
							} else {
								/* Hmmmm, what should we do here... Is this correct?? */
								wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
							}
						}
					} else {
						if ( col || !(tile_palette & 4 ) ) {
							if ( state->m_vdp.color_mode ) {
								wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_pal[tile_palette][col] );
							} else {
								wswan_plot_pixel( machine, x_offset, state->m_vdp.current_line, state->m_vdp.main_palette[state->m_pal[tile_palette][col]] );
							}
						}
					}
				}
			}
		}
	}
}

void wswan_refresh_scanline( running_machine &machine )
{
	wswan_state *state = machine.driver_data<wswan_state>();
	rectangle rec;

	wswan_setup_palettes(state);

	rec.min_x = 0;
	rec.max_x = WSWAN_X_PIXELS;
	rec.min_y = rec.max_y = state->m_vdp.current_line;
	if ( state->m_ws_portram[0x14] ) {
		/* Not sure if these background color checks and settings are correct */
		if ( state->m_vdp.color_mode && state->m_vdp.colors_16 ) {
			wswan_fillbitmap( machine, state->m_pal[state->m_ws_portram[0x01]>>4][state->m_ws_portram[0x01]&0x0F], &rec );
		} else {
			wswan_fillbitmap( machine, state->m_vdp.main_palette[state->m_ws_portram[0x01]&0x07], &rec );
		}
	} else {
		wswan_fillbitmap( machine, 0, &rec );
		return;
	}

	/*
     * Draw background layer
     */
	if ( state->m_vdp.layer_bg_enable ) {
		wswan_draw_background(machine);
	}

	/*
     * Draw sprites between background and foreground layers
     */
	if ( state->m_vdp.sprites_enable ) {
		wswan_handle_sprites( machine, 0 );
	}

	/*
     * Draw foreground layer, taking window settings into account
     */
	if ( state->m_vdp.layer_fg_enable ) {
		switch( state->m_vdp.window_fg_mode ) {
		case 0:	/* FG inside & outside window area */
			wswan_draw_foreground_0(machine);
			break;
		case 1:	/* ??? */
			logerror( "Unknown foreground mode 1 set\n" );
			break;
		case 2:	/* FG only inside window area */
			if ( state->m_vdp.current_line >= state->m_vdp.window_fg_top && state->m_vdp.current_line <= state->m_vdp.window_fg_bottom ) {
				wswan_draw_foreground_2(machine);
			}
			break;
		case 3:	/* FG only outside window area */
			if ( state->m_vdp.current_line < state->m_vdp.window_fg_top || state->m_vdp.current_line > state->m_vdp.window_fg_bottom ) {
				wswan_draw_foreground_0(machine);
			} else {
				wswan_draw_foreground_3(machine);
			}
			break;
		}
	}

	/*
     * Draw sprites in front of foreground layer
     */
	if ( state->m_vdp.sprites_enable ) {
		wswan_handle_sprites( machine, 0x2000 );
	}
}

