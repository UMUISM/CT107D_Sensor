#include "CT107D.h"
#include "STC15.h"

//-----------------------------------------
// select BUZZ mode, 1->open, 0->close
void SelectBUZZ(bit mode) {
  SelectHC573(5); // Select Y5
  P0 = 0x00;
  if (mode) {
    P0 &= 0xef; // Open BUZZ
  } else {
    P0 |= 0x10; // Close BUZZ
  }
  SelectHC573(0); // Close Y5
}

//-----------------------------------------
// select RELAY mode, 1->open, 0->close
void SelectRELAY(bit mode) {
  SelectHC573(5); // Select Y5
  P0 = 0x00;
  if (mode) {
    P0 &= 0xb0; // Open Relay
  } else {
    P0 = 0x40; // Close Relay
  }
  SelectHC573(0); // Close Y5
}

//-----------------------------------------
// select P0 Channel, 4->Y4, 5->Y5, 6->Y6, 7->Y7, other -> close
void SelectHC573(u8 channel) {
  switch (channel) {
  case 4:
    P2 = (P2 & 0x1f) | 0x80;
    break;
  case 5:
    P2 = (P2 & 0x1f) | 0xa0;
    break;
  case 6: /* ถฮัก */
    P2 = (P2 & 0x1f) | 0xc0;
    break;
  case 7: /* ฮปัก */
    P2 = (P2 & 0x1f) | 0xe0;
    break;
  default:
    P2 &= 0x00;
    break;
  }
}
