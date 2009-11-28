extern void border_force_redraw (void);
extern void border_set_last_color (int NewColor);
extern void border_draw(running_machine *machine, bitmap_t *bitmap, int full_refresh,
                int TopBorderLines, int ScreenLines, int BottomBorderLines,
                int LeftBorderPixels, int ScreenPixels, int RightBorderPixels,
                int LeftBorderCycles, int ScreenCycles, int RightBorderCycles,
                int HorizontalRetraceCycles, int VRetraceTime, int EventID);
