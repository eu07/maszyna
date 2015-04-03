#include "TextureDDS.h"

void DxtcReadColors(const GLubyte *Data, Color8888 *result)
{
    GLubyte r0, g0, b0, r1, g1, b1;

    b0 = Data[0] & 0x1F;
    g0 = ((Data[0] & 0xE0) >> 5) | ((Data[1] & 0x7) << 3);
    r0 = (Data[1] & 0xF8) >> 3;

    b1 = Data[2] & 0x1F;
    g1 = ((Data[2] & 0xE0) >> 5) | ((Data[3] & 0x7) << 3);
    r1 = (Data[3] & 0xF8) >> 3;

    result[0].r = r0 << 3 | r0 >> 2;
    result[0].g = g0 << 2 | g0 >> 3;
    result[0].b = b0 << 3 | b0 >> 2;

    result[1].r = r1 << 3 | r1 >> 2;
    result[1].g = g1 << 2 | g1 >> 3;
    result[1].b = b1 << 3 | b1 >> 2;
};

void DxtcReadColor(GLushort Data, Color8888 *Out)
{
    GLubyte r, g, b;

    b = Data & 0x1f;
    g = (Data & 0x7E0) >> 5;
    r = (Data & 0xF800) >> 11;

    Out->r = r << 3 | r >> 2;
    Out->g = g << 2 | g >> 3;
    Out->b = b << 3 | r >> 2;
};

void DecompressDXT1(DDS_IMAGE_DATA lImage, const GLubyte *lCompData, GLubyte *Data)
{
    GLint x, y, i, j, k;
    GLuint Select;
    const GLubyte *Temp;
    Color8888 colours[4], *col;
    GLushort color_0, color_1;
    GLuint bitmask, Offset;

    Temp = lCompData;
    colours[0].a = 0xFF;
    colours[1].a = 0xFF;
    colours[2].a = 0xFF;

    for (y = 0; y < lImage.height; y += 4)
    {
        for (x = 0; x < lImage.width; x += 4)
        {
            color_0 = *((const GLushort *)Temp);
            color_1 = *((const GLushort *)(Temp + 2));

            DxtcReadColor(color_0, colours);
            DxtcReadColor(color_1, colours + 1);
            bitmask = ((const GLuint *)Temp)[1];

            Temp += 8;

            if (color_0 > color_1)
            {
                // Four-color block: derive the other two colors.
                // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
                // These 2-bit codes correspond to the 2-bit fields
                // stored in the 64-bit block.
                colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
                colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
                colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;

                colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                colours[3].a = 0xFF;
            }
            else
            {
                // Three-color block: derive the other color.
                // 00 = color_0,  01 = color_1,  10 = color_2,
                // 11 = transparent.
                // These 2-bit codes correspond to the 2-bit fields
                // stored in the 64-bit block.
                colours[2].b = (colours[0].b + colours[1].b) / 2;
                colours[2].g = (colours[0].g + colours[1].g) / 2;
                colours[2].r = (colours[0].r + colours[1].r) / 2;

                colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                colours[3].a = 0x00;
            }

            for (j = 0, k = 0; j < 4; j++)
            {
                for (i = 0; i < 4; i++, k++)
                {
                    Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                    col = &colours[Select];

                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components;
                        Data[Offset + 0] = col->r;
                        Data[Offset + 1] = col->g;
                        Data[Offset + 2] = col->b;
                        Data[Offset + 3] = col->a;
                    }
                }
            }
        }
    }
}

void DecompressDXT3(DDS_IMAGE_DATA lImage, const GLubyte *lCompData, GLubyte *Data)
{
    const GLubyte *Temp = lCompData;

    Color8888 colours[4], *col;
    GLuint bitmask, Offset;
    const GLubyte *alpha;

    for (GLint y = 0; y < lImage.height; y += 4)
    {
        for (GLint x = 0; x < lImage.width; x += 4)
        {
            alpha = Temp;
            Temp += 8;
            DxtcReadColors(Temp, colours);
            bitmask = ((GLuint *)Temp)[1];

            Temp += 8;

            // Four-color block: derive the other two colors.
            // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
            // These 2-bit codes correspond to the 2-bit fields
            // stored in the 64-bit block.
            colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
            colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
            colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;

            colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
            colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
            colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;

            GLuint k = 0;
            for (GLint j = 0; j < 4; j++)
            {
                for (GLint i = 0; i < 4; i++, k++)
                {
                    GLuint Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                    col = &colours[Select];

                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components;
                        Data[Offset + 0] = col->r;
                        Data[Offset + 1] = col->g;
                        Data[Offset + 2] = col->b;
                    }
                }
            }

            for (GLint j = 0; j < 4; j++)
            {
                GLushort word = alpha[2 * j] + 256 * alpha[2 * j + 1];
                for (GLint i = 0; i < 4; i++)
                {
                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components + 3;
                        Data[Offset] = word & 0x0F;
                        Data[Offset] = Data[Offset] | (Data[Offset] << 4);
                    }
                    word >>= 4;
                }
            }
        }
    }
}

