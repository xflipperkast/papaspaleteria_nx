#define _GNU_SOURCE 1
#include "runtime.h"


/* dialog_backend */
#pragma push_macro("COL")
#pragma push_macro("C_BLUE")
#pragma push_macro("C_GROUPED")
#pragma push_macro("C_SECONDARY")
#pragma push_macro("C_WHITE")
#pragma push_macro("G")
#pragma push_macro("GLYPH_CACHE")
#pragma push_macro("UI_H")
#pragma push_macro("UI_MAX_FACES")
#pragma push_macro("UI_W")
#include <switch.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#define UI_W 1280
#define UI_H 720
#define COL(r,g,b,a) ((uint32_t)(r)|((uint32_t)(g)<<8)|((uint32_t)(b)<<16)|((uint32_t)(a)<<24))
#define C_WHITE COL(255,255,255,255)
#define C_SECONDARY COL(235,235,245,153)
#define C_BLUE COL(10,132,255,255)
#define C_GROUPED COL(44,44,46,255)
typedef struct Canvas {
    uint8_t *base;
    int stride;
    int w;
    int h;
} Canvas;

static uint64_t dialog_backend_monotonic_ms(void) {
    return apk64_clock_monotonic_ms();
}

static float dialog_backend_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float dialog_backend_ease_out(float t) { t = dialog_backend_clampf(t, 0.0f, 1.0f); float u = 1.0f - t; return 1.0f - u * u * u; }
static uint32_t dialog_backend_with_alpha(uint32_t col, unsigned a) { return (col & 0x00ffffffu) | ((((col >> 24) * a + 127u) / 255u) << 24); }

/* ---------------------------------------------------------------------------
 * Pixel primitives. Destinations are treated as opaque; sources carry alpha.
 * ------------------------------------------------------------------------ */

static inline uint32_t *dialog_backend_row_ptr(const Canvas *c, int y) { return (uint32_t *)(c->base + (size_t)y * c->stride); }
static inline unsigned dialog_backend_div255(unsigned v) { return ((v + 128u) * 257u) >> 16; }

static inline uint32_t dialog_backend_blend(uint32_t dst, uint32_t src, unsigned a) {
    if (!a) return dst;
    if (a >= 255u) return src | 0xff000000u;
    const unsigned ia = 255u - a;
    const unsigned r = dialog_backend_div255((src & 255u) * a + (dst & 255u) * ia);
    const unsigned g = dialog_backend_div255(((src >> 8) & 255u) * a + ((dst >> 8) & 255u) * ia);
    const unsigned b = dialog_backend_div255(((src >> 16) & 255u) * a + ((dst >> 16) & 255u) * ia);
    return r | (g << 8) | (b << 16) | 0xff000000u;
}

/* Premultiplied source over opaque destination. */
static inline uint32_t dialog_backend_over_premul(uint32_t dst, uint32_t src) {
    const unsigned sa = src >> 24;
    if (sa >= 255u) return src;
    if (!sa) return dst;
    const unsigned ia = 255u - sa;
    const unsigned r = (src & 255u) + dialog_backend_div255((dst & 255u) * ia);
    const unsigned g = ((src >> 8) & 255u) + dialog_backend_div255(((dst >> 8) & 255u) * ia);
    const unsigned b = ((src >> 16) & 255u) + dialog_backend_div255(((dst >> 16) & 255u) * ia);
    return (r > 255u ? 255u : r) | ((g > 255u ? 255u : g) << 8) | ((b > 255u ? 255u : b) << 16) | 0xff000000u;
}

