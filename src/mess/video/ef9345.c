/*********************************************************************

    ef9345.c

    Thomson EF9345 video controller emulator code

    This code is based on Daniel Coulom's implementation in DCVG5k
    and DCAlice released by Daniel Coulom under GPL license

    The implementation below is released under the MAME license for use
    in MAME, MESS and derivatives by permission of the author.

*********************************************************************/

#include "emu.h"
#include "ef9345.h"

typedef struct _ef9345_t ef9345_t;
struct _ef9345_t
{
	const ef9345_config *conf;

	UINT8 bf;						//busy flag
	UINT8 char_mode;				//40 or 80 chars for line
	UINT8* charset;					//charset
	UINT8 ram[0x4000];				//16Ko (logique) pour 8Ko (physique)
	UINT8 acc_char[0x2000];			//8Ko (logique) polices accentuees
	UINT8 registers[8];				//registers R0-R7
	UINT8 state;					//status register
	UINT8 TGS,MAT,PAT,DOR,ROR;		//registres acces indirect
	UINT8 border[80];				//caractere du cadre
	UINT8 *block;					//pointeur block courant
	UINT8 *matrice;					//pointeur matrice caractere courant
	UINT8 *char_ptr[16];			//pointeurs polices de caracteres
	UINT16 blink;					//etat du clignotement
	UINT8 dial;						//cadran en cours d'affichage
	UINT16 last_dial[40];			//dernier cadran affiche
	UINT8 latchc0;					//couleur c0 latchee
	UINT16 latchm;					//attribut m(conceal) latche
	UINT16 latchi;					//attribut i(insert) latche
	UINT16 latchu;					//attribut u(underline) latche

	bitmap_t *screen_out;

	emu_timer *busy_timer;
	emu_timer *blink_timer;

};

static TIMER_CALLBACK( ef9345_done );


INLINE _ef9345_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == EF9345);
	return (_ef9345_t *)downcast<legacy_device_base *>(device)->token();
}

/*-------------------------------------------------
    set busy flag and timer to clear it
-------------------------------------------------*/
static void set_busy_flag(ef9345_t *ef9345, int period)
{
	ef9345->bf = 1;

	timer_adjust_oneshot(ef9345->busy_timer, ATTOTIME_IN_USEC(period), 0);
}

/*-------------------------------------------------
    draw a char in 40 char line mode
-------------------------------------------------*/
void draw_char_40(ef9345_t *ef9345, UINT8 *c, int x, int y)
{
	UINT16 i, j;

	/* verify size limit */
	if (y * 10 >= ef9345->conf->height || x * 8 >= ef9345->conf->width)
		return;

	for(i = 0; i < 10; i++)
		for(j = 0; j < 8; j++)
				*BITMAP_ADDR16(ef9345->screen_out, y * 10 + i, x * 8 + j)  = c[8 * i + j] & 0x07;
}

/*-------------------------------------------------
    draw a char in 80 char line mode
-------------------------------------------------*/
void draw_char_80(ef9345_t *ef9345, UINT8 *c, int x, int y)
{
	int i, j;

	/* verify size limit */
	if (y * 10 >= ef9345->conf->height || x * 6 >= ef9345->conf->width)
		return;

	for(i = 0; i < 10; i++)
		for(j = 0; j < 6; j++)
				*BITMAP_ADDR16(ef9345->screen_out, y * 10 + i, x * 6 + j)  = c[6 * i + j] & 0x07;
}


/*-------------------------------------------------
    set then ef9345 mode
-------------------------------------------------*/
void set_ef9345_mode(ef9345_t *ef9345)
{
	UINT8 i;
	ef9345->char_mode = ((ef9345->PAT & 0x80) >> 5) | ((ef9345->TGS & 0xc0) >> 6);

	//couleur de bordure
	for(i = 0; i < 80; i++)
		ef9345->border[i] = ef9345->MAT & 0x07;

	//polices (avec ram 8K)
	ef9345->char_ptr[0x08] = ef9345->ram + ((ef9345->DOR & 0x07) << 11); //G'0
	ef9345->char_ptr[0x09] = ef9345->char_ptr[0x08];                     //G'0 underlined
	ef9345->char_ptr[0x0a] = ef9345->ram + ((ef9345->DOR & 0x30) << 8);  //G'10
	ef9345->char_ptr[0x0b] = ef9345->char_ptr[0x0a] + 0x800;             //G'11
	ef9345->char_ptr[0x0c] = ef9345->ram;                        //Quadrichrome
	ef9345->char_ptr[0x0d] = ef9345->char_ptr[0x0c];
	ef9345->char_ptr[0x0e] = ef9345->char_ptr[0x0c];
	ef9345->char_ptr[0x0f] = ef9345->char_ptr[0x0c];

	//adresse block memoire courant
	i = ((ef9345->ROR & 0xf0) >> 4) | ((ef9345->ROR & 0x40) >> 5) | ((ef9345->ROR & 0x20) >> 3);
	ef9345->block = ef9345->ram + 0x800 * (i & 0x0c);
}

