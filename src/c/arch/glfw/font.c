#include "globals.h"
#include "gui.h"
#include "stb_truetype.h"
#include "stb_rect_pack.h"
#include "stb_ds.h"

extern const FileDescriptor font_table[];

static const float background_colour[] = {0.0f, 0.0f, 0.0f};
static const float dim_colour[] = {0.33f, 0.33f, 0.33f};
static const float normal_colour[] = {0.66f, 0.66f, 0.66f};
static const float bright_colour[] = {1.0f, 1.0f, 0.0f};

#define FONT_XPADDING 1
#define FONT_YPADDING 2
#define PAGE_WIDTH 256
#define PAGE_HEIGHT 256

struct font
{
    void* data;
    bool needsFreeing;
    stbtt_fontinfo info;
};

struct page
{
    uint8_t textureData[PAGE_WIDTH * PAGE_HEIGHT];
    stbtt_pack_context ctx;
    GLuint texture;
};

struct chardata
{
    uint32_t key;
    struct page* page;
    stbtt_packedchar packData;
};

int fontWidth;
int fontHeight;
static int fontSize;
static int fontAscent;
static int fontXOffset;
static float fontScale;
static struct font* fonts[8];
static struct page** pages = NULL;
static struct chardata* chardata = NULL;

static void freeFont(struct font* font)
{
    if (font->needsFreeing)
        free(font->data);
    free(font);
}

static struct font* loadFont(const char* filename)
{
    struct font* font = NULL;

    FILE* fp = fopen(filename, "rb");
    if (fp)
    {
        (void)fseek(fp, 0, SEEK_END);
        unsigned len = ftell(fp);
        (void)fseek(fp, 0, SEEK_SET);

        font = calloc(1, sizeof(struct font));
        font->data = malloc(len);
        font->needsFreeing = true;
        fread(font->data, 1, len, fp);
        fclose(fp);
    }
    else
    {
        const FileDescriptor* p = font_table;
        while (p->data)
        {
            if (strcmp(p->name, filename) == 0)
            {
                font = calloc(1, sizeof(struct font));
                font->data = (void*)p->data;
                font->needsFreeing = false;
                break;
            }

            p++;
        }

        if (!font)
            font = loadFont("font_regular");
    }

    stbtt_InitFont(&font->info, font->data, 0);
    return font;
}

void loadFonts()
{
    fontSize = get_ivar("font_size");
    fonts[REGULAR] = loadFont(get_svar("font_regular"));
    fonts[ITALIC] = loadFont(get_svar("font_italic"));
    fonts[BOLD] = loadFont(get_svar("font_bold"));
    fonts[BOLD | ITALIC] = loadFont(get_svar("font_bolditalic"));

    struct font* font = fonts[REGULAR];

    fontScale = stbtt_ScaleForPixelHeight(&font->info, fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &lineGap);
    fontAscent = ascent * fontScale;
    fontHeight = (ascent - descent + lineGap) * fontScale + FONT_XPADDING;

    int advance, bearing;
    stbtt_GetCodepointHMetrics(&font->info, 'M', &advance, &bearing);
    fontWidth = advance * fontScale + FONT_YPADDING;
    fontXOffset = bearing * fontScale;
}

void unloadFonts()
{
    for (int i=0; i<sizeof(fonts)/sizeof(*fonts); i++)
    {
        if (fonts[i])
        {
            freeFont(fonts[i]);
            fonts[i] = NULL;
        }
    }
}

static struct page* newPage()
{
    struct page* page = calloc(1, sizeof(struct page));
    glGenTextures(1, &page->texture);
    glBindTexture(GL_TEXTURE_2D, page->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbtt_PackBegin(&page->ctx,
        &page->textureData[0],
        PAGE_WIDTH,
        PAGE_HEIGHT,
        PAGE_WIDTH,
        1,
        NULL);

    return page;
}

static void freePage(struct page* page)
{
    stbtt_PackEnd(&page->ctx);
    glDeleteTextures(1, &page->texture);
    free(page);
}

static struct page* addPage()
{
    struct page* page = newPage();
    arrput(pages, page);
    return page;
}

void flushFontCache()
{
    while (arrlen(pages) > 0)
        freePage(arrpop(pages));
    hmfree(chardata);
}

static int rawRender(
    struct font* font, struct page* page, struct chardata* cd, uni_t c)
{
    stbtt_pack_range range;
    range.first_unicode_codepoint_in_range = c;
    range.array_of_unicode_codepoints = NULL;
    range.num_chars = 1;
    range.font_size = STBTT_POINT_SIZE(fontSize);
    range.chardata_for_range = &cd->packData;
    range.chardata_for_range->x0 = range.chardata_for_range->y0 =
        range.chardata_for_range->x1 = range.chardata_for_range->y1 = 0;

    stbrp_rect rect;

    int n = stbtt_PackFontRangesGatherRects(
        &page->ctx, &font->info, &range, 1, &rect);
    stbtt_PackFontRangesPackRects(&page->ctx, &rect, n);