static void dialog_backend_fill_rect(Canvas *c, int x, int y, int w, int h, uint32_t col) {
    if (!c || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > c->w ? c->w : x + w, y1 = y + h > c->h ? c->h : y + h;
    const unsigned a = col >> 24;
    for (int yy = y0; yy < y1; ++yy) {
        uint32_t *row = dialog_backend_row_ptr(c, yy);
        if (a >= 255u) for (int xx = x0; xx < x1; ++xx) row[xx] = col;
        else for (int xx = x0; xx < x1; ++xx) row[xx] = dialog_backend_blend(row[xx], col, a);
    }
}

/* Signed-distance rounded rectangle. feather=1 gives a crisp anti-aliased
 * edge; larger values produce soft shadows. Interior spans are filled without
 * per-pixel distance work. */
static void dialog_backend_fill_rrect_ex(Canvas *c, float x, float y, float w, float h, float r, uint32_t col, float feather) {
    if (!c || w <= 0.0f || h <= 0.0f || !(col >> 24)) return;
    if (feather < 1.0f) feather = 1.0f;
    const float hw = w * 0.5f, hh = h * 0.5f;
    r = dialog_backend_clampf(r, 0.0f, hw < hh ? hw : hh);
    const float cx = x + hw, cy = y + hh, ix = hw - r, iy = hh - r;
    const float ext = feather * 0.5f + 1.0f, inv_f = 1.0f / feather;
    int y0 = (int)floorf(y - ext), y1 = (int)ceilf(y + h + ext);
    int x0 = (int)floorf(x - ext), x1 = (int)ceilf(x + w + ext);
    if (y0 < 0) y0 = 0;
    if (x0 < 0) x0 = 0;
    if (y1 > c->h) y1 = c->h;
    if (x1 > c->w) x1 = c->w;
    const unsigned ca = col >> 24;
    for (int py = y0; py < y1; ++py) {
        uint32_t *row = dialog_backend_row_ptr(c, py);
        const float fy = fabsf((float)py + 0.5f - cy) - iy;
        int sx0 = x1, sx1 = x1;
        if (fy <= 0.0f && fy - r <= -feather * 0.5f) {
            const float lim = hw - feather * 0.5f;
            if (lim > 0.0f) {
                sx0 = (int)ceilf(cx - lim - 0.5f);
                sx1 = (int)floorf(cx + lim - 0.5f) + 1;
                if (sx0 < x0) sx0 = x0;
                if (sx1 > x1) sx1 = x1;
                if (sx1 < sx0) sx1 = sx0;
            }
        }
        for (int px = x0; px < x1; ++px) {
            if (px == sx0 && sx1 > sx0) {
                if (ca >= 255u) for (; px < sx1; ++px) row[px] = col;
                else for (; px < sx1; ++px) row[px] = dialog_backend_blend(row[px], col, ca);
                if (px >= x1) break;
            }
            const float fx = fabsf((float)px + 0.5f - cx) - ix;
            float d;
            if (fx > 0.0f && fy > 0.0f) d = sqrtf(fx * fx + fy * fy) - r;
            else d = (fx > fy ? fx : fy) - r;
            const float cov = 0.5f - d * inv_f;
            if (cov <= 0.0f) continue;
            row[px] = dialog_backend_blend(row[px], col, cov >= 1.0f ? ca : (unsigned)(cov * (float)ca + 0.5f));
        }
    }
}

static void dialog_backend_fill_rrect(Canvas *c, float x, float y, float w, float h, float r, uint32_t col) {
    dialog_backend_fill_rrect_ex(c, x, y, w, h, r, col, 1.0f);
}

static void dialog_backend_fill_circle(Canvas *c, float cx, float cy, float radius, uint32_t col) {
    dialog_backend_fill_rrect(c, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, radius, col);
}

static void dialog_backend_stroke_rrect(Canvas *c, float x, float y, float w, float h, float r, float t, uint32_t col) {
    if (!c || w <= 0.0f || h <= 0.0f) return;
    const float hw = w * 0.5f, hh = h * 0.5f;
    r = dialog_backend_clampf(r, 0.0f, hw < hh ? hw : hh);
    const float cx = x + hw, cy = y + hh, ix = hw - r, iy = hh - r;
    int y0 = (int)floorf(y - 1.0f), y1 = (int)ceilf(y + h + 1.0f);
    int x0 = (int)floorf(x - 1.0f), x1 = (int)ceilf(x + w + 1.0f);
    if (y0 < 0) y0 = 0;
    if (x0 < 0) x0 = 0;
    if (y1 > c->h) y1 = c->h;
    if (x1 > c->w) x1 = c->w;
    const unsigned ca = col >> 24;
    for (int py = y0; py < y1; ++py) {
        uint32_t *row = dialog_backend_row_ptr(c, py);
        const float fy = fabsf((float)py + 0.5f - cy) - iy;
        /* Skip the hollow middle of rows that are away from the corners. */
        int hole0 = x1, hole1 = x1;
        if (fy - r < -(t + 1.0f)) {
            const float lim = hw - t - 1.0f;
            if (lim > 0.0f) { hole0 = (int)ceilf(cx - lim); hole1 = (int)floorf(cx + lim); }
        }
        for (int px = x0; px < x1; ++px) {
            if (px >= hole0 && px < hole1) { px = hole1 - 1; continue; }
            const float fx = fabsf((float)px + 0.5f - cx) - ix;
            float d;
            if (fx > 0.0f && fy > 0.0f) d = sqrtf(fx * fx + fy * fy) - r;
            else d = (fx > fy ? fx : fy) - r;
            const float cov = dialog_backend_clampf(0.5f - d, 0.0f, 1.0f) - dialog_backend_clampf(0.5f - (d + t), 0.0f, 1.0f);
            if (cov <= 0.0f) continue;
            row[px] = dialog_backend_blend(row[px], col, (unsigned)(cov * (float)ca + 0.5f));
        }
    }
}

static void dialog_backend_draw_capsule(Canvas *c, float ax, float ay, float bx, float by, float radius, uint32_t col) {
    const float minx = (ax < bx ? ax : bx) - radius - 1.0f, maxx = (ax > bx ? ax : bx) + radius + 1.0f;
    const float miny = (ay < by ? ay : by) - radius - 1.0f, maxy = (ay > by ? ay : by) + radius + 1.0f;
    const float bax = bx - ax, bay = by - ay, bb = bax * bax + bay * bay;
    const unsigned ca = col >> 24;
    for (int py = (int)floorf(miny); py < (int)ceilf(maxy); ++py) {
        if (py < 0 || py >= c->h) continue;
        uint32_t *row = dialog_backend_row_ptr(c, py);
        for (int px = (int)floorf(minx); px < (int)ceilf(maxx); ++px) {
            if (px < 0 || px >= c->w) continue;
            const float pax = (float)px + 0.5f - ax, pay = (float)py + 0.5f - ay;
            const float hcl = bb > 0.0f ? dialog_backend_clampf((pax * bax + pay * bay) / bb, 0.0f, 1.0f) : 0.0f;
            const float dx = pax - bax * hcl, dy = pay - bay * hcl;
            const float cov = dialog_backend_clampf(0.5f - (sqrtf(dx * dx + dy * dy) - radius), 0.0f, 1.0f);
            if (cov > 0.0f) row[px] = dialog_backend_blend(row[px], col, (unsigned)(cov * (float)ca + 0.5f));
        }
    }
}

/* ---------------------------------------------------------------------------
 * Text: shared system font through FreeType, with a glyph cache. A tiny
 * built-in bitmap font remains as a fallback if the font service fails.
 * ------------------------------------------------------------------------ */

#define GLYPH_CACHE 1024u
#define UI_MAX_FACES 6

typedef struct dialog_backend_GlyphEntry {
    uint32_t cp;
    uint16_t size;
    uint8_t bold;
    uint8_t used;
    int16_t left, top;
    uint16_t w, h;
    int32_t advance; /* 26.6 */
    uint8_t *bmp;
} dialog_backend_GlyphEntry;

typedef struct dialog_backend_UiFont {
    bool ready;
    bool lib_ready;
    bool pl_ready;
    FT_Library lib;
    FT_Face face[UI_MAX_FACES];
    uint16_t face_size[UI_MAX_FACES];
    int nfaces;
    dialog_backend_GlyphEntry cache[GLYPH_CACHE];
} dialog_backend_UiFont;

static dialog_backend_UiFont dialog_backend_g_font;

static void dialog_backend_ui_font_init(void) {
    memset(&dialog_backend_g_font, 0, sizeof(dialog_backend_g_font));
    if (FT_Init_FreeType(&dialog_backend_g_font.lib) != 0) { APK64_LOGCW("LAUNCHER", "FreeType init failed"); return; }
    dialog_backend_g_font.lib_ready = true;
    Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) { APK64_LOGCW("LAUNCHER", "plInitialize failed rc=0x%x", rc); return; }
    dialog_backend_g_font.pl_ready = true;
    /* Standard first; the rest only supply glyphs Standard lacks (CJK/Hangul
     * app names). */
    static const PlSharedFontType types[] = {
        PlSharedFontType_Standard, PlSharedFontType_ChineseSimplified, PlSharedFontType_ExtChineseSimplified,
        PlSharedFontType_ChineseTraditional, PlSharedFontType_KO, PlSharedFontType_NintendoExt,
    };
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]) && dialog_backend_g_font.nfaces < UI_MAX_FACES; ++i) {
        PlFontData fd;
        memset(&fd, 0, sizeof(fd));
        if (R_FAILED(plGetSharedFontByType(&fd, types[i])) || !fd.address || !fd.size) continue;
        FT_Face face = NULL;
        if (FT_New_Memory_Face(dialog_backend_g_font.lib, (const FT_Byte *)fd.address, (FT_Long)fd.size, 0, &face) != 0) continue;
        dialog_backend_g_font.face[dialog_backend_g_font.nfaces++] = face;
    }
    dialog_backend_g_font.ready = dialog_backend_g_font.nfaces > 0;
    if (!dialog_backend_g_font.ready) APK64_LOGCW("LAUNCHER", "shared font unavailable; using fallback font");
}

