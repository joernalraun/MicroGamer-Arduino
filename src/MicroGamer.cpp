/**
 * @file MicroGamer.cpp
 * \brief
 * The MicroGamerBase and MicroGamer classes and support objects and definitions.
 */

#include "MicroGamer.h"
#include "ab_logo.c"
#include "glcdfont.c"

//==========================================
//========== class MicroGamerBase ==========
//==========================================

uint8_t MicroGamerBase::staticAllocatedBuffer[];
uint8_t *MicroGamerBase::displayBuffer;
uint8_t *MicroGamerBase::sBuffer;

MicroGamerBase::MicroGamerBase()
{
  currentButtonState = 0;
  previousButtonState = 0;
  // frame management
  setFrameRate(60);
  frameCount = -1;
  nextFrameStart = 0;
  justRendered = false;
  // init not necessary, will be reset after first use
  // lastFrameStart
  // lastFrameDurationMs

  sBuffer = staticAllocatedBuffer;
  displayBuffer = NULL;
}

// functions called here should be public so users can create their
// own init functions if they need different behavior than `begin`
// provides by default
void MicroGamerBase::begin()
{
  boot(); // raw hardware

  clear();
  display();
  waitDisplayUpdate();

  flashlight(); // light the RGB LED and screen if UP button is being held.

  // check for and handle buttons held during start up for system control
  systemButtons();

  audio.begin();

  // bootLogoText();
  // alternative logo functions. Work the same a bootLogo() but may reduce
  // memory size if the sketch uses the same bitmap drawing function
  // bootLogoCompressed();
  // bootLogoSpritesSelfMasked();
  // bootLogoSpritesOverwrite();

  // wait for all buttons to be released
  do {
    delayShort(50);
  } while (buttonsState());
}

void MicroGamerBase::flashlight()
{
  // if (!pressed(UP_BUTTON)) {
  //   return;
  // }

  // sendLCDCommand(OLED_ALL_PIXELS_ON); // smaller than allPixelsOn()
  // digitalWriteRGB(RGB_ON, RGB_ON, RGB_ON);

  // // prevent the bootloader magic number from being overwritten by timer 0
  // // when a timer variable overlaps the magic number location, for when
  // // flashlight mode is used for upload problem recovery
  // power_timer0_disable();

  // while (true) {
  //   idle();
  // }
}

void MicroGamerBase::systemButtons()
{
  // while (pressed(B_BUTTON)) {
  //   digitalWriteRGB(BLUE_LED, RGB_ON); // turn on blue LED
  //   sysCtrlSound(UP_BUTTON + B_BUTTON, GREEN_LED, 0xff);
  //   sysCtrlSound(DOWN_BUTTON + B_BUTTON, RED_LED, 0);
  //   delayShort(200);
  // }

  // digitalWriteRGB(BLUE_LED, RGB_OFF); // turn off blue LED
}

void MicroGamerBase::sysCtrlSound(uint8_t buttons, uint8_t led, uint8_t eeVal)
{
  // if (pressed(buttons)) {
  //   digitalWriteRGB(BLUE_LED, RGB_OFF); // turn off blue LED
  //   delayShort(200);
  //   digitalWriteRGB(led, RGB_ON); // turn on "acknowledge" LED
  //   EEPROM.update(EEPROM_AUDIO_ON_OFF, eeVal);
  //   delayShort(500);
  //   digitalWriteRGB(led, RGB_OFF); // turn off "acknowledge" LED

  //   while (pressed(buttons)) { } // Wait for button release
  // }
}

void MicroGamerBase::bootLogo()
{
  bootLogoShell(drawLogoBitmap);
}

void MicroGamerBase::drawLogoBitmap(int16_t y)
{
  drawBitmap(20, y, arduboy_logo, 88, 16);
}

void MicroGamerBase::bootLogoCompressed()
{
  bootLogoShell(drawLogoCompressed);
}

