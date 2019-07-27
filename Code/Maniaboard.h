/*
  Keyboard.h

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett
  Modified by yclee126 for Maniaboard project

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MANIABOARD_h
#define MANIABOARD_h

#include "HID.h"

#if !defined(_USING_HID)

#warning "Using legacy HID core (non pluggable)"

#else

//================================================================================
//================================================================================
//  Keyboard

//  #define of keys are replaced with usb_hid_keys.h

//  Low level key report: up to 12 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[12];
} KeyReport;

class Keyboard_ : public Print
{
private:
  KeyReport _keyReport;
  void sendReport(KeyReport* keys);
public:
  Keyboard_(void);
  void begin(void);
  void end(void);
  size_t write(uint8_t k);
  size_t press(uint8_t k);
  size_t release(uint8_t k);
  size_t packet(uint8_t m = 0, uint8_t k1 = 0, uint8_t k2 = 0, uint8_t k3 = 0, uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0, uint8_t k7 = 0, uint8_t k8 = 0, uint8_t k9 = 0, uint8_t k10 = 0, uint8_t k11 = 0, uint8_t k12 = 0);
  void releaseAll(void);
};
extern Keyboard_ Keyboard;

#endif
#endif
