#ifndef AMIGACRT_H
#define AMIGACRT_H

void amiga_cart_init( running_machine *machine );
void amiga_cart_check_overlay( running_machine *machine );
void amiga_cart_nmi( void );

#endif /* AMIGACRT_H */