static void dialog_backend_ui_font_exit(void) {
    for (unsigned i = 0; i < GLYPH_CACHE; ++i) free(dialog_backend_g_font.cache[i].bmp);
    for (int i = 0; i < dialog_backend_g_font.nfaces; ++i) FT_Done_Face(dialog_backend_g_font.face[i]);
    if (dialog_backend_g_font.lib_ready) FT_Done_FreeType(dialog_backend_g_font.lib);
    if (dialog_backend_g_font.pl_ready) plExit();
    memset(&dialog_backend_g_font, 0, sizeof(dialog_backend_g_font));
}

static void dialog_backend_glyph_render(dialog_backend_GlyphEntry *e, uint32_t cp, int size, bool bold) {
    free(e->bmp);
    memset(e, 0, sizeof(*e));
    e->used = 1; e->cp = cp; e->size = (uint16_t)size; e->bold = bold ? 1u : 0u;
    int fi = -1;
    FT_UInt gi = 0;
    for (int i = 0; i < dialog_backend_g_font.nfaces; ++i) {
        gi = FT_Get_Char_Index(dialog_backend_g_font.face[i], (FT_ULong)cp);
        if (gi) { fi = i; break; }
    }
    if (fi < 0) {
        fi = 0;
        gi = FT_Get_Char_Index(dialog_backend_g_font.face[0], 0xfffdu);
        if (!gi) gi = FT_Get_Char_Index(dialog_backend_g_font.face[0], '?');
    }
    FT_Face face = dialog_backend_g_font.face[fi];
    if (dialog_backend_g_font.face_size[fi] != (uint16_t)size) {
        if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)size) != 0) return;
        dialog_backend_g_font.face_size[fi] = (uint16_t)size;
    }
    if (FT_Load_Glyph(face, gi, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT) != 0) return;
    FT_GlyphSlot slot = face->glyph;
    FT_Pos extra = 0;
    if (bold && slot->format == FT_GLYPH_FORMAT_OUTLINE) {
        extra = (FT_Pos)size * 64 / 24;
        FT_Outline_Embolden(&slot->outline, extra);
    }
    if (FT_Render_Glyph(slot, FT_RENDER_MODE_LIGHT) != 0) return;
    const FT_Bitmap *bm = &slot->bitmap;
    e->advance = (int32_t)(slot->advance.x + extra);
    e->left = (int16_t)slot->bitmap_left;
    e->top = (int16_t)slot->bitmap_top;
    if (bm->width && bm->rows && bm->pixel_mode == FT_PIXEL_MODE_GRAY) {
        e->bmp = malloc((size_t)bm->width * bm->rows);
        if (!e->bmp) return;
        e->w = (uint16_t)bm->width; e->h = (uint16_t)bm->rows;
        for (unsigned y = 0; y < bm->rows; ++y) {
            const unsigned char *src = bm->pitch >= 0 ? bm->buffer + (size_t)y * (size_t)bm->pitch
                                                      : bm->buffer + (size_t)(bm->rows - 1u - y) * (size_t)(-bm->pitch);
            memcpy(e->bmp + (size_t)y * bm->width, src, bm->width);
        }
    }
}