void MicroGamerBase::drawLogoCompressed(int16_t y)
{
  drawCompressed(20, y, arduboy_logo_compressed);
}

void MicroGamerBase::bootLogoSpritesSelfMasked()
{
  bootLogoShell(drawLogoSpritesSelfMasked);
}

void MicroGamerBase::drawLogoSpritesSelfMasked(int16_t y)
{
  Sprites::drawSelfMasked(20, y, arduboy_logo_sprite, 0);
}

void MicroGamerBase::bootLogoSpritesOverwrite()
{
  bootLogoShell(drawLogoSpritesOverwrite);
}

void MicroGamerBase::drawLogoSpritesOverwrite(int16_t y)
{
  Sprites::drawOverwrite(20, y, arduboy_logo_sprite, 0);
}

// bootLogoText() should be kept in sync with bootLogoShell()
// if changes are made to one, equivalent changes should be made to the other
void MicroGamerBase::bootLogoShell(void (*drawLogo)(int16_t))
{

  // for (int16_t y = -18; y <= 24; y++) {
  //   if (pressed(RIGHT_BUTTON)) {
  //     return;
  //   }

  //   clear();
  //   (*drawLogo)(y); // call the function that actually draws the logo
  //   display();
  //   delayShort(27);
  //   // longer delay post boot, we put it inside the loop to
  //   // save the flash calling clear/delay again outside the loop
  //   if (y==-16) {
  //     delayShort(250);
  //   }
  // }

  // delayShort(700);

  // bootLogoExtra();
}

// Virtual function overridden by derived class
void MicroGamerBase::bootLogoExtra() { }

/* Frame management */

void MicroGamerBase::setFrameRate(uint8_t rate)
{
  eachFrameMillis = 1000 / rate;
}

bool MicroGamerBase::everyXFrames(uint8_t frames)
{
  return frameCount % frames == 0;
}

bool MicroGamerBase::nextFrame()
{
  unsigned long now = millis();
  bool tooSoonForNextFrame = now < nextFrameStart;

  if (justRendered) {
    lastFrameDurationMs = now - lastFrameStart;
    justRendered = false;
    return false;
  }
  else if (tooSoonForNextFrame) {
    // if we have MORE than 1ms to spare (hence our comparison with 2),
    // lets sleep for power savings.  We don't compare against 1 to avoid
    // potential rounding errors - say we're actually 0.5 ms away, but a 1
    // is returned if we go to sleep we might sleep a full 1ms and then
    // we'd be running the frame slighly late.  So the last 1ms we stay
    // awake for perfect timing.

    // This is likely trading power savings for absolute timing precision
    // and the power savings might be the better goal. At 60 FPS trusting
    // chance here might actually achieve a "truer" 60 FPS than the 16ms
    // frame duration we get due to integer math.

    // We should be woken up by timer0 every 1ms, so it's ok to sleep.
    if ((uint8_t)(nextFrameStart - now) >= 2)
      idle();

    return false;
  }

  // pre-render
  justRendered = true;
  lastFrameStart = now;
  nextFrameStart = now + eachFrameMillis;
  frameCount++;

  return true;
}

bool MicroGamerBase::nextFrameDEV()
{
  bool ret = nextFrame();

  // if (ret) {
  //   if (lastFrameDurationMs > eachFrameMillis)
  //     TXLED1;
  //   else
  //     TXLED0;
  // }
  return ret;
}

int MicroGamerBase::cpuLoad()
{
  return lastFrameDurationMs*100 / eachFrameMillis;
}

void MicroGamerBase::initRandomSeed()
{
  // power_adc_enable(); // ADC on

  // // do an ADC read from an unconnected input pin
  // ADCSRA |= _BV(ADSC); // start conversion (ADMUX has been pre-set in boot())
  // while (bit_is_set(ADCSRA, ADSC)) { } // wait for conversion complete

  // randomSeed(((unsigned long)ADC << 16) + micros());

  // power_adc_disable(); // ADC off

  randomSeed(analogRead(0));

}