/*-------------------------------------------------
    Initialize the ef9345 char_ptr
-------------------------------------------------*/
void init_char_prt(ef9345_t *ef9345)
{
	int i, j;
	//polices accentuees
	for(j = 0; j < 0x10; j++)
		for(i = 0; i < 0x200; i++)
			ef9345->acc_char[(j << 9) + i] = ef9345->charset[0x0600 + i];

	for(j = 0; j < 0x200; j += 0x40)
		for(i = 0; i < 4; i++)
		{
			ef9345->acc_char[0x0200 + j + i +  4] |= 0x1c; //tilde
			ef9345->acc_char[0x0400 + j + i +  4] |= 0x10; //aigu
			ef9345->acc_char[0x0400 + j + i +  8] |= 0x08; //aigu
			ef9345->acc_char[0x0600 + j + i +  4] |= 0x04; //grave
			ef9345->acc_char[0x0600 + j + i +  8] |= 0x08; //grave
			
			ef9345->acc_char[0x0a00 + j + i +  4] |= 0x1c; //tilde
			ef9345->acc_char[0x0c00 + j + i +  4] |= 0x10; //aigu
			ef9345->acc_char[0x0c00 + j + i +  8] |= 0x08; //aigu
			ef9345->acc_char[0x0e00 + j + i +  4] |= 0x04; //grave
			ef9345->acc_char[0x0e00 + j + i +  8] |= 0x08; //grave
			
			ef9345->acc_char[0x1200 + j + i +  4] |= 0x08; //point
			ef9345->acc_char[0x1400 + j + i +  4] |= 0x14; //trema
			ef9345->acc_char[0x1600 + j + i + 32] |= 0x08; //cedille
			ef9345->acc_char[0x1600 + j + i + 36] |= 0x04; //cedille
			
			ef9345->acc_char[0x1a00 + j + i +  4] |= 0x08; //point
			ef9345->acc_char[0x1c00 + j + i +  4] |= 0x14; //trema
			ef9345->acc_char[0x1e00 + j + i + 32] |= 0x08; //cedille
			ef9345->acc_char[0x1e00 + j + i + 36] |= 0x04; //cedille
		}
	
	//initialisation des pointeurs de polices fixes
	ef9345->char_ptr[0x00] = ef9345->charset;          //G0
	ef9345->char_ptr[0x01] = ef9345->charset + 0x0800; //G0 underlined
	ef9345->char_ptr[0x02] = ef9345->charset + 0x1000; //G11
	ef9345->char_ptr[0x03] = ef9345->charset + 0x1800; //G10
	ef9345->char_ptr[0x04] = ef9345->acc_char;          //G20
	ef9345->char_ptr[0x05] = ef9345->acc_char + 0x0800; //G20 underlined
	ef9345->char_ptr[0x06] = ef9345->acc_char + 0x1000; //G21
	ef9345->char_ptr[0x07] = ef9345->acc_char + 0x1800; //G21 underlined
}


/* calculate the internal RAM offset */
UINT16 indexram(ef9345_t *ef9345, UINT8 r)
{
	UINT8 x, y;
	x = ef9345->registers[r];
	y = ef9345->registers[r - 1];
	if(y < 8)
		y &= 1;
	return ((x&0x3f) | ((x & 0x40) << 6) | ((x & 0x80) << 4) | ((y & 0x1f) << 6) | ((y & 0x20) << 8));
}

/* calculate the internal ROM offset */
UINT16 indexrom(ef9345_t *ef9345, int r)
{
	UINT8 x, y;
	x = ef9345->registers[r];
	y = ef9345->registers[r - 1];
	if(y < 8)
		y &= 1;
	return((x&0x3f)|((x&0x40)<<6)|((x&0x80)<<4)|((y&0x1f)<<6));
}

//calcul de l'adresse du caractere x, y dans le block courant
int indexblock(ef9345_t *ef9345, int x, int y)
{
	int i, j;
	i = x;
	j = (y == 0) ? ((ef9345->TGS & 0x20) >> 5) : ((ef9345->ROR & 0x1f) + y - 1);
	//partie droite d'un caractere en double largeur en mode 40 caracteres
	if((ef9345->TGS & 0x80) == 0) if(x > 0)
	{
		if(ef9345->last_dial[x - 1] == 1) i--; //colonne precedente
		if(ef9345->last_dial[x - 1] == 4) i--; //colonne precedente
		if(ef9345->last_dial[x - 1] == 5) i--; //colonne precedente
	}

	return 0x40 * j + i;
}

// Increment X
void inc_x(ef9345_t *ef9345, int r)
{
	int i;
	i = (ef9345->registers[r] & 0x3f) + 1;
	if(i > 39)
	{
		i -= 40;
		ef9345->state |= 0x40;
	}
	ef9345->registers[r] = (ef9345->registers[r] & 0xc0) | i;
}

// Increment Y
void inc_y(ef9345_t *ef9345, int r)
{
	int i;
	i = (ef9345->registers[r] & 0x1f) + 1;
	if(i > 31)
		i -= 24;
	ef9345->registers[r] = (ef9345->registers[r] & 0xe0) | i;
}

// Determination du cadran a afficher
void Cadran(ef9345_t *ef9345, int x, int attrib)
{
	while(1)
	{
		if(x > 0)
			if(ef9345->last_dial[x-1] == 1)
			{ef9345->dial = 2; break;}			//cadran haut droit
		if(x > 0)
			if(ef9345->last_dial[x-1] == 5)
				{ef9345->dial = 10; break;}		//moitie droite
		if(ef9345->last_dial[x] == 1)
			{ef9345->dial = 4; break;}			//cadran bas gauche
		if(ef9345->last_dial[x] == 2)
			{ef9345->dial = 8; break;}			//dial bas droit
		if(ef9345->last_dial[x] == 3)
			{ef9345->dial = 12; break;}			//moitie basse
		if(attrib == 1)
			{ef9345->dial = 5; break;}			//moitie gauche
		if(attrib == 2)
			{ef9345->dial = 3; break;}			//moitie haute
		if(attrib == 3)
			{ef9345->dial = 1; break;}			//dial haut gauche
		ef9345->dial = 0; break;				//caractere complet
	}
	ef9345->last_dial[x] = ef9345->dial;
}