static const dialog_backend_GlyphEntry *dialog_backend_glyph_get(uint32_t cp, int size, bool bold) {
    const uint32_t h = (cp * 2654435761u) ^ ((uint32_t)size * 40503u) ^ (bold ? 0x9e3779b9u : 0u);
    const unsigned start = h & (GLYPH_CACHE - 1u);
    for (unsigned probe = 0; probe < 16u; ++probe) {
        dialog_backend_GlyphEntry *e = &dialog_backend_g_font.cache[(start + probe) & (GLYPH_CACHE - 1u)];
        if (!e->used) { dialog_backend_glyph_render(e, cp, size, bold); return e; }
        if (e->cp == cp && e->size == (uint16_t)size && e->bold == (bold ? 1u : 0u)) return e;
    }
    dialog_backend_GlyphEntry *e = &dialog_backend_g_font.cache[start];
    dialog_backend_glyph_render(e, cp, size, bold);
    return e;
}

static size_t dialog_backend_utf8_next(const char *s, size_t remaining, uint32_t *out) {
    const uint8_t c0 = (uint8_t)s[0];
    if (c0 < 0x80u) { *out = c0; return 1; }
    if ((c0 & 0xe0u) == 0xc0u && remaining >= 2u && ((uint8_t)s[1] & 0xc0u) == 0x80u) {
        *out = ((uint32_t)(c0 & 0x1fu) << 6) | ((uint8_t)s[1] & 0x3fu); return 2;
    }
    if ((c0 & 0xf0u) == 0xe0u && remaining >= 3u && ((uint8_t)s[1] & 0xc0u) == 0x80u && ((uint8_t)s[2] & 0xc0u) == 0x80u) {
        *out = ((uint32_t)(c0 & 0x0fu) << 12) | ((uint32_t)((uint8_t)s[1] & 0x3fu) << 6) | ((uint8_t)s[2] & 0x3fu); return 3;
    }
    if ((c0 & 0xf8u) == 0xf0u && remaining >= 4u && ((uint8_t)s[1] & 0xc0u) == 0x80u &&
        ((uint8_t)s[2] & 0xc0u) == 0x80u && ((uint8_t)s[3] & 0xc0u) == 0x80u) {
        *out = ((uint32_t)(c0 & 0x07u) << 18) | ((uint32_t)((uint8_t)s[1] & 0x3fu) << 12) |
               ((uint32_t)((uint8_t)s[2] & 0x3fu) << 6) | ((uint8_t)s[3] & 0x3fu);
        return 4;
    }
    *out = 0xfffdu;
    return 1;
}

