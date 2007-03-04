void flash_init(int);
void flash_reset(int);
void flash_finish(int);
void flash_store(int, const char *);
void flash_restore(int, const char *);
char *flash_get_base(int);

int     flash_bank_handler_r(int index1, int offset);
void    flash_bank_handler_w(int index1, int offset, int data);

