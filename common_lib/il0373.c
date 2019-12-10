#include <il0373.h>
#include <memory.h>
#include <lcd.h>

#if LCD_ORIENTATION == LCD_ORIENTATION_PORTRAIT
#define IL0373_GET_BUFFER_OFFSET(x, y) ((y * LCD_WIDTH + x) >> 3U)
#define IL0373_GET_BUFFER_BIT(x, y) (7U - (x % 8U))
#elif LCD_ORIENTATION == LCD_ORIENTATION_LANDSCAPE
#define IL0373_GET_BUFFER_OFFSET(x, y) ((LCD_HEIGHT - 1 + x * LCD_HEIGHT - y) >> 3U)
#define IL0373_GET_BUFFER_BIT(x, y) (y % 8U)
#elif LCD_ORIENTATION == LCD_ORIENTATION_PORTRAIT_REVERSED
#define IL0373_GET_BUFFER_OFFSET(x, y) ((LCD_WIDTH * LCD_HEIGHT - 1 - y * LCD_WIDTH - x) >> 3U)
#define IL0373_GET_BUFFER_BIT(x, y) (x % 8U)
#elif LCD_ORIENTATION == LCD_ORIENTATION_LANDSCAPE_REVERSED
#define IL0373_GET_BUFFER_OFFSET(x, y) (((LCD_WIDTH -1) * LCD_HEIGHT - x * LCD_HEIGHT + y) >> 3U)
#define IL0373_GET_BUFFER_BIT(x, y) (7U - (y % 8U))
#endif

static unsigned char blackBuffer[IL0373_DISPLAY_BUFFER_SIZE];
static unsigned char redBuffer[IL0373_DISPLAY_BUFFER_SIZE];

#define GET_BLACK(c) (c == IL0373_COLOR_BLACK ? 0U : 0xFFU)
#define GET_RED(c) (c == IL0373_COLOR_RED ? 0U : 0xFFU)

unsigned int LcdInit(void)
{
  LcdScreenFill(IL0373_COLOR_WHITE);
  return 1;
}

void LcdScreenFill(unsigned int color)
{
  memset(blackBuffer, GET_BLACK(color), sizeof(blackBuffer));
  memset(redBuffer, GET_RED(color), sizeof(redBuffer));
}

static void internalDrawPixel(unsigned int x, unsigned int y, unsigned int color)
{
  unsigned int o = IL0373_GET_BUFFER_OFFSET(x, y);
  unsigned int bit = IL0373_GET_BUFFER_BIT(x, y);
  unsigned char *bp = blackBuffer + o;
  unsigned char *rp = redBuffer + o;
  unsigned char v = 1U << bit;
  unsigned char nv = ~v;

  if (GET_BLACK(color))
    *bp |= v;
  else
    *bp &= nv;
  if (GET_RED(color))
    *rp |= v;
  else
    *rp &= nv;
}

void LcdRectFill(unsigned int x1, unsigned int y1, unsigned int dx, unsigned int dy, unsigned int color)
{
  unsigned int y, y2;

  if (y1 >= LCD_HEIGHT || x1 >= LCD_WIDTH)
    return;

  y2 = y1 + dy - 1;

  while (dx--)
  {
    for (y = y1; y <= y2; y++)
    {
      if (y >= LCD_HEIGHT)
        break;
      internalDrawPixel(x1, y1, color);
    }
    x1++;
    if (x1 >= LCD_WIDTH)
      break;
  }
}

static unsigned int ili0373_wakeUp(void)
{
  if (!ili0373_writeCommand(IL0373_COMMAND_BOOSTER_SOFT_START))
    return 0;
  if (!ili0373_writeData(0x17))
    return 0;
  if (!ili0373_writeData(0x17))
    return 0;
  if (!ili0373_writeData(0x17))
    return 0;
  if (!ili0373_writeCommand(IL0373_COMMAND_POWER_ON))
    return 0;
  ili0373_waitWhileBusy();
  if (!ili0373_writeCommand(IL0373_COMMAND_PANEL_SETTING))
    return 0;
  if (!ili0373_writeData(0x8F))
    return 0;
  if (!ili0373_writeCommand(IL0373_COMMAND_VCOM_DATA_INTERVAL_SETTING))
    return 0;
  if (!ili0373_writeData(0x37))
    return 0;
  if (!ili0373_writeCommand(IL0373_COMMAND_RESOLUTION_SETTING))
    return 0;
  if (!ili0373_writeData(LCD_DEFAULT_WIDTH))
    return 0;
  if (!ili0373_writeData(0))
    return 0;
  return ili0373_writeData(LCD_DEFAULT_HEIGHT);
}