static void dialog_backend_fallback_glyph_rows(uint32_t cp, uint8_t out[7]) {
    memset(out, 0, 7);
    const char ch = cp < 128u ? (char)toupper((int)cp) : '?';
#define G(a,b,c,d,e,f,g) do{out[0]=a;out[1]=b;out[2]=c;out[3]=d;out[4]=e;out[5]=f;out[6]=g;}while(0)
    switch (ch) {
    case 'A':G(14,17,17,31,17,17,17);break; case 'B':G(30,17,17,30,17,17,30);break;
    case 'C':G(14,17,16,16,16,17,14);break; case 'D':G(30,17,17,17,17,17,30);break;
    case 'E':G(31,16,16,30,16,16,31);break; case 'F':G(31,16,16,30,16,16,16);break;
    case 'G':G(14,17,16,23,17,17,15);break; case 'H':G(17,17,17,31,17,17,17);break;
    case 'I':G(14,4,4,4,4,4,14);break; case 'J':G(7,2,2,2,18,18,12);break;
    case 'K':G(17,18,20,24,20,18,17);break; case 'L':G(16,16,16,16,16,16,31);break;
    case 'M':G(17,27,21,21,17,17,17);break; case 'N':G(17,25,21,19,17,17,17);break;
    case 'O':G(14,17,17,17,17,17,14);break; case 'P':G(30,17,17,30,16,16,16);break;
    case 'Q':G(14,17,17,17,21,18,13);break; case 'R':G(30,17,17,30,20,18,17);break;
    case 'S':G(15,16,16,14,1,1,30);break; case 'T':G(31,4,4,4,4,4,4);break;
    case 'U':G(17,17,17,17,17,17,14);break; case 'V':G(17,17,17,17,17,10,4);break;
    case 'W':G(17,17,17,21,21,21,10);break; case 'X':G(17,17,10,4,10,17,17);break;
    case 'Y':G(17,17,10,4,4,4,4);break; case 'Z':G(31,1,2,4,8,16,31);break;
    case '0':G(14,17,19,21,25,17,14);break; case '1':G(4,12,4,4,4,4,14);break;
    case '2':G(14,17,1,2,4,8,31);break; case '3':G(30,1,1,14,1,1,30);break;
    case '4':G(2,6,10,18,31,2,2);break; case '5':G(31,16,16,30,1,1,30);break;
    case '6':G(14,16,16,30,17,17,14);break; case '7':G(31,1,2,4,8,8,8);break;
    case '8':G(14,17,17,14,17,17,14);break; case '9':G(14,17,17,15,1,1,14);break;
    case '.':G(0,0,0,0,0,6,6);break; case ':':G(0,6,6,0,6,6,0);break;
    case '-':G(0,0,0,31,0,0,0);break; case '_':G(0,0,0,0,0,0,31);break;
    case '/':G(1,2,2,4,8,8,16);break; case '(':G(2,4,8,8,8,4,2);break;
    case ')':G(8,4,2,2,2,4,8);break; case '+':G(0,4,4,31,4,4,0);break;
    case '?':G(14,17,1,2,4,0,4);break; case '%':G(17,2,4,8,17,0,0);break;
    case '!':G(4,4,4,4,4,0,4);break; case ' ':break; default:G(0,14,14,14,14,14,0);break;
    }
#undef G
}

static int dialog_backend_fallback_scale(int size) { int s = size / 9; return s < 1 ? 1 : s; }

static int32_t dialog_backend_glyph_advance26(uint32_t cp, int size, bool bold) {
    if (!dialog_backend_g_font.ready) return (int32_t)dialog_backend_fallback_scale(size) * 6 * 64;
    return dialog_backend_glyph_get(cp, size, bold)->advance;
}