/* Graphics */

void MicroGamerBase::clear()
{
  fillScreen(BLACK);
}


// // Used by drawPixel to help with left bitshifting since AVR has no
// // multiple bit shift instruction.  We can bit shift from a lookup table
// // in flash faster than we can calculate the bit shifts on the CPU.
// const uint8_t bitshift_left[] PROGMEM = {
//   _BV(0), _BV(1), _BV(2), _BV(3), _BV(4), _BV(5), _BV(6), _BV(7)
// };

void MicroGamerBase::drawPixel(int16_t x, int16_t y, uint8_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) {
    return;
  }

  // x is which column
  switch (color)
  {
    case WHITE:   sBuffer[x+ (y/8)*WIDTH] |=  (1 << (y&7)); break;
    case BLACK:   sBuffer[x+ (y/8)*WIDTH] &= ~(1 << (y&7)); break;
    case INVERSE: sBuffer[x+ (y/8)*WIDTH] ^=  (1 << (y&7)); break;
  }
}

uint8_t MicroGamerBase::getPixel(uint8_t x, uint8_t y)
{
  uint8_t row = y / 8;
  uint8_t bit_position = y % 8;
  return (sBuffer[(row*WIDTH) + x] & bit_position) >> bit_position;
}

void MicroGamerBase::drawCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0+r, color);
  drawPixel(x0, y0-r, color);
  drawPixel(x0+r, y0, color);
  drawPixel(x0-r, y0, color);

  while (x<y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void MicroGamerBase::drawCircleHelper
(int16_t x0, int16_t y0, uint8_t r, uint8_t corners, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x<y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (corners & 0x4) // lower right
    {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (corners & 0x2) // upper right
    {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (corners & 0x8) // lower left
    {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (corners & 0x1) // upper left
    {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void MicroGamerBase::fillCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color)
{
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void MicroGamerBase::fillCircleHelper
(int16_t x0, int16_t y0, uint8_t r, uint8_t sides, int16_t delta,
 uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (sides & 0x1) // right side
    {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }

    if (sides & 0x2) // left side
    {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

void MicroGamerBase::drawLine
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  // bresenham's algorithm - thx wikpedia
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      drawPixel(y0, x0, color);
    }
    else
    {
      drawPixel(x0, y0, color);
    }

    err -= dy;
    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}

void MicroGamerBase::drawRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

void MicroGamerBase::drawFastVLine
(int16_t x, int16_t y, uint8_t h, uint8_t color)
{
  int end = y+h;
  for (int a = max(0,y); a < min(end,HEIGHT); a++)
  {
    drawPixel(x,a,color);
  }
}

void MicroGamerBase::drawFastHLine
(int16_t x, int16_t y, uint8_t w, uint8_t color)
{
  int16_t xEnd; // last x point + 1

  // Do y bounds checks
  if (y < 0 || y >= HEIGHT)
    return;

  xEnd = x + w;

  // Check if the entire line is not on the display
  if (xEnd <= 0 || x >= WIDTH)
    return;

  // Don't start before the left edge
  if (x < 0)
    x = 0;

  // Don't end past the right edge
  if (xEnd > WIDTH)
    xEnd = WIDTH;

  // calculate actual width (even if unchanged)
  w = xEnd - x;

  // buffer pointer plus row offset + x offset
  register uint8_t *pBuf = sBuffer + ((y / 8) * WIDTH) + x;

  // pixel mask
  register uint8_t mask = 1 << (y & 7);

  switch (color)
  {
    case WHITE:
      while (w--)
      {
        *pBuf++ |= mask;
      }
      break;

    case BLACK:
      mask = ~mask;
      while (w--)
      {
        *pBuf++ &= mask;
      }
      break;
  }
}

void MicroGamerBase::fillRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
  // stupidest version - update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++)
  {
    drawFastVLine(i, y, h, color);
  }
}

void MicroGamerBase::fillScreen(uint8_t color)
{
    memset(sBuffer, 0, WIDTH*HEIGHT/8);
}

void MicroGamerBase::drawRoundRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t color)
{
  // smarter version
  drawFastHLine(x+r, y, w-2*r, color); // Top
  drawFastHLine(x+r, y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x, y+r, h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r, h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r, y+r, r, 1, color);
  drawCircleHelper(x+w-r-1, y+r, r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r, y+h-r-1, r, 8, color);
}

void MicroGamerBase::fillRoundRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t color)
{
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r, y+r, r, 2, h-2*r-1, color);
}

void MicroGamerBase::drawTriangle
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

void MicroGamerBase::fillTriangle
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{

  int16_t a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1)
  {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2)
  {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1)
  {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2)
  { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)
    {
      a = x1;
    }
    else if(x1 > b)
    {
      b = x1;
    }
    if(x2 < a)
    {
      a = x2;
    }
    else if(x2 > b)
    {
      b = x2;
    }
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t dx01 = x1 - x0,
      dy01 = y1 - y0,
      dx02 = x2 - x0,
      dy02 = y2 - y0,
      dx12 = x2 - x1,
      dy12 = y2 - y1,
      sa = 0,
      sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
  {
    last = y1;   // Include y1 scanline
  }
  else
  {
    last = y1-1; // Skip it
  }


  for(y = y0; y <= last; y++)
  {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if(a > b)
    {
      swap(a,b);
    }

    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);

  for(; y <= y2; y++)
  {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if(a > b)
    {
      swap(a,b);
    }

    drawFastHLine(a, y, b-a+1, color);
  }
}

void MicroGamerBase::drawBitmap
(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h,
 uint8_t color)
{
  // no need to draw at all if we're offscreen
  if (x+w < 0 || x > WIDTH-1 || y+h < 0 || y > HEIGHT-1)
    return;

  int yOffset = abs(y) % 8;
  int sRow = y / 8;
  if (y < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h/8;
  if (h%8!=0) rows++;
  for (int a = 0; a < rows; a++) {
    int bRow = sRow + a;
    if (bRow > (HEIGHT/8)-1) break;
    if (bRow > -2) {
      for (int iCol = 0; iCol<w; iCol++) {
        if (iCol + x > (WIDTH-1)) break;
        if (iCol + x >= 0) {
          if (bRow >= 0) {
            if (color == WHITE)
              sBuffer[(bRow*WIDTH) + x + iCol] |= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
            else if (color == BLACK)
              sBuffer[(bRow*WIDTH) + x + iCol] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) << yOffset);
            else
              sBuffer[(bRow*WIDTH) + x + iCol] ^= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
          }
          if (yOffset && bRow<(HEIGHT/8)-1 && bRow > -2) {
            if (color == WHITE)
              sBuffer[((bRow+1)*WIDTH) + x + iCol] |= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
            else if (color == BLACK)
              sBuffer[((bRow+1)*WIDTH) + x + iCol] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset));
            else
              sBuffer[((bRow+1)*WIDTH) + x + iCol] ^= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
          }
        }
      }
    }
  }
}

