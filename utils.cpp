#include "utils.h"
#include <cstdio>
#include <cmath>
#include <functional>

using namespace ospcommon;

namespace utils
{
// https://stackoverflow.com/a/54014428
// input: h in [0,360] and s,v in [0,1] - output: r,g,b in [0,1]
vec3f hsl2RGB(float h, float s, float l)
{
    const float a = s * std::min(l, 1 - l);
    std::function<float(float)> f;
    f = [&](float n) {
        float k = std::fmodf(n + h / 30.f, 12.f);
        return l - a * std::max(std::min(std::min(k - 3, 9 - k), 1.f), -1.f);
    };
    return {f(0), f(8), f(4)};
}

// helper function to write the rendered image as PPM file
// (from OSPRay tutorials)
void writePPM(const char *fileName, const vec2i &size, const uint32_t *pixel)
{
    FILE *file = fopen(fileName, "wb");
    if (file == nullptr)
    {
        fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
        return;
    }
    fprintf(file, "P6\n%i %i\n255\n", size.x, size.y);
    unsigned char *out = (unsigned char *)alloca(3 * size.x);
    for (int y = 0; y < size.y; y++)
    {
        const unsigned char *in =
            (const unsigned char *)&pixel[(size.y - 1 - y) * size.x];
        for (int x = 0; x < size.x; x++)
        {
            out[3 * x + 0] = in[4 * x + 0];
            out[3 * x + 1] = in[4 * x + 1];
            out[3 * x + 2] = in[4 * x + 2];
        }
        fwrite(out, 3 * size.x, sizeof(char), file);
    }
    fprintf(file, "\n");
    fclose(file);
}

} // namespace utils