static int dialog_backend_ui_text_width(const char *s, int size, bool bold) {
    if (!s) return 0;
    int32_t pen = 0;
    const size_t n = strlen(s);
    for (size_t i = 0; i < n;) { uint32_t cp; i += dialog_backend_utf8_next(s + i, n - i, &cp); pen += dialog_backend_glyph_advance26(cp, size, bold); }
    return (int)((pen + 32) >> 6);
}

/* Copy src into dst, truncating with an ellipsis so it fits max_w pixels. */
static void dialog_backend_fit_text(char *dst, size_t cap, const char *src, int size, bool bold, int max_w) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    const size_t n = strlen(src);
    if (max_w <= 0 || dialog_backend_ui_text_width(src, size, bold) <= max_w) {
        size_t m = n < cap - 1u ? n : cap - 1u;
        memcpy(dst, src, m); dst[m] = 0;
        return;
    }
    static const char ell[] = "\xE2\x80\xA6";
    const int32_t limit = (int32_t)(max_w - dialog_backend_ui_text_width(ell, size, bold)) * 64;
    int32_t pen = 0;
    size_t cut = 0;
    for (size_t i = 0; i < n;) {
        uint32_t cp; const size_t l = dialog_backend_utf8_next(src + i, n - i, &cp);
        const int32_t adv = dialog_backend_glyph_advance26(cp, size, bold);
        if (pen + adv > limit) break;
        pen += adv; i += l; cut = i;
    }
    while (cut > 0 && src[cut - 1u] == ' ') --cut;
    if (cut + sizeof(ell) > cap) cut = cap > sizeof(ell) ? cap - sizeof(ell) : 0;
    memcpy(dst, src, cut);
    memcpy(dst + cut, ell, sizeof(ell));
}

/* Draw text with its baseline at y. Returns the drawn width. */
static int dialog_backend_ui_text(Canvas *c, int x, int baseline, const char *s, int size, bool bold, uint32_t col, int max_w) {
    if (!c || !s || !*s || !(col >> 24)) return 0;
    char fitted[512];
    dialog_backend_fit_text(fitted, sizeof(fitted), s, size, bold, max_w);
    const unsigned ca = col >> 24;
    const size_t n = strlen(fitted);
    int32_t pen = (int32_t)x * 64;
    for (size_t i = 0; i < n;) {
        uint32_t cp; i += dialog_backend_utf8_next(fitted + i, n - i, &cp);
        if (!dialog_backend_g_font.ready) {
            const int sc = dialog_backend_fallback_scale(size);
            uint8_t rows[7]; dialog_backend_fallback_glyph_rows(cp, rows);
            const int gx = (int)(pen >> 6), gy = baseline - 7 * sc;
            for (int ry = 0; ry < 7; ++ry) for (int rx = 0; rx < 5; ++rx)
                if (rows[ry] & (1u << (4 - rx))) dialog_backend_fill_rect(c, gx + rx * sc, gy + ry * sc, sc, sc, col);
            pen += sc * 6 * 64;
            continue;
        }
        const dialog_backend_GlyphEntry *g = dialog_backend_glyph_get(cp, size, bold);
        if (g->bmp) {
            const int gx = (int)((pen + 32) >> 6) + g->left, gy = baseline - g->top;
            for (int yy = 0; yy < g->h; ++yy) {
                const int dy = gy + yy;
                if (dy < 0 || dy >= c->h) continue;
                uint32_t *row = dialog_backend_row_ptr(c, dy);
                const uint8_t *src = g->bmp + (size_t)yy * g->w;
                for (int xx = 0; xx < g->w; ++xx) {
                    const int dx = gx + xx;
                    if (dx < 0 || dx >= c->w || !src[xx]) continue;
                    row[dx] = dialog_backend_blend(row[dx], col, dialog_backend_div255((unsigned)src[xx] * ca));
                }
            }
        }
        pen += g->advance;
    }
    return (int)((pen + 32) >> 6) - x;
}

static void dialog_backend_ui_text_center(Canvas *c, int cx, int baseline, const char *s, int size, bool bold, uint32_t col, int max_w) {
    char fitted[512];
    dialog_backend_fit_text(fitted, sizeof(fitted), s, size, bold, max_w);
    dialog_backend_ui_text(c, cx - dialog_backend_ui_text_width(fitted, size, bold) / 2, baseline, fitted, size, bold, col, 0);
}

static void dialog_backend_ui_text_right(Canvas *c, int rx, int baseline, const char *s, int size, bool bold, uint32_t col) {
    dialog_backend_ui_text(c, rx - dialog_backend_ui_text_width(s, size, bold), baseline, s, size, bold, col, 0);
}

/* Baseline that visually centres Latin text in a box. */
static int dialog_backend_baseline_in(int box_y, int box_h, int size) { return box_y + (box_h + (size * 7) / 10) / 2; }