// zoom sur une partie d'un caractere
void zoom(ef9345_t *ef9345, UINT8 *pix, int n)
{
	//pix est le tableau des pixels (8x10=80)
	//n definit la zone (zoomx2y2 pour cadran, zoomx2 ou zoomy2 pour moitie)
	//cadran haut-gauche=1, haut-droit=2, bas-gauche=4, bas-droite=8
	//moitie haute=3, basse=12, gauche=5, droite=10
	int i, j;
	if((n & 0x0a) == 0)
		for(i = 0; i < 80; i += 8) // 1, 4, 5
			for(j = 7; j > 0; j--)
				pix[i + j] = pix[i + j / 2];

	if((n & 0x05) == 0)
		for(i = 0; i < 80; i += 8) // 2, 8, 10
			for(j =0 ; j < 7; j++)
				pix[i + j] = pix[i + 4 + j / 2];

	if((n & 0x0c) == 0)
		for(i = 0; i < 8; i++) // 1, 2, 3
			for(j = 9; j > 0; j--)
				pix[i + 8 * j] = pix[i + 8 * (j / 2)];
	if((n & 0x03) == 0)
		for(i = 0; i < 8; i++) // 4, 8, 12
			for(j = 0; j < 9; j++)
				pix[i + 8 * j] = pix[i + 40 + 8 * (j / 2)];
}

// Display bichrome character (40 columns)
void Bichrome40(ef9345_t *ef9345, int iblock, int x, int y, int c0, int c1, int insert, int flash, int conceal, int negative, int underline)
{
	UINT8 i, j, pix[80];

	if(flash && ef9345->PAT & 0x40 && ef9345->blink)
		c1 = c0;										//flash
	if(conceal && ef9345->PAT & 0x08)
		c1 = c0;        								//conceal
	if(negative)										//negative
	{
		i = c1;
		c1 = c0;
		c0 = i;
	}
	//? v?rifier cf instructions en commentaires ci-dessous : insert = 0 ou 1 ??
	if((ef9345->PAT & 0x30) == 0x30)
		insert = 0;         							//active area mark
	if(insert == 0)
		c1 += 8;                    					//couleur etendue foreground
	if((ef9345->PAT & 0x30) == 0x00)
		insert = 1;         							//inlay
	if(insert == 0)
		c0 += 8;                    					//couleur etendue background


	//affichage du curseur
	i = (ef9345->registers[6] & 0x1f);
	if(i < 8)
		i &= 1;

	if(iblock == 0x40 * i + (ef9345->registers[7] & 0x3f))	//position du curseur
	{
		//affichage du curseur si ef9345->MAT 6 est positionne
		//le clignotement depend de ef9345->MAT 5, la forme depend de ef9345->MAT 4
		switch(ef9345->MAT & 0x70)
		{
		case 0x40:  									//00 = fixed complemented
			c0 = (23 - c0) & 15;
			c1 = (23 - c1) & 15;
			break;
		case 0x50:  									//01 = fixed underlined
			underline = 1;
			break;
		case 0x60:  									//10 = flash complemented
			if(ef9345->blink)
			{
				c0 = (23 - c0) & 15;
				c1 = (23 - c1) & 15;
			}
			break;
		case 0x70:  									//11 = flash underlined
			if(ef9345->blink)
				underline = 1;
				break;
		}
	}

	//initialisation du tableau des pixels du caractere
	j = 0;

	for(i = 0; i < 10; i++)
	{
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x01) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x02) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x04) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x08) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x10) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x20) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x40) ? c1 : c0;
		pix[j++] = (*(ef9345->matrice + 4 * i) & 0x80) ? c1 : c0;

	}

	//caractere souligne
	if(underline)
		for(j = 72; j < 80; j++)
			pix[j] = c1;

	//affichage en fonction du cadran
	//remarque : la double hauteur par le bit 7 du registre ef9345->MAT
	//est prise en UINT8ge par la fonction d'affichage de l'ecran
	//voir Displayscreen() dans le module de traitement de la video
	if(ef9345->dial > 0)
		zoom(ef9345, pix, ef9345->dial);

	draw_char_40(ef9345, pix, x + 1 , y + 1);
}

// Display quadrichrome character (40 columns)
void Quadrichrome40(ef9345_t *ef9345, int c, int b, int a, int x, int y)
{
	//C0-6= character code
	//B0= insert             not yet implemented !!!
	//B1= low resolution
	//B2= subset index (low resolution only)
	//B3-5 = set number
	//A0-6 = 4 color palette

	UINT8 i, j, n, col[8], pix[80];
	UINT8 lowresolution = (b & 0x02) >> 1, ramx, ramy, ramblock;
	UINT8 *ramindex;
	//pas de double hauteur ou largeur pour les quadrichromes
	ef9345->last_dial[x] = 0;
	//color table initialization (seules les couleurs 0-3 sont utilis?es)
	j = 1; n = 0;
	for(i = 0; i < 4; i++)
		col[i] = 7; //blanc par defaut

	for(i = 0; i < 8; i++)
	{
		if(a & j)
			col[n++] = i;
		j <<= 1;
	}

	//find block number i in ram
	ramblock = 0;
	if(b & 0x20)	ramblock |= 4;      //B5
	if(b & 0x08)	ramblock |= 2;      //B3
	if(b & 0x10)	ramblock |= 1;      //B4

	//find character address in ram
	ramx = c & 0x03; ramy =(c & 0x7f) >> 2;
	ramindex = ef9345->char_ptr[0x0c] + 0x800 * ramblock + 0x40 * ramy + ramx;
	if(lowresolution) ramindex += 5 * (b & 0x04);

	//fill pixel table
	j = 0; for(i = 0; i < 10; i++)
	{
		pix[j] = pix[j + 1] = col[(*(ramindex + 4 * (i >> lowresolution)) & 0x03) >> 0]; j += 2;
		pix[j] = pix[j + 1] = col[(*(ramindex + 4 * (i >> lowresolution)) & 0x0c) >> 2]; j += 2;
		pix[j] = pix[j + 1] = col[(*(ramindex + 4 * (i >> lowresolution)) & 0x30) >> 4]; j += 2;
		pix[j] = pix[j + 1] = col[(*(ramindex + 4 * (i >> lowresolution)) & 0xc0) >> 6]; j += 2;
	}

	draw_char_40(ef9345, pix, x + 1, y + 1);
}

