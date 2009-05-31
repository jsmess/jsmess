/***************************************************************************

    infomess.h

    MESS-specific augmentation to info.c

***************************************************************************/

#ifndef INFOMESS_H
#define INFOMESS_H

/* code used by print_mame_xml() */
void print_game_categories(FILE *out, const game_driver *game, const input_port_config *portlist);
void print_game_device(FILE *out, const game_driver *game, const machine_config *config);
void print_game_ramoptions(FILE *out, const game_driver *game, const machine_config *config);

#endif /* INFOMESS_H */
