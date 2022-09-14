#ifndef __CT107D_H_
#define __CT107D_H_

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;

//-----------------------------------------
// select BUZZ mode, 1->open, 0->close
void SelectBUZZ(bit mode);

//-----------------------------------------
// select RELAY mode, 1->open, 0->close
void SelectRELAY(bit mode);

//-----------------------------------------
// select P0 Channel, 4->Y4, 5->Y5, 6->Y6, 7->Y7, other -> close
void SelectHC573(u8 channel);

#endif // __CT107D_H_