// Display bichrome character (80 columns)
void Bichrome80(ef9345_t *ef9345, int c, int a, int x, int y)
{
	UINT8 pix[60]; //tableau des pixels a afficher
	int i, j, d, c0, c1;
	c1 = (a & 1) ? (ef9345->DOR >> 4) & 7 : ef9345->DOR & 7; //foreground color = ef9345->DOR
	c0 =  ef9345->MAT & 7;                           //background color = ef9345->MAT
	switch(c & 0x80)
	{
	case 0: //alphanumeric G0 set
		//A0: D = color set
		//A1: U = underline
		//A2: F = flash
		//A3: N = negative
		//C0-6: character code
		j = 0;
		if(a & 4) if(ef9345->PAT & 0x40) if(ef9345->blink) c1 = c0; //flash
		if(a & 8) {i = c1; c1 = c0; c0 = i;}        //couleurs en mode inverse
		d = ((c & 0x7f) >> 2) * 0x40 + (c & 0x03);  //deplacement dans table police
		for(i = 0; i < 10; i++)
		{
			pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x01) ? c1 : c0; pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x02) ? c1 : c0;
			pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x04) ? c1 : c0; pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x08) ? c1 : c0;
			pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x10) ? c1 : c0; pix[j++] = (*(ef9345->charset + d + 4 * i) & 0x20) ? c1 : c0;
		}
		if(a & 2) for(j = 54; j < 60; j++) pix[j] = c1; //underline
		break;
	default: //dedicated mosaic set
		//A0: D = color set
		//A1-3: 3 blocks de 6 pixels
		//C0-6: 7 blocks de 6 pixels
		pix[ 0] = (c & 0x01) ? c1 : c0;
		pix[ 3] = (c & 0x02) ? c1 : c0;
		pix[12] = (c & 0x04) ? c1 : c0;
		pix[15] = (c & 0x08) ? c1 : c0;
		pix[24] = (c & 0x10) ? c1 : c0;
		pix[27] = (c & 0x20) ? c1 : c0;
		pix[36] = (c & 0x40) ? c1 : c0;
		pix[39] = (a & 0x02) ? c1 : c0;
		pix[48] = (a & 0x04) ? c1 : c0;
		pix[51] = (a & 0x08) ? c1 : c0;
		for(i = 0; i < 60; i += 12)
		{
			pix[i + 6] = pix[i];
			pix[i + 9] = pix[i + 3];
		}
		for(i = 0; i < 60; i += 3)
			pix[i + 2] = pix[i + 1] = pix[i];
		break;
	}
	draw_char_80(ef9345, pix, x, y);
}


// Affichage d'un caractere 16 bits 40 colonnes
void makechar_16x40(ef9345_t *ef9345, int x, int y)
{
	int i, a, b, type, c0, c1, f, m, n, u, iblock;

	//recherche des octets b, a (precedent si double largeur)
	iblock = indexblock(ef9345, x, y);
	a = ef9345->block[iblock];
	b = ef9345->block[iblock + 0x800];

	//determination attribut HL et numero du cadran a afficher
	Cadran(ef9345, x, (a & 0x80) ? 0 : (((a & 0x20) >> 5) | ((a & 0x10) >> 3)));

	//adresse de la matrice de definition du caractere
	type = ((b & 0x80) >> 4) | ((a & 0x80) >> 6);
	ef9345->matrice = ef9345->char_ptr[type] + ((b & 0x7f) >> 2) * 0x40 + (b & 0x03);
	if((b & 0xe0) == 0x80)
		ef9345->matrice = ef9345->char_ptr[0x03]; //negative space

	//attributs latches
	if(x == 0) ef9345->latchm = ef9345->latchi = ef9345->latchu = ef9345->latchc0 = 0;
	if(type == 4) {ef9345->latchm = b & 1; ef9345->latchi = (b & 2) >> 1; ef9345->latchu = (b & 4) >> 2;}
	if(a & 0x80) ef9345->latchc0 = (a & 0x70) >> 4;

	//attributs du caractere
	c0 = ef9345->latchc0;					//background
	c1 = a & 0x07;							//foreground
	i = ef9345->latchi;						//insert mode
	f  = (a & 0x08) >> 3;					//flash
	m = ef9345->latchm;						//conceal
	n  = (a & 0x80) ? 0: ((a & 0x40) >> 6);	//negative
	u = ef9345->latchu;						//underline

	//affichage
	Bichrome40(ef9345, iblock, x, y, c0, c1, i, f, m, n, u);
}

