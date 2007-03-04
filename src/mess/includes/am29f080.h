void amd_flash_init(int);
void amd_flash_reset(int);
void amd_flash_finish(int);
void amd_flash_store(int, const char *);
void amd_flash_restore(int, const char *);
char *amd_flash_get_base(int);

int     amd_flash_bank_handler_r(int index1, int offset);
void    amd_flash_bank_handler_w(int index1, int offset, int data);