    return stbtt_PackFontRangesRenderIntoRects(
        &page->ctx, &font->info, &range, 1, &rect);
}

static void renderTtfChar(uni_t c, uint8_t attrs, float x, float y)
{
    int style = REGULAR;
    if (attrs & DPY_BOLD)
        style |= BOLD;
    if (attrs & DPY_ITALIC)
        style |= ITALIC;

    uint32_t key = c | (style << 24);
    struct chardata* cd = hmgetp_null(chardata, key);
    if (!cd)
    {
        struct page* page;
        if (arrlen(pages) == 0)
            page = addPage();
        else
            page = pages[arrlen(pages) - 1];

        struct font* font = fonts[style];
        if (!font)
            return;

        struct chardata cds = {.key = key};
        hmputs(chardata, cds);
        cd = hmgetp_null(chardata, key);

        /* First try rendering into the current page. If that fails, the
         * page is full and we need a new one. */

        if (!rawRender(font, page, cd, c))
        {
            page = addPage();
            if (!rawRender(font, page, cd, c))
            {
                printf("Unrenderable codepoint %d\n", c);
                freePage(arrpop(pages));
                return;
            }
        }
        cd->page = page;

        /* Now we have a valid rendered glyph, but we need to update the
         * texture. */

        glBindTexture(GL_TEXTURE_2D, page->texture);
        glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_ALPHA,
            PAGE_WIDTH,
            PAGE_HEIGHT,
            0,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            &page->textureData[0]);
    }

    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(
        &cd->packData, PAGE_WIDTH, PAGE_HEIGHT, 0, &x, &y, &q, true);

    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, cd->page->texture);
    glBegin(GL_QUADS);
    glTexCoord2f(q.s0, q.t0);
    glVertex2f(q.x0, q.y0);
    glTexCoord2f(q.s1, q.t0);
    glVertex2f(q.x1, q.y0);
    glTexCoord2f(q.s1, q.t1);
    glVertex2f(q.x1, q.y1);
    glTexCoord2f(q.s0, q.t1);
    glVertex2f(q.x0, q.y1);
    glEnd();
}

void printChar(uni_t c, uint8_t attrs, float x, float y)
{
    /* Draw background. */

    glDisable(GL_BLEND);
    const float* colour;
    if (attrs & DPY_REVERSE)
    {
        if (attrs & DPY_BRIGHT)
            colour = bright_colour;
        else if (attrs & DPY_DIM)
            colour = dim_colour;
        else
            colour = normal_colour;
        glColor3fv(colour);

        glRectf(x, y, x + fontWidth, y + fontHeight);
    }

    /* Draw foreground. */

    if (attrs & DPY_REVERSE)
        colour = background_colour;
    else if (attrs & DPY_BRIGHT)
        colour = bright_colour;
    else if (attrs & DPY_DIM)
        colour = dim_colour;
    else
        colour = normal_colour;
    glColor3fv(colour);

    int w = fontWidth;
    int h = fontHeight;
    int w2 = fontWidth / 2;
    int h2 = fontHeight / 2;
    switch (c)
    {
        case 32:
        case 160: /* non-breaking space */
            break;

        case 0x2500: /* ─ */
        case 0x2501: /* ━ */
            glBegin(GL_LINES);
            glVertex2i(x + 0, y + h2);
            glVertex2i(x + w, y + h2);
            glEnd();
            break;

        case 0x2502: /* │ */
        case 0x2503: /* ┃ */
            glBegin(GL_LINES);
            glVertex2i(x + w2, y + 0);
            glVertex2i(x + w2, y + h);
            glEnd();
            break;

        case 0x250c: /* ┌ */
        case 0x250d: /* ┍ */
        case 0x250e: /* ┎ */
        case 0x250f: /* ┏ */
            glBegin(GL_LINES);
            glVertex2i(x + w2, y + h2);
            glVertex2i(x + w2, y + h);

            glVertex2i(x + w2, y + h2);
            glVertex2i(x + w, y + h2);
            glEnd();
            break;

        case 0x2510: /* ┐ */
        case 0x2511: /* ┑ */
        case 0x2512: /* ┒ */
        case 0x2513: /* ┓ */
            glBegin(GL_LINES);
            glVertex2i(x + w2, y + h2);
            glVertex2i(x + w2, y + h);

            glVertex2i(x + 0, y + h2);
            glVertex2i(x + w2, y + h2);
            glEnd();
            break;

        case 0x2514: /* └ */
        case 0x2515: /* ┕ */
        case 0x2516: /* ┖ */
        case 0x2517: /* ┗ */
            glBegin(GL_LINES);
            glVertex2i(x + w2, y + 0);
            glVertex2i(x + w2, y + h2);

            glVertex2i(x + w2, y + h2);
            glVertex2i(x + w, y + h2);
            glEnd();
            break;

        case 0x2518: /* ┘ */
        case 0x2519: /* ┙ */
        case 0x251a: /* ┚ */
        case 0x251b: /* ┛ */
            glBegin(GL_LINES);
            glVertex2i(x + w2, y + 0);
            glVertex2i(x + w2, y + h2);

            glVertex2i(x + 0, y + h2);
            glVertex2i(x + w2, y + h2);
            glEnd();
            break;

        case 0x2551: /* ║ */
            glBegin(GL_LINES);
            glVertex2i(x + w2 - 1, y);
            glVertex2i(x + w2 - 1, y + h);

            glVertex2i(x + w2 + 1, y);
            glVertex2i(x + w2 + 1, y + h);
            glEnd();
            break;

        case 0x2594: /* ▔ */
            glBegin(GL_LINES);
            glVertex2i(x, y + 2);
            glVertex2i(x + w, y + 2);
            glEnd();
            break;

        default:
            renderTtfChar(c, attrs, x + fontXOffset, y + fontAscent);
    }

    if (attrs & DPY_UNDERLINE)
    {
        glDisable(GL_BLEND);
        glBegin(GL_LINES);
        glVertex2i(x + fontXOffset, y + fontAscent + 1);
        glVertex2i(x + fontXOffset + fontWidth, y + fontAscent + 1);
        glEnd();
    }
}