// Affichage d'un caractere 24 bits 40 colonnes
void makechar_24x40(ef9345_t *ef9345, int x, int y)
{
	int i, a, b, c, c0, c1, f, m, n, u, iblock;

	//recherche des octets c, b, a
	//iblock = (ef9345->MAT & 0x80) ? indexblock(x, y / 2) : indexblock(x, y);
	//la double hauteur est maintenant prise en compte dans Displayscreen()
	iblock = indexblock(ef9345, x, y);
	c = ef9345->block[iblock]; b = ef9345->block[iblock + 0x800]; a = ef9345->block[iblock + 0x1000];
	if((b & 0xc0) == 0xc0) {Quadrichrome40(ef9345, c, b, a, x, y); return;}

	//determination attribut HL et numero du cadran a afficher
	Cadran(ef9345, x, (b & 0x02) + ((b & 0x08) >> 3));

	//adresse de la matrice de definition du caractere
	ef9345->matrice = ef9345->char_ptr[(b & 0xf0) >> 4] + ((c & 0x7f) >> 2) * 0x40 + (c & 0x03);

	//attributs du caractere
	c0 = a & 0x07;        //background
	c1 = (a & 0x70) >> 4; //foreground
	i = b & 0x01;         //insert
	f = (a & 0x08) >> 3;  //flash
	m = (b & 0x04) >> 2;  //conceal
	n = (a & 0x80) >> 7;  //negative
	u = (((b & 0x60) == 0) || ((b & 0xc0) == 0x40)) ? ((b & 0x10) >> 4) : 0;

	//affichage
	Bichrome40(ef9345, iblock, x, y, c0, c1, i, f, m, n, u);
}


// Affichage de deux caracteres 12 bits 80 colonnes
void makechar_12x80(ef9345_t *ef9345, int x, int y)
{
	int iblock;
	iblock = indexblock(ef9345, x, y);
	Bichrome80(ef9345, ef9345->block[iblock], (ef9345->block[iblock + 0x1000] >> 4) & 0x0f, 2 * x + 2, y + 1);
	Bichrome80(ef9345, ef9345->block[iblock + 0x800], ef9345->block[iblock + 0x1000] & 0x0f, 2 * x + 3, y + 1);
}

void makechar(ef9345_t *ef9345, int x, int y)
{
	switch (ef9345->char_mode)
	{
		case 0:
			makechar_24x40(ef9345, x, y);
			break;
		case 3:
			makechar_12x80(ef9345, x, y);
			break;
		case 4:
			makechar_16x40(ef9345, x, y);
			break;
		default:
			break;
	}
}

// Lecture-ecriture en memoire EF9345
void OCTwrite(ef9345_t *ef9345, int r, int i)
{
	set_busy_flag(ef9345, 4);
	ef9345->ram[indexram(ef9345, r)] = ef9345->registers[1];
	if(i == 0) return;
	inc_x(ef9345, r);
	if((ef9345->registers[r] & 0x3f) == 0 && r == 7)
			inc_y(ef9345, 6);
}
void OCTread(ef9345_t *ef9345, int r, int i)
{
	set_busy_flag(ef9345, 4.5);
	ef9345->registers[1] = ef9345->ram[indexram(ef9345, r)];
	if(i == 0) return;
	inc_x(ef9345, r);

	if((ef9345->registers[r] & 0x3f) == 0 && r == 7)
		inc_y(ef9345, 6); //increment
}

void KRGwrite(ef9345_t *ef9345, int i) //mode 40 caracteres 16 bits
{
	int a = indexram(ef9345, 7);
	set_busy_flag(ef9345, 5.5);
	ef9345->ram[a] = ef9345->registers[1];
	ef9345->ram[a + 0x0800] = ef9345->registers[2];
	if(i) inc_x(ef9345, 7);
}
void KRGread(ef9345_t *ef9345, int i)
{
	int a = indexram(ef9345, 7);

	set_busy_flag(ef9345, 7.5);
	ef9345->registers[1] = ef9345->ram[a];
	ef9345->registers[2] = ef9345->ram[a + 0x0800];
	if(i) inc_x(ef9345, 7);
}

void KRFwrite(ef9345_t *ef9345, int i) //mode 40 caracteres 24 bits
{
	int a = indexram(ef9345, 7);

	set_busy_flag(ef9345, 4);
	ef9345->ram[a] = ef9345->registers[1];
	ef9345->ram[a + 0x0800] = ef9345->registers[2];
	ef9345->ram[a + 0x1000] = ef9345->registers[3];
	if(i) inc_x(ef9345, 7);
}

void KRFread(ef9345_t *ef9345, int i)
{
	int a = indexram(ef9345, 7);

	set_busy_flag(ef9345, 7.5);
	ef9345->registers[1] = ef9345->ram[a];
	ef9345->registers[2] = ef9345->ram[a + 0x0800];
	ef9345->registers[3] = ef9345->ram[a + 0x1000];
	if(i) inc_x(ef9345, 7);
}

void KRLwrite(ef9345_t *ef9345, int i) //mode 80 caracteres 12 bits
{
	int a = indexram(ef9345, 7);

	set_busy_flag(ef9345, 12.5);
	ef9345->ram[a] = ef9345->registers[1];
	switch((a / 0x800) & 1)
	{
	case 0: ef9345->ram[a + 0x1000] &= 0x0f;
		ef9345->ram[a + 0x1000] |= (ef9345->registers[3] & 0xf0); break;
	case 1: ef9345->ram[a + 0x0800] &= 0xf0;
		ef9345->ram[a + 0x0800] |= (ef9345->registers[3] & 0x0f); break;
	}
	if(i)
	{
		if((ef9345->registers[7] & 0x80) == 0x00) {ef9345->registers[7] |= 0x80; return;}
		ef9345->registers[7] &= 0x80;
		inc_x(ef9345, 7);
	}
}