static unsigned int ili0373_sleep(void)
{
  return ili0373_writeCommand(IL0373_COMMAND_POWER_OFF);
}

unsigned int LcdUpdate(void)
{
  unsigned int i;
  unsigned char *p;

  if (!ili0373_wakeUp())
    return 0;
  if (!ili0373_writeCommand(IL0373_COMMAND_START_DATA_TRANSMISSION1))
    return 0;
  p = blackBuffer;
  for (i = 0; i < IL0373_DISPLAY_BUFFER_SIZE; i++)
  {
    if (!ili0373_writeData(*p++))
      return 0;
  }
  if (!ili0373_writeCommand(IL0373_COMMAND_START_DATA_TRANSMISSION2))
    return 0;
  p = redBuffer;
  for (i = 0; i < IL0373_DISPLAY_BUFFER_SIZE; i++)
  {
    if (!ili0373_writeData(*p++))
      return 0;
  }
  if (!ili0373_writeCommand(IL0373_COMMAND_DISPLAY_REFRESH))
    return 0;
  ili0373_waitWhileBusy();
  return ili0373_sleep();
}

void il0373_writeData(unsigned int x, unsigned int y, const unsigned char *bitmap, unsigned int height,
                      unsigned int bit, unsigned int char_bytes, unsigned int textColor, unsigned int bkColor)
{
  if (bitmap != NULL)
    bitmap += bit >> 3U;
  bit =  1U << (7U - (bit % 8U));

  while (height)
  {
    if (bitmap != NULL && (*bitmap & bit))
      internalDrawPixel(x, y, textColor);
    else
      internalDrawPixel(x, y, bkColor);

    height--;
    y++;
    bitmap += char_bytes;
  }
}

unsigned int LcdDrawText(unsigned int x, unsigned int y, char *text, const FONT_INFO *f, unsigned int textColor, unsigned int bkColor, unsigned int *owidth)
{
  char c;
  unsigned int col, width, max_width, w, h, bit, character_bytes;
  const unsigned char *bitmap;
  static FONT_CHAR_INFO fci[DRAW_TEXT_MAX], *pfci;

  if (y >= LCD_HEIGHT || x >= LCD_WIDTH)
    return 0;

  col = x;
  pfci = fci;
  width = 0;
  max_width = LCD_WIDTH - col;

  while ((c = *text++) != 0 && max_width)
  {
    if (FontGetChar(c, f, pfci))
    {
      w = pfci->char_width > max_width ? max_width : pfci->char_width;
      col += w;
      width += w;
      max_width -= w;
    }
    pfci++;
    if (max_width)
    {
      w = f->character_spacing > max_width ? max_width : f->character_spacing;
      col += w;
      width += w;
      max_width -= w;
    }
  }

  pfci = fci;
  bitmap = f->char_bitmaps + pfci->char_offset;
  h = y + f->char_height > LCD_HEIGHT ? LCD_HEIGHT - y : f->char_height;
  w = width;
  bit = 0;
  character_bytes = pfci->char_width >> 3U;
  if (pfci->char_width & 7U)
    character_bytes++;

  while (w--)
  {
    il0373_writeData(x++, y, bitmap, h, bit++, character_bytes, textColor, bkColor);
    if (bit >= pfci->char_width)
    {
      if (!w)
        break;
      for (bit = 0; bit < f->character_spacing; bit++)
      {
        il0373_writeData(x++, y, 0, h, 0, 0, textColor, bkColor);
        w--;
        if (!w)
          return width;
      }
      pfci++;
      character_bytes = pfci->char_width >> 3;
      if (pfci->char_width & 7)
        character_bytes++;
      bitmap = f->char_bitmaps + pfci->char_offset;
      bit = 0;
    }
  }

  if (owidth)
    *owidth = width;

  return 1;
}
