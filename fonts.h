#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace fonts
{
// Letter size in pixels
constexpr int lettersWidth = 8;
constexpr int lettersHeight = 8;

// Get a line of pixels for a letter
std::uint8_t getLetterScanLine(char letter, int line);
// Invoke the callback with the coordinates of each pixel of the letter 
void renderLetter(char letter,
                  std::function<void(int x, int y)> pixelsCallback);
// Invoke the callback with the position of each pixel of the text
// (coordinates are normalized for each letter, meaning the first one has
// x in [0, 1[, second has x in [1, 2[, etc. and y is always in [0, 1[
void renderText(const std::string& text,
                std::function<void(float x, float y)> positionsCallback);

} // namespace fonts
