#include "fonts.h"

// Collection of c-header fonts
// https://github.com/dhepper/font8x8
#include "font8x8_basic.h"

namespace fonts
{
inline bool isValidLetter(char letter)
{
    return (letter > 0) &&
           (letter < 128); // Null character always rendered empty
}

std::uint8_t getLetterScanLine(char letter, int line)
{
    if (isValidLetter(letter) && (line >= 0) && (line < lettersHeight))
    {
        return font8x8_basic[letter][line];
    }
    return 0;
}

void renderLetter(char letter, std::function<void(int x, int y)> pixelsCallback)
{
    // Iterate each line of the letter
    for (int y = 0; y < lettersHeight; ++y)
    {
        char line = getLetterScanLine(letter, y);

        // And iterate on each pixel of the line
        for (int x = 0; x < lettersWidth; ++x)
        {
            if (line & 1)
            {
                pixelsCallback(x, y);
            }
            line >>= 1;
        }
    }
}

void renderText(const std::string& text,
                std::function<void(float x, float y)> positionsCallback)
{
    const float pixelSize = 1.f / fonts::lettersWidth;
    float cursorX = 0, cursorY = 0;

    for (auto& c : text)
    {
        if (c == '\n')
        {
            cursorX = 0;
            ++cursorY;
        }
        else
        {
            fonts::renderLetter(c, [&](int x, int y) {
                positionsCallback(cursorX + x * pixelSize,
                                  cursorY + y * pixelSize);
            });
            ++cursorX;
        }
    }
}
} // namespace fonts