/* dialog_sheet */
/* Uses the launcher's font/pixel backend, but owns no launcher state. */


static struct {
    uint8_t *pixels;
    bool open;
    const void *identity;
    int selected,scroll;
    bool touched,dragged;
    float touch_y;
    float touch_start_x,touch_start_y;
    int touch_item;
} dialog_backend_g_dialog_sheet;

void apk64_dialog_close(void) {
    if(dialog_backend_g_dialog_sheet.open) {
        apk64_graphics_release_software_surface();
        dialog_backend_ui_font_exit();
    }
    free(dialog_backend_g_dialog_sheet.pixels);
    memset(&dialog_backend_g_dialog_sheet,0,sizeof(dialog_backend_g_dialog_sheet));
}

/* Wrap every UTF-8 character, including long words, without dropping content.
 * Rendering uses a clipped canvas so scrolling cannot overwrite the heading. */
static int dialog_backend_dialog_sheet_text(Canvas *c,const char *s,int y,int scroll,uint32_t color) {
    if(!s)s="";
    const int size=22,line_h=30,max_w=1000;
    size_t n=strlen(s),at=0;
    int lines=0;
    do {
        char line[1024];size_t used=0;int width=0;
        while(at<n&&s[at]!='\n') {
            uint32_t cp;size_t bytes=dialog_backend_utf8_next(s+at,n-at,&cp);
            int advance=(dialog_backend_glyph_advance26(cp,size,false)+32)>>6;
            if(used&&(width+advance>max_w||used+bytes>=sizeof(line)))break;
            memcpy(line+used,s+at,bytes);used+=bytes;at+=bytes;width+=advance;
        }
        line[used]=0;
        if(at<n&&s[at]=='\n')++at;
        if(c)dialog_backend_ui_text(c,24,y+lines*line_h-scroll+size,line,size,false,color,0);
        ++lines;
    } while(at<n);
    return lines*line_h;
}