void KRLread(ef9345_t *ef9345, int i)
{
	int a = indexram(ef9345, 7);

	set_busy_flag(ef9345, 11.5);
	ef9345->registers[1] = ef9345->ram[a];
	switch((a / 0x800) & 1)
	{
	case 0: ef9345->registers[3] = ef9345->ram[a + 0x1000]; break;
	case 1: ef9345->registers[3] = ef9345->ram[a + 0x0800]; break;
	}
	if(i)
	{
		if((ef9345->registers[7] & 0x80) == 0x00) {ef9345->registers[7] |= 0x80; return;}
		ef9345->registers[7] &= 0x80;
		inc_x(ef9345, 7);
	}
}

// Move buffer
void MVB(ef9345_t *ef9345, int n, int r1, int r2, int s)
{
	//s=1 pour mode normal, s=0 pour remplir tout le buffer
	int i, a1, a2;

	set_busy_flag(ef9345, 2);
	for(i = 0; i < 1280; i++)
	{
		a1 = indexram(ef9345, r1); a2 = indexram(ef9345, r2);
		ef9345->ram[a2] = ef9345->ram[a1]; //ef9345_cycle += 16 * n;
		if(n > 1) ef9345->ram[a2 + 0x0800] = ef9345->ram[a1 + 0x0800];
		if(n > 2) ef9345->ram[a2 + 0x1000] = ef9345->ram[a1 + 0x1000];
		inc_x(ef9345, r1); inc_x(ef9345, r2);
		if((ef9345->registers[5] & 0x3f) == 0) {if(s) break;}
		if((ef9345->registers[7] & 0x3f) == 0) {if(s) break; else inc_y(ef9345, 6);}
	}
	ef9345->state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
}

// Increment registre R6
void INY(ef9345_t *ef9345)
{
	set_busy_flag(ef9345, 2);
	inc_y(ef9345, 6);
	ef9345->state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
}

// Lecture/Ecriture des registres indirects
void INDwrite(ef9345_t *ef9345, int x)
{
	set_busy_flag(ef9345, 2);
	switch(x)
	{
	case 1: ef9345->TGS = ef9345->registers[1]; break;
	case 2: ef9345->MAT = ef9345->registers[1]; break;
	case 3: ef9345->PAT = ef9345->registers[1]; break;
	case 4: ef9345->DOR = ef9345->registers[1]; break;
	case 7: ef9345->ROR = ef9345->registers[1]; break;
	}
	set_ef9345_mode(ef9345);
	ef9345->state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
}

void INDread(ef9345_t *ef9345, int x)
{
	set_busy_flag(ef9345, 3.5);
	switch(x)
	{
	case 0: ef9345->registers[1] = ef9345->charset[indexrom(ef9345, 7) & 0x1fff]; break;
	case 1: ef9345->registers[1] = ef9345->TGS; break;
	case 2: ef9345->registers[1] = ef9345->MAT; break;
	case 3: ef9345->registers[1] = ef9345->PAT; break;
	case 4: ef9345->registers[1] = ef9345->DOR; break;
	case 7: ef9345->registers[1] = ef9345->ROR; break;
	case 8: ef9345->registers[1] = ef9345->ROR; break;
	}
	ef9345->state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
}

