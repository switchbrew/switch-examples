#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

#include <ft2build.h>
#include FT_FREETYPE_H

//See also libnx pl.h.

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

//This requires the switch-freetype package.
//Freetype code here is based on the example code from freetype docs.

static u32 framebuf_width=0;

static PadState pad;

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
void draw_text(FT_Face face, u32* framebuf, u32 x, u32 y, const char* str)
{
    u32 tmpx = x;
    FT_Error ret=0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount=0;

    for (i = 0; i < str_size; )
    {
        unitcount = decode_utf8 (&tmpchar, (const uint8_t*)&str[i]);
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

__attribute__((format(printf, 1, 2)))
static int error_screen(const char* fmt, ...)
{
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop())
    {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

static u64 getSystemLanguage(void)
{
    Result rc;
    u64 code = 0;

    rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = setGetSystemLanguage(&code);
        setExit();
    }

    return R_SUCCEEDED(rc) ? code : 0;
}

// LanguageCode is only needed with shared-font when using plGetSharedFont.
static u64 LanguageCode;

void userAppInit(void)
{
    Result rc;

    rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc))
        diagAbortWithResult(rc);

    LanguageCode = getSystemLanguage();
}

void userAppExit(void)
{
    plExit();
}

int main(int argc, char **argv)
{
    Result rc=0;
    FT_Error ret=0;

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    //Use this when using multiple shared-fonts.
    /*
    PlFontData fonts[PlSharedFontType_Total];
    size_t total_fonts=0;
    rc = plGetSharedFont(LanguageCode, fonts, PlSharedFontType_Total, &total_fonts);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFont() failed: 0x%x\n", rc);
    */

    // Use this when you want to use specific shared-font(s). Since this example only uses 1 font, only the font loaded by this will be used.
    PlFontData font;
    rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFontByType() failed: 0x%x\n", rc);

    FT_Library library;
    ret = FT_Init_FreeType(&library);
    if (ret)
        return error_screen("FT_Init_FreeType() failed: %d\n", ret);

    FT_Face face;
    ret = FT_New_Memory_Face( library,
                              font.address,    /* first byte in memory */
                              font.size,       /* size in bytes        */
                              0,               /* face_index           */
                              &face);
    if (ret) {
        FT_Done_FreeType(library);
        return error_screen("FT_New_Memory_Face() failed: %d\n", ret);
    }

    ret = FT_Set_Char_Size(
            face,    /* handle to face object           */
            0,       /* char_width in 1/64th of points  */
            24*64,   /* char_height in 1/64th of points */
            96,      /* horizontal device resolution    */
            96);     /* vertical device resolution      */
    if (ret) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return error_screen("FT_Set_Char_Size() failed: %d\n", ret);
    }

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        u32 stride;
        u32* framebuf = (u32*)framebufferBegin(&fb, &stride);
        framebuf_width = stride / sizeof(u32);

        memset(framebuf, 0, stride*FB_HEIGHT);
        draw_text(face, framebuf, 64, 64, u8"The quick brown fox jumps over the lazy dog. ファイル\ntest Test");

        framebufferEnd(&fb);
    }

    framebufferClose(&fb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return 0;
}
