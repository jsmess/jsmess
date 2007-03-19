extern void force_border_redraw (void);
extern void set_last_border_color (int NewColor);
extern void draw_border(mame_bitmap *bitmap, int full_refresh,
                int TopBorderLines, int ScreenLines, int BottomBorderLines,
                int LeftBorderPixels, int ScreenPixels, int RightBorderPixels,
                int LeftBorderCycles, int ScreenCycles, int RightBorderCycles,
                int HorizontalRetraceCycles, int VRetraceTime, int EventID);