// Execute EF9345 command
void ef9345_exec(ef9345_t *ef9345, UINT8 cmd)
{
	ef9345->state = 0;
	if((ef9345->registers[5] & 0x3f) == 39) ef9345->state |= 0x10; //S4(LXa) set
	if((ef9345->registers[7] & 0x3f) == 39) ef9345->state |= 0x20; //S5(LXm) set

	switch(cmd)
	{
		case 0x00:		KRFwrite(ef9345, 0);	break;		//KRF: R1,R2,R3->ram
		case 0x01:		KRFwrite(ef9345, 1);	break;		//KRF: R1,R2,R3->ram + increment
		case 0x02:		KRGwrite(ef9345, 0);	break;		//KRG: R1,R2->ram
		case 0x03:		KRGwrite(ef9345, 1);	break;		//KRG: R1,R2->ram + increment
		case 0x05:								break;		//CLF: Clear page 24 bits
		case 0x07:								break;		//CLG: Clear page 16 bits
		case 0x08:		KRFread(ef9345, 0);		break;		//KRF: ram->R1,R2,R3
		case 0x09:		KRFread(ef9345, 1);		break;		//KRF: ram->R1,R2,R3 + increment
		case 0x0a:		KRGread(ef9345, 0);		break;		//KRG: ram->R1,R2
		case 0x0b:		KRGread(ef9345, 1);		break;		//KRG: ram->R1,R2 + increment
		case 0x30:		OCTwrite(ef9345, 7, 0);	break;		//OCT: R1->RAM, main pointer
		case 0x31:		OCTwrite(ef9345, 7, 1);	break;		//OCT: R1->RAM, main pointer + inc
		case 0x34:		OCTwrite(ef9345, 5, 0);	break;		//OCT: R1->RAM, aux pointer
		case 0x35:		OCTwrite(ef9345, 5, 1);	break;		//OCT: R1->RAM, aux pointer + inc
		case 0x38:		OCTread(ef9345, 7, 0);	break;		//OCT: RAM->R1, main pointer
		case 0x39:		OCTread(ef9345, 7, 1);	break;		//OCT: RAM->R1, main pointer + inc
		case 0x3c:		OCTread(ef9345, 5, 0);	break;		//OCT: RAM->R1, aux pointer
		case 0x3d:		OCTread(ef9345, 5, 1);	break;		//OCT: RAM->R1, aux pointer + inc
		case 0x40:								break;		//KRC: R1 -> ram
		case 0x41:								break;		//KRC: R1 -> ram + inc
		case 0x48:								break;		//KRC: 80 characters - 8 bits
		case 0x49:								break;		//KRC: 80 characters - 8 bits
		case 0x50:		KRLwrite(ef9345, 0);	break;		//KRL: 80 UINT8 - 12 bits write
		case 0x51:		KRLwrite(ef9345, 1);	break;		//KRL: 80 UINT8 - 12 bits write + inc
		case 0x58:		KRLread(ef9345, 0);		break;		//KRL: 80 UINT8 - 12 bits read
		case 0x59:		KRLread(ef9345, 1);		break;		//KRL: 80 UINT8 - 12 bits read + inc
		case 0x80:								break;		//IND: R1->ROM (impossible ?)
		case 0x81:		INDwrite(ef9345, 1);	break;		//IND: R1->ef9345->TGS
		case 0x82:		INDwrite(ef9345, 2);	break;		//IND: R1->ef9345->MAT
		case 0x83:		INDwrite(ef9345, 3);	break;		//IND: R1->ef9345->PAT
		case 0x84:		INDwrite(ef9345, 4);	break;		//IND: R1->ef9345->DOR
		case 0x87:		INDwrite(ef9345, 7);	break;		//IND: R1->ef9345->ROR
		case 0x88:		INDread(ef9345, 0);		break;		//IND: ROM->R1
		case 0x89:		INDread(ef9345, 1);		break;		//IND: ef9345->TGS->R1
		case 0x8a:		INDread(ef9345, 2);		break;		//IND: ef9345->MAT->R1
		case 0x8b:		INDread(ef9345, 3);		break;		//IND: ef9345->PAT->R1
		case 0x8c:		INDread(ef9345, 4);		break;		//IND: ef9345->DOR->R1
		case 0x8f:		INDread(ef9345, 7);		break;		//IND: ef9345->ROR->R1
		case 0x90:								break;		//NOP: no operation
		case 0x91:								break;		//NOP: no operation
		case 0x95:								break;		//VRM: vertical sync mask reset
		case 0x99:								break;		//VSM: vertical sync mask set
		case 0xb0:		INY(ef9345);			break;		//INY: increment Y
		case 0xd5:		MVB(ef9345, 1, 7, 5, 1);	break;	//MVB: move buffer MP->AP stop
		case 0xd6:		MVB(ef9345, 1, 7, 5, 0);	break;	//MVB: move buffer MP->AP nostop
		case 0xd9:		MVB(ef9345, 1, 5, 7, 1);	break;	//MVB: move buffer AP->MP stop
		case 0xda:		MVB(ef9345, 1, 5, 7, 0);	break;	//MVB: move buffer AP->MP nostop
		case 0xe5:		MVB(ef9345, 2, 7, 5, 1);	break;	//MVD: move double buffer MP->AP stop
		case 0xe6:		MVB(ef9345, 2, 7, 5, 0);	break;	//MVD: move double buffer MP->AP nostop
		case 0xe9:		MVB(ef9345, 2, 5, 7, 1);	break;	//MVD: move double buffer AP->MP stop
		case 0xea:		MVB(ef9345, 2, 5, 7, 0);	break;	//MVD: move double buffer AP->MP nostop
		case 0xf5:		MVB(ef9345, 3, 7, 5, 1);	break;	//MVT: move triple buffer MP->AP stop
		case 0xf6:		MVB(ef9345, 3, 7, 5, 0);	break;	//MVT: move triple buffer MP->AP nostop
		case 0xf9:		MVB(ef9345, 3, 5, 7, 1);	break;	//MVT: move triple buffer AP->MP stop
		case 0xfa:		MVB(ef9345, 3, 5, 7, 0);	break;	//MVT: move triple buffer AP->MP nostop
		default:		logerror("Unemulated EF9345 cmd: %02x\n", cmd);
	}
}


/**************************************************************
            EF9345 interface
**************************************************************/

void video_update_ef9345(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	ef9345_t *ef9345 = get_safe_token(device);
	copybitmap(bitmap, ef9345->screen_out, 0, 0, 0, 0, cliprect);
}


READ8_DEVICE_HANDLER ( ef9345_r )
{
	ef9345_t *ef9345 = get_safe_token(device);

	if(offset & 7)
		return ef9345->registers[offset & 7];

	if(ef9345->bf)
		ef9345->state |= 0x80;
	else
		ef9345->state &= 0x7f;

	return ef9345->state;
}


WRITE8_DEVICE_HANDLER ( ef9345_w )
{
	ef9345_t *ef9345 = get_safe_token(device);

	ef9345->registers[offset & 7] = data;

	if(offset & 8)
		ef9345_exec(ef9345, ef9345->registers[0] & 0xff);
}


static TIMER_CALLBACK( ef9345_done )
{
	running_device *device = (running_device *)ptr;
	ef9345_t *ef9345 = get_safe_token(device);

	ef9345->bf = 0;
}

