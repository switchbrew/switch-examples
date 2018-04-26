#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <switch.h>

#include <ft2build.h>
#include FT_FREETYPE_H

//See also libnx pl.h.

//This requires the switch-freetype package.
//Freetype code here is based on the example code from freetype docs.

static u32 framebuf_width=0;

//Note that this doesn't handle any blending.
void draw_glyph(FT_Bitmap* bitmap, u32* framebuf, u32 x, u32 y)
{
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8* imageptr = bitmap->buffer;

    if (bitmap->pixel_mode!=FT_PIXEL_MODE_GRAY) return;

    for (tmpy=0; tmpy<bitmap->rows; tmpy++)
    {
        for (tmpx=0; tmpx<bitmap->width; tmpx++)
        {
            framex = x + tmpx;
            framey = y + tmpy;

            framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(imageptr[tmpx], imageptr[tmpx], imageptr[tmpx]);
        }

        imageptr+= bitmap->pitch;
    }
}

//Note that this doesn't handle {tmpx > width}, etc.
//str is UTF-8.
void draw_text(FT_Face face, u32* framebuf, u32 x, u32 y, const uint8_t* str)
{
    u32 tmpx = x;
    FT_Error ret=0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen((const char*)str);
    uint32_t tmpchar;
    ssize_t unitcount=0;

    for (i = 0; i < str_size; )
    {
        unitcount = decode_utf8 (&tmpchar, &str[i]);
        if (unitcount <= 0) break;
        i+= unitcount;

        if (tmpchar == '\n')
        {
            tmpx = x;
            y+= face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret==0)
        {
            ret = FT_Render_Glyph( face->glyph,   /* glyph slot  */
                                   FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

int main(int argc, char **argv)
{
    Result rc=0;
    u32* framebuf;

    u64 LanguageCode=0;
    PlFontData font;
    PlFontData fonts[PlSharedFontType_Total];
    size_t total_fonts=0;
    FT_Error ret=0, libret=1, faceret=1;
    FT_Library library;
    FT_Face face;

    gfxInitDefault();
    consoleInit(NULL);

    rc = setInitialize();//Only needed with shared-font when using plGetSharedFont.
    if (R_SUCCEEDED(rc)) rc = setGetSystemLanguage(&LanguageCode);
    setExit();

    if (R_FAILED(rc)) printf("Failed to get system-language: 0x%x\n", rc);

    if (R_SUCCEEDED(rc))
    {
        rc = plInitialize();
        if (R_FAILED(rc)) printf("plInitialize() failed: 0x%x\n", rc);

        if (R_SUCCEEDED(rc))
        {
            //Use this when using multiple shared-fonts.
            rc = plGetSharedFont(LanguageCode, fonts, PlSharedFontType_Total, &total_fonts);
            if (R_FAILED(rc)) printf("plGetSharedFont() failed: 0x%x\n", rc);

            //Use this when you want to use specific shared-font(s). Since this example only uses 1 font, only the font loaded by this will be used.
            rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
            if (R_FAILED(rc)) printf("plGetSharedFontByType() failed: 0x%x\n", rc);

            if (R_SUCCEEDED(rc))
            {
                ret = FT_Init_FreeType(&library);
                libret = ret;
                if (ret) printf("FT_Init_FreeType() failed: %d\n", ret);

                if (ret==0)
                {
                    ret = FT_New_Memory_Face( library,
                                              font.address,    /* first byte in memory */
                                              font.size,       /* size in bytes        */
                                              0,               /* face_index           */
                                              &face);

                    faceret = ret;
                    if (ret) printf("FT_New_Memory_Face() failed: %d\n", ret);

                    if (ret==0)
                    {
                        ret = FT_Set_Char_Size(
                                face,    /* handle to face object           */
                                0,       /* char_width in 1/64th of points  */
                                8*64,    /* char_height in 1/64th of points */
                                300,     /* horizontal device resolution    */
                                300);    /* vertical device resolution      */

                        if (ret) printf("FT_Set_Char_Size() failed: %d\n", ret);
                    }
                }
            }
        }
    }

    if (R_SUCCEEDED(rc) && ret==0)
    {
        //Switch to using regular framebuffer.
        consoleClear();
        gfxSetMode(GfxMode_LinearDouble);
    }

    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        framebuf = (u32*) gfxGetFramebuffer(&framebuf_width, NULL);

        memset(framebuf, 0, gfxGetFramebufferSize());

        if (R_SUCCEEDED(rc) && ret==0) draw_text(face, framebuf, 64, 64, (const uint8_t*)"The quick brown fox jumps over the lazy dog. ファイル\ntest Test");

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    if (faceret==0) FT_Done_Face(face);
    if (libret==0) FT_Done_FreeType(library);

    plExit();
    gfxExit();
    return 0;
}
