extern unsigned char *s2636_1_ram;
extern unsigned char *s2636_2_ram;
extern unsigned char *s2636_3_ram;

extern mame_bitmap *s2636_1_bitmap;
extern mame_bitmap *s2636_2_bitmap;
extern mame_bitmap *s2636_3_bitmap;

extern unsigned char s2636_1_dirty[4];
extern unsigned char s2636_2_dirty[4];
extern unsigned char s2636_3_dirty[4];

extern int s2636_x_offset;
extern int s2636_y_offset;

void s2636_w(unsigned char *workram,int offset,int data,unsigned char *dirty);
void Update_Bitmap(mame_bitmap *bitmap,unsigned char *workram,unsigned char *dirty,int Graphics_Bank,mame_bitmap *collision_bitmap);