void MicroGamerBase::drawSlowXYBitmap
(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color)
{
  // no need to draw at all of we're offscreen
  if (x+w < 0 || x > WIDTH-1 || y+h < 0 || y > HEIGHT-1)
    return;

  int16_t xi, yi, byteWidth = (w + 7) / 8;
  for(yi = 0; yi < h; yi++) {
    for(xi = 0; xi < w; xi++ ) {
      if(pgm_read_byte(bitmap + yi * byteWidth + xi / 8) & (128 >> (xi & 7))) {
        drawPixel(x + xi, y + yi, color);
      }
    }
  }
}

typedef struct CSESSION {
  int byte;
  int bit;
  const uint8_t *src;
  int src_pos;
} CSESSION;
static CSESSION cs;

static int getval(int bits)
{
  int val = 0;
  int i;
  for (i = 0; i < bits; i++)
  {
    if (cs.bit == 0x100)
    {
      cs.bit = 0x1;
      cs.byte = pgm_read_byte(&cs.src[cs.src_pos]);
      cs.src_pos ++;
    }
    if (cs.byte & cs.bit)
      val += (1 << i);
    cs.bit <<= 1;
  }
  return val;
}

void MicroGamerBase::drawCompressed(int16_t sx, int16_t sy, const uint8_t *bitmap, uint8_t color)
{
  int bl, len;
  int col;
  int i;
  int a, iCol;
  int byte = 0;
  int bit = 0;
  int w, h;

  // set up decompress state

  cs.src = bitmap;
  cs.bit = 0x100;
  cs.byte = 0;
  cs.src_pos = 0;

  // read header

  w = getval(8) + 1;
  h = getval(8) + 1;

  col = getval(1); // starting colour

  // no need to draw at all if we're offscreen
  if (sx + w < 0 || sx > WIDTH - 1 || sy + h < 0 || sy > HEIGHT - 1)
    return;

  // sy = sy - (frame*h);

  int yOffset = abs(sy) % 8;
  int sRow = sy / 8;
  if (sy < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h / 8;
  if (h % 8 != 0) rows++;

  a = 0; // +(frame*rows);
  iCol = 0;

  byte = 0; bit = 1;
  while (a < rows) // + (frame*rows))
  {
    bl = 1;
    while (!getval(1))
      bl += 2;

    len = getval(bl) + 1; // span length

    // draw the span


    for (i = 0; i < len; i++)
    {
      if (col)
        byte |= bit;
      bit <<= 1;

      if (bit == 0x100) // reached end of byte
      {
        // draw

        int bRow = sRow + a;

        //if (byte) // possible optimisation
        if (bRow <= (HEIGHT / 8) - 1)
          if (bRow > -2)
            if (iCol + sx <= (WIDTH - 1))
              if (iCol + sx >= 0) {

                if (bRow >= 0)
                {
                  if (color)
                    sBuffer[(bRow * WIDTH) + sx + iCol] |= byte << yOffset;
                  else
                    sBuffer[(bRow * WIDTH) + sx + iCol] &= ~(byte << yOffset);
                }
                if (yOffset && bRow < (HEIGHT / 8) - 1 && bRow > -2)
                {
                  if (color)
                    sBuffer[((bRow + 1)*WIDTH) + sx + iCol] |= byte >> (8 - yOffset);
                  else
                    sBuffer[((bRow + 1)*WIDTH) + sx + iCol] &= ~(byte >> (8 - yOffset));
                }
              }

        // iterate
        iCol ++;
        if (iCol >= w)
        {
          iCol = 0;
          a ++;
        }

        // reset byte
        byte = 0; bit = 1;
      }
    }

    col = 1 - col; // toggle colour for next span
  }
}

void MicroGamerBase::display()
{
  uint8_t *tmp;

  waitEndOfPaintScreen();

  if(displayBuffer != NULL) {
    tmp = displayBuffer;
    displayBuffer = sBuffer;
    sBuffer = tmp;
    paintScreen(displayBuffer);
  } else {
      paintScreen(sBuffer);
  }
}

void MicroGamerBase::display(bool clear)
{
  display();
  if (clear) {
    this->clear();
  }
}

void MicroGamerBase::waitDisplayUpdate()
{
  waitEndOfPaintScreen();
}

void MicroGamerBase::enableDoubleBuffer()
{
  if(displayBuffer == NULL) {
    displayBuffer = (uint8_t *) malloc(((HEIGHT * WIDTH) / 8) * sizeof(uint8_t));
  }
}

bool MicroGamerBase::doubleBuffer()
{
  return displayBuffer != NULL;
}

uint8_t* MicroGamerBase::getBuffer()
{
    return sBuffer;
}

bool MicroGamerBase::pressed(uint8_t buttons)
{
  return (buttonsState() & buttons) == buttons;
}

bool MicroGamerBase::notPressed(uint8_t buttons)
{
  return (buttonsState() & buttons) == 0;
}

void MicroGamerBase::pollButtons()
{
  previousButtonState = currentButtonState;
  currentButtonState = buttonsState();
}

bool MicroGamerBase::justPressed(uint8_t button)
{
  return (!(previousButtonState & button) && (currentButtonState & button));
}

bool MicroGamerBase::justReleased(uint8_t button)
{
  return ((previousButtonState & button) && !(currentButtonState & button));
}

bool MicroGamerBase::collide(Point point, Rect rect)
{
  return ((point.x >= rect.x) && (point.x < rect.x + rect.width) &&
      (point.y >= rect.y) && (point.y < rect.y + rect.height));
}

bool MicroGamerBase::collide(Rect rect1, Rect rect2)
{
  return !(rect2.x                >= rect1.x + rect1.width  ||
           rect2.x + rect2.width  <= rect1.x                ||
           rect2.y                >= rect1.y + rect1.height ||
           rect2.y + rect2.height <= rect1.y);
}

uint16_t MicroGamerBase::readUnitID()
{
    //TODO
    return 0;
}

void MicroGamerBase::writeUnitID(uint16_t id)
{
  // TODO
  // EEPROM.update(EEPROM_UNIT_ID, (uint8_t)(id & 0xff));
  // EEPROM.update(EEPROM_UNIT_ID + 1, (uint8_t)(id >> 8));
}

uint8_t MicroGamerBase::readUnitName(char* name)
{
  // char val;
  // uint8_t dest;
  // uint8_t src = EEPROM_UNIT_NAME;

  // for (dest = 0; dest < ARDUBOY_UNIT_NAME_LEN; dest++)
  // {
  //   val = EEPROM.read(src);
  //   name[dest] = val;
  //   src++;
  //   if (val == 0x00 || (byte)val == 0xFF) {
  //     break;
  //   }
  // }

  // name[dest] = 0x00;
  // return dest;
  return 0;
}

void MicroGamerBase::writeUnitName(char* name)
{
  // bool done = false;
  // uint8_t dest = EEPROM_UNIT_NAME;

  // for (uint8_t src = 0; src < ARDUBOY_UNIT_NAME_LEN; src++)
  // {
  //   if (name[src] == 0x00) {
  //     done = true;
  //   }
  //   // write character or 0 pad if finished
  //   EEPROM.update(dest, done ? 0x00 : name[src]);
  //   dest++;
  // }
}

bool MicroGamerBase::readShowUnitNameFlag()
{
  // return (EEPROM.read(EEPROM_SYS_FLAGS) & SYS_FLAG_UNAME_MASK);
    return 0;
}

void MicroGamerBase::writeShowUnitNameFlag(bool val)
{
  // uint8_t flags = EEPROM.read(EEPROM_SYS_FLAGS);

  // bitWrite(flags, SYS_FLAG_UNAME, val);
  // EEPROM.update(EEPROM_SYS_FLAGS, flags);
}

void MicroGamerBase::swap(int16_t& a, int16_t& b)
{
  int16_t temp = a;
  a = b;
  b = temp;
}


//======================================
//========== class MicroGamer ==========
//======================================

MicroGamer::MicroGamer()
{
  cursor_x = 0;
  cursor_y = 0;
  textColor = 1;
  textBackground = 0;
  textSize = 1;
  textWrap = 0;
}

// bootLogoText() should be kept in sync with bootLogoShell()
// if changes are made to one, equivalent changes should be made to the other
void MicroGamer::bootLogoText()
{
  textSize = 2;

  for (int8_t y = -18; y <= 24; y++) {
    clear();
    cursor_x = 23;
    cursor_y = y;
    print("Micro:Gamer");
    display();
    delayShort(27);
    // longer delay post boot, we put it inside the loop to
    // save the flash calling clear/delay again outside the loop
    if (y==-16) {
      delayShort(250);
    }
  }

  delayShort(700);
  textSize = 1;

  bootLogoExtra();
}

void MicroGamer::bootLogoExtra()
{
  // uint8_t c;

  // if (!readShowUnitNameFlag())
  // {
  //   return;
  // }

  // c = EEPROM.read(EEPROM_UNIT_NAME);

  // if (c != 0xFF && c != 0x00)
  // {
  //   uint8_t i = EEPROM_UNIT_NAME;
  //   cursor_x = 50;
  //   cursor_y = 56;

  //   do
  //   {
  //     write(c);
  //     c = EEPROM.read(++i);
  //   }
  //   while (i < EEPROM_UNIT_NAME + ARDUBOY_UNIT_NAME_LEN);

  //   display();
  //   delayShort(1000);
  // }
}

size_t MicroGamer::write(uint8_t c)
{
  if (c == '\n')
  {
    cursor_y += textSize * 8;
    cursor_x = 0;
  }
  else if (c == '\r')
  {
    // skip em
  }
  else
  {
    drawChar(cursor_x, cursor_y, c, textColor, textBackground, textSize);
    cursor_x += textSize * 6;
    if (textWrap && (cursor_x > (WIDTH - textSize * 6)))
    {
      // calling ourselves recursively for 'newline' is
      // 12 bytes smaller than doing the same math here
      write('\n');
    }
  }
  return 1;
}

void MicroGamer::drawChar
  (int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t bg, uint8_t size)
{
  uint8_t line;
  bool draw_background = bg != color;
  const unsigned char* bitmap = font + c * 5;

  if ((x >= WIDTH) ||              // Clip right
      (y >= HEIGHT) ||             // Clip bottom
      ((x + 5 * size - 1) < 0) ||  // Clip left
      ((y + 8 * size - 1) < 0)     // Clip top
     )
  {
    return;
  }

  for (uint8_t i = 0; i < 6; i++ )
  {
    line = pgm_read_byte(bitmap++);
    if (i == 5) {
      line = 0x0;
    }

    for (uint8_t j = 0; j < 8; j++)
    {
      uint8_t draw_color = (line & 0x1) ? color : bg;

      if (draw_color || draw_background) {
        for (uint8_t a = 0; a < size; a++ ) {
          for (uint8_t b = 0; b < size; b++ ) {
            drawPixel(x + (i * size) + a, y + (j * size) + b, draw_color);
          }
        }
      }
      line >>= 1;
    }
  }
}

void MicroGamer::setCursor(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}

int16_t MicroGamer::getCursorX()
{
  return cursor_x;
}

int16_t MicroGamer::getCursorY()
{
  return cursor_y;
}

void MicroGamer::setTextColor(uint8_t color)
{
  textColor = color;
}

uint8_t MicroGamer::getTextColor()
{
  return textColor;
}

void MicroGamer::setTextBackground(uint8_t bg)
{
  textBackground = bg;
}

uint8_t MicroGamer::getTextBackground()
{
  return textBackground;
}

void MicroGamer::setTextSize(uint8_t s)
{
  // size must always be 1 or higher
  textSize = max(1, s);
}

uint8_t MicroGamer::getTextSize()
{
  return textSize;
}

void MicroGamer::setTextWrap(bool w)
{
  textWrap = w;
}

bool MicroGamer::getTextWrap()
{
  return textWrap;
}

void MicroGamer::clear()
{
    MicroGamerBase::clear();
    cursor_x = cursor_y = 0;
}

