#pragma once
// Compatibility shim to satisfy firmware includes and method names
// Maps custom mySSD1306Ascii APIs to SSD1306Ascii equivalents

#include <SSD1306Ascii.h>

#ifndef MYSSD1306_SHIM_DEFINED
#define MYSSD1306_SHIM_DEFINED

// Map firmware's methods to SSD1306Ascii methods on the same object
// Usage in code: _Display->Display_Normal(); becomes _Display->setInvertMode(false);
#define Display_Normal() setInvertMode(false)

// No-op: display is ON after begin(); you can implement full power control if needed
#define Display_On() /* no-op */

// Basic rotation/remap handling not provided here; keep as no-op to compile
// If you need rotation, consider using displayRemap(true/false) depending on library version
#define Display_Rotate(x) /* no-op */

// Set maximum contrast for brightness
#define Display_Bright() setContrast(255)

#endif // MYSSD1306_SHIM_DEFINED
