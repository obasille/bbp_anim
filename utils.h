#pragma once

#include <ospcommon/vec.h>

namespace utils
{
// Convert colors from HSL space to RGB
ospcommon::vec3f hsl2RGB(float h, float s, float l);

// Write frame of pixels into a file
void writePPM(const char *fileName, const ospcommon::vec2i &size,
              const uint32_t *pixel);
} // namespace utils