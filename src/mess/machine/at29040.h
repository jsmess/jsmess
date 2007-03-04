void at29c040a_init_data_ptr(int id, UINT8 *data_ptr);
void at29c040a_init(int id);
int at29c040a_file_load(int id, mame_file *file);
int at29c040a_file_save(int id, mame_file *file);
UINT8 at29c040a_r(int id, offs_t offset);
void at29c040a_w(int id, offs_t offset, UINT8 data);
int at29c040a_get_dirty_flag(int id);