void DecompressDXT5(DDS_IMAGE_DATA lImage, const GLubyte *lCompData, GLubyte *Data)
{
    GLint x, y, z, i, j, k;
    GLuint Select;
    const GLubyte *Temp; //, r0, g0, b0, r1, g1, b1;
    Color8888 colours[4], *col;
    GLuint bitmask, Offset;
    GLubyte alphas[8];
    GLuint bits;

    Temp = lCompData;

    for (y = 0; y < lImage.height; y += 4)
    {
        for (x = 0; x < lImage.width; x += 4)
        {
            if (y >= lImage.height || x >= lImage.width)
                break;
            alphas[0] = Temp[0];
            alphas[1] = Temp[1];
            const GLubyte *alphamask = Temp + 2;
            Temp += 8;

            DxtcReadColors(Temp, colours);
            bitmask = ((const GLuint *)Temp)[1];

            Temp += 8;

            // Four-color block: derive the other two colors.
            // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
            // These 2-bit codes correspond to the 2-bit fields
            // stored in the 64-bit block.
            colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
            colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
            colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;

            colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
            colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
            colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;

            k = 0;
            for (j = 0; j < 4; j++)
            {
                for (i = 0; i < 4; i++, k++)
                {

                    Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                    col = &colours[Select];

                    // only put pixels out < width or height
                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components;
                        Data[Offset + 0] = col->r;
                        Data[Offset + 1] = col->g;
                        Data[Offset + 2] = col->b;
                    }
                }
            }

            // 8-alpha or 6-alpha block?
            if (alphas[0] > alphas[1])
            {
                // 8-alpha block:  derive the other six alphas.
                // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7; // bit code 010
                alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7; // bit code 011
                alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7; // bit code 100
                alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7; // bit code 101
                alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7; // bit code 110
                alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7; // bit code 111
            }
            else
            {
                // 6-alpha block.
                // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5; // Bit code 010
                alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5; // Bit code 011
                alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5; // Bit code 100
                alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5; // Bit code 101
                alphas[6] = 0x00;                                    // Bit code 110
                alphas[7] = 0xFF;                                    // Bit code 111
            }

            // Note: Have to separate the next two loops,
            //	it operates on a 6-byte system.

            // First three bytes
            // bits = *((ILint*)alphamask);
            bits = (alphamask[0]) | (alphamask[1] << 8) | (alphamask[2] << 16);
            for (j = 0; j < 2; j++)
            {
                for (i = 0; i < 4; i++)
                {
                    // only put pixels out < width or height
                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components + 3;
                        Data[Offset] = alphas[bits & 0x07];
                    }
                    bits >>= 3;
                }
            }

            // Last three bytes
            // bits = *((ILint*)&alphamask[3]);
            bits = (alphamask[3]) | (alphamask[4] << 8) | (alphamask[5] << 16);
            for (j = 2; j < 4; j++)
            {
                for (i = 0; i < 4; i++)
                {
                    // only put pixels out < width or height
                    if (((x + i) < lImage.width) && ((y + j) < lImage.height))
                    {
                        Offset = (y + j) * lImage.width * lImage.components +
                                 (x + i) * lImage.components + 3;
                        Data[Offset] = alphas[bits & 0x07];
                    }
                    bits >>= 3;
                }
            }
        }
    }
}

void DecompressDXT(DDS_IMAGE_DATA lImage, const GLubyte *lCompData, GLubyte *Data)
{
    switch (lImage.format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        DecompressDXT1(lImage, lCompData, Data);
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        DecompressDXT3(lImage, lCompData, Data);
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        DecompressDXT5(lImage, lCompData, Data);
        break;
    };
}