int apk64_dialog_present(const Apk64DialogSheet *sheet,const Apk64InputSnapshot *input) {
    if(!sheet||!input||sheet->count>256)return -3;
    if(dialog_backend_g_dialog_sheet.identity!=sheet->identity) {
        apk64_dialog_close();
        dialog_backend_g_dialog_sheet.identity=sheet->identity;
        dialog_backend_g_dialog_sheet.selected=-1;
        dialog_backend_g_dialog_sheet.touch_item=-1;
    }
    if(!dialog_backend_g_dialog_sheet.open) {
        dialog_backend_g_dialog_sheet.pixels=malloc(UI_W*UI_H*4);
        if(!dialog_backend_g_dialog_sheet.pixels)return -3;
        dialog_backend_g_dialog_sheet.open=true;dialog_backend_ui_font_init();
        APK64_LOGCI("ANDROID","Dialog presentation opened items=%zu",sheet->count);
    }
    int tops[256],heights[256],total=0;
    for(size_t i=0;i<sheet->count;++i) {
        if(sheet->items[i].text&&strlen(sheet->items[i].text)>1024u*1024u)return -3;
        tops[i]=total;
        heights[i]=dialog_backend_dialog_sheet_text(NULL,sheet->items[i].text,0,0,0)+(sheet->items[i].actionable?24:8);
        total+=heights[i]+12;
    }
    int selected=dialog_backend_g_dialog_sheet.selected;
    int direction=(input->buttons_down&APK64_BTN_DDOWN)?1:(input->buttons_down&APK64_BTN_DUP)?-1:0;
    if(direction) {
        int start=selected<0?(direction>0?-1:(int)sheet->count):selected;
        for(int i=start+direction;i>=0&&i<(int)sheet->count;i+=direction)
            if(sheet->items[i].actionable&&sheet->items[i].enabled){selected=i;break;}
        if(selected>=0) {
            if(tops[selected]<dialog_backend_g_dialog_sheet.scroll)dialog_backend_g_dialog_sheet.scroll=tops[selected];
            if(tops[selected]+heights[selected]>dialog_backend_g_dialog_sheet.scroll+500)
                dialog_backend_g_dialog_sheet.scroll=tops[selected]+heights[selected]-500;
        }
    }
    if(input->buttons_down&APK64_BTN_L)dialog_backend_g_dialog_sheet.scroll-=360;
    if(input->buttons_down&APK64_BTN_R)dialog_backend_g_dialog_sheet.scroll+=360;
    dialog_backend_g_dialog_sheet.selected=selected;
    int action=-1;
    if(input->buttons_down&APK64_BTN_B)action=-2;
    else if((input->buttons_down&APK64_BTN_A)&&selected>=0)action=selected;
    if(input->touch_count>0) {
        float x=input->touches[0].x,y=input->touches[0].y;
        Apk64DisplayPlan plan;
        if(apk64_graphics_presentation_plan(&plan)) {
            if(plan.aspect==APK64_DISPLAY_LANDSCAPE)plan.reverse=false;
            float lx=0,ly=0;
            if(apk64_display_map_touch(&plan,x,y,&lx,&ly)) {
                x=lx*UI_W/plan.logical_width;y=ly*UI_H/plan.logical_height;
            } else {x=-1;y=-1;}
        }
        if(!dialog_backend_g_dialog_sheet.touched) {
            dialog_backend_g_dialog_sheet.touched=true;dialog_backend_g_dialog_sheet.dragged=false;
            dialog_backend_g_dialog_sheet.touch_start_x=x;dialog_backend_g_dialog_sheet.touch_start_y=y;
            dialog_backend_g_dialog_sheet.touch_item=-1;
            if(x>=100&&x<1180&&y>=120&&y<620)
                for(size_t i=0;i<sheet->count;++i)
                    if(y-120+dialog_backend_g_dialog_sheet.scroll>=tops[i]&&y-120+dialog_backend_g_dialog_sheet.scroll<tops[i]+heights[i]&&
                       sheet->items[i].actionable&&sheet->items[i].enabled)dialog_backend_g_dialog_sheet.touch_item=(int)i;
        } else {
            int delta=(int)(dialog_backend_g_dialog_sheet.touch_y-y);
            if(hypotf(x-dialog_backend_g_dialog_sheet.touch_start_x,y-dialog_backend_g_dialog_sheet.touch_start_y)>8.0f)dialog_backend_g_dialog_sheet.dragged=true;
            if(dialog_backend_g_dialog_sheet.dragged)dialog_backend_g_dialog_sheet.scroll+=delta;
        }
        if(input->touch_count>1)dialog_backend_g_dialog_sheet.dragged=true;
        dialog_backend_g_dialog_sheet.touch_y=y;
    } else if(dialog_backend_g_dialog_sheet.touched) {
        if(!dialog_backend_g_dialog_sheet.dragged&&dialog_backend_g_dialog_sheet.touch_item>=0)action=dialog_backend_g_dialog_sheet.touch_item;
        dialog_backend_g_dialog_sheet.touched=false;
    }
    int max_scroll=total>500?total-500:0;
    if(dialog_backend_g_dialog_sheet.scroll<0)dialog_backend_g_dialog_sheet.scroll=0;
    if(dialog_backend_g_dialog_sheet.scroll>max_scroll)dialog_backend_g_dialog_sheet.scroll=max_scroll;
    uint32_t stride=UI_W*4;uint8_t *buf=dialog_backend_g_dialog_sheet.pixels;
    Canvas c={buf,(int)stride,UI_W,UI_H};
    dialog_backend_fill_rect(&c,0,0,UI_W,UI_H,COL(16,18,24,255));
    dialog_backend_fill_rrect(&c,80,44,1120,632,18,COL(36,38,44,255));
    dialog_backend_ui_text(&c,120,92,sheet->title?sheet->title:"",26,true,C_WHITE,1040);
    Canvas content={buf+(size_t)120*stride+(size_t)100*4,(int)stride,1080,500};
    for(size_t i=0;i<sheet->count;++i) {
        const Apk64DialogItem *item=&sheet->items[i];
        int y=tops[i]-dialog_backend_g_dialog_sheet.scroll;
        if(y+heights[i]<0||y>=content.h)continue;
        if(item->actionable) {
            dialog_backend_fill_rrect(&content,8,(float)y,1064,(float)heights[i],8,(int)i==selected?C_BLUE:C_GROUPED);
        }
        dialog_backend_dialog_sheet_text(&content,item->text,tops[i]+(item->actionable?12:0),dialog_backend_g_dialog_sheet.scroll,
                          item->enabled?C_WHITE:C_SECONDARY);
    }
    dialog_backend_ui_text(&c,120,653,"D-pad: choose   A: select   B: back   L/R: scroll",17,false,C_SECONDARY,1040);
    if(!apk64_graphics_present_rgba(buf,UI_W,UI_H,stride))return -3;
    return action;
}



#pragma pop_macro("UI_W")
#pragma pop_macro("UI_MAX_FACES")
#pragma pop_macro("UI_H")
#pragma pop_macro("GLYPH_CACHE")
#pragma pop_macro("G")
#pragma pop_macro("C_WHITE")
#pragma pop_macro("C_SECONDARY")
#pragma pop_macro("C_GROUPED")
#pragma pop_macro("C_BLUE")
#pragma pop_macro("COL")