void ef9345_scanline(running_device *device, UINT16 scanline)
{
	ef9345_t *ef9345 = get_safe_token(device);
	UINT8 i;

	if (scanline == 250)
		ef9345->state &= 0xfb;

	set_busy_flag(ef9345, 104);

	draw_char_40(ef9345, ef9345->border, 0, (scanline / 10) + 1);
	draw_char_40(ef9345, ef9345->border, 41, (scanline / 10) + 1);

	if(scanline == 0)
	{
		ef9345->state |= 0x04;
		for(i = 0; i < 42; i++)
			draw_char_40(ef9345, ef9345->border, i, 0);

		if(ef9345->PAT & 1)
			for(i = 0; i < 40; i++)
				makechar(ef9345, i, (scanline / 10));
		else
			for(i = 0; i < 42; i++)
				draw_char_40(ef9345, ef9345->border, i, 1);
	}
	else if(scanline < 120)
	{
		if(ef9345->PAT & 2)
			for(i = 0; i < 40; i++)
				makechar(ef9345, i, (scanline / 10));
		else
			for(i = 0; i < 42; i++)
				draw_char_40(ef9345, ef9345->border, i, (scanline / 10));
	}
	else if(scanline < 250)
	{
		if(ef9345->PAT & 4)
			for(i = 0; i < 40; i++)
				makechar(ef9345, i, (scanline / 10));
		else
			for(i = 0; i < 42; i++)
				draw_char_40(ef9345, ef9345->border, i, (scanline / 10));

		if(scanline == 240)
			for(i = 0; i < 42; i++)
				draw_char_40(ef9345, ef9345->border, i, 26);
	}
}


static TIMER_CALLBACK( blinking )
{
	running_device *device = (running_device *)ptr;
	ef9345_t *ef9345 = get_safe_token(device);

	ef9345->blink = (!ef9345->blink) & 0x01;
}


/*-------------------------------------------------
    DEVICE_RESET( ef9345 )
-------------------------------------------------*/
static DEVICE_RESET( ef9345 )
{
	ef9345_t *ef9345 = get_safe_token(device);

	ef9345->TGS =  0;
	ef9345->MAT =  0;
	ef9345->PAT =  0;
	ef9345->DOR =  0;
	ef9345->ROR =  0;
	ef9345->state = 0;
	ef9345->bf = 0;
	ef9345->blink = 0;
	ef9345->dial = 0;
	ef9345->latchc0 = 0;
	ef9345->latchm = 0;
	ef9345->latchi = 0;
	ef9345->latchu = 0;
	ef9345->char_mode = 0;

	memset(ef9345->last_dial, 0, sizeof(ef9345->last_dial));
	memset(ef9345->ram, 0, sizeof(ef9345->ram));
	memset(ef9345->registers, 0, sizeof(ef9345->registers));
	memset(ef9345->border, 0, sizeof(ef9345->border));

	bitmap_fill(ef9345->screen_out, NULL, 0);

	set_ef9345_mode(ef9345);
}


/*-------------------------------------------------
    DEVICE_START( ef9345 )
-------------------------------------------------*/
static DEVICE_START( ef9345 )
{
	ef9345_t *ef9345 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);

	ef9345->conf = (ef9345_config*)device->baseconfig().static_config();

	ef9345->charset = memory_region(device->machine, ef9345->conf->charset);

	ef9345->busy_timer = timer_alloc(device->machine, ef9345_done, (void *)device);
	ef9345->blink_timer = timer_alloc(device->machine, blinking, (void *)device);

	ef9345->screen_out = auto_bitmap_alloc(device->machine, ef9345->conf->width , ef9345->conf->height , BITMAP_FORMAT_INDEXED16);

	timer_adjust_periodic(ef9345->blink_timer, ATTOTIME_IN_MSEC(500), 0, ATTOTIME_IN_MSEC(500));

	init_char_prt(ef9345);

	state_save_register_device_item_array(device, 0, ef9345->ram);
	state_save_register_device_item_array(device, 0, ef9345->border);
	state_save_register_device_item_array(device, 0, ef9345->registers);
	state_save_register_device_item_array(device, 0, ef9345->last_dial);

	state_save_register_device_item(device, 0, ef9345->bf);
	state_save_register_device_item(device, 0, ef9345->char_mode);
	state_save_register_device_item(device, 0, ef9345->state);
	state_save_register_device_item(device, 0, ef9345->TGS);
	state_save_register_device_item(device, 0, ef9345->MAT);
	state_save_register_device_item(device, 0, ef9345->PAT);
	state_save_register_device_item(device, 0, ef9345->DOR);
	state_save_register_device_item(device, 0, ef9345->ROR);
	state_save_register_device_item(device, 0, ef9345->blink);
	state_save_register_device_item(device, 0, ef9345->dial);
	state_save_register_device_item(device, 0, ef9345->latchc0);
	state_save_register_device_item(device, 0, ef9345->latchm);
	state_save_register_device_item(device, 0, ef9345->latchi);
	state_save_register_device_item(device, 0, ef9345->latchu);

	state_save_register_device_item_bitmap(device, 0, ef9345->screen_out);
}

DEVICE_GET_INFO( ef9345 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
	case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ef9345_t);				break;
	case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(ef9345_config);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
	case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(ef9345);	break;
	case DEVINFO_FCT_STOP:							/* Nothing */								break;
	case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(ef9345);	break;
	case DEVINFO_PTR_ROM_REGION:					/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
	case DEVINFO_STR_NAME:							strcpy(info->s, "SGS Thomson EF9345");		break;
	case DEVINFO_STR_FAMILY:						strcpy(info->s, "EF9345");					break;
	case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
	case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
	case DEVINFO_STR_CREDITS:						strcpy(info->s, "Daniel Coulom");					break;
	}
}

DEFINE_LEGACY_DEVICE(EF9345, ef9345);
