#include <stdio.h>
#include <string.h>

#include <md4c.h>

#include "md2gtktextbuf.h"
#include "entity.h"

typedef struct {
    GtkTextBuffer *buffer;
    const char *img_prefix;
} Context;

struct _MdTag {
    Context* ctx;
    unsigned tag_start;
    unsigned flags;
    int image_nesting_level;
    char escape_map[256];
};

typedef struct _MdTag MdTag;

static void
insert_text_internal (const char *output, unsigned int size, Context *ctx)
{
  GtkTextIter end;

  gtk_text_buffer_get_end_iter (ctx->buffer, &end);
  gtk_text_buffer_insert (ctx->buffer, &end, output, size);
}

static void
insert_text_len (MdTag* r, const MD_CHAR* text, MD_SIZE size)
{
    insert_text_internal (text, size, r->ctx);
}

static void
insert_text_all (MdTag* r, const MD_CHAR* text)
{
    insert_text_internal (text, strlen(text), r->ctx);
}

static void
tag_apply_style(MdTag *r, const char *style)
{
    GtkTextIter start_span;
    GtkTextIter end_span;
    gtk_text_buffer_get_start_iter (r->ctx->buffer, &start_span);
    gtk_text_iter_forward_chars (&start_span, r->tag_start);
    gtk_text_buffer_get_end_iter (r->ctx->buffer, &end_span);

    /*
    gchar *slice = gtk_text_iter_get_slice (&start_span, &end_span);
    unsigned tag_end = gtk_text_iter_get_offset (&end_span);
    g_print ("%s[%u -> %u]: %s\n", style, r->tag_start, tag_end, slice);
    g_free (slice);
    */

    gtk_text_buffer_apply_tag_by_name (r->ctx->buffer, style, &start_span, &end_span);
}

static unsigned
hex_val(char ch)
{
    if('0' <= ch && ch <= '9')
        return ch - '0';
    if('A' <= ch && ch <= 'Z')
        return ch - 'A' + 10;
    else
        return ch - 'a' + 10;
}

static void
render_utf8_codepoint(MdTag* r, unsigned codepoint,
                      void (*fn_append)(MdTag*, const MD_CHAR*, MD_SIZE))
{
    static const MD_CHAR utf8_replacement_char[] = { 0xef, 0xbf, 0xbd };

    unsigned char utf8[4];
    size_t n;

    if(codepoint <= 0x7f) {
        n = 1;
        utf8[0] = codepoint;
    } else if(codepoint <= 0x7ff) {
        n = 2;
        utf8[0] = 0xc0 | ((codepoint >>  6) & 0x1f);
        utf8[1] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else if(codepoint <= 0xffff) {
        n = 3;
        utf8[0] = 0xe0 | ((codepoint >> 12) & 0xf);
        utf8[1] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  0) & 0x3f);
    } else {
        n = 4;
        utf8[0] = 0xf0 | ((codepoint >> 18) & 0x7);
        utf8[1] = 0x80 + ((codepoint >> 12) & 0x3f);
        utf8[2] = 0x80 + ((codepoint >>  6) & 0x3f);
        utf8[3] = 0x80 + ((codepoint >>  0) & 0x3f);
    }

    if(0 < codepoint  &&  codepoint <= 0x10ffff)
        fn_append(r, (char*)utf8, n);
    else
        fn_append(r, utf8_replacement_char, 3);
}


/* Translate entity to its UTF-8 equivalent, or output the verbatim one
 * if such entity is unknown (or if the translation is disabled). */
static void
render_entity(MdTag* r, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MdTag*, const MD_CHAR*, MD_SIZE))
{
    /* We assume UTF-8 output is what is desired. */
    if(size > 3 && text[1] == '#') {
        unsigned codepoint = 0;

        if(text[2] == 'x' || text[2] == 'X') {
            /* Hexadecimal entity (e.g. "&#x1234abcd;")). */
            MD_SIZE i;
            for(i = 3; i < size-1; i++)
                codepoint = 16 * codepoint + hex_val(text[i]);
        } else {
            /* Decimal entity (e.g. "&1234;") */
            MD_SIZE i;
            for(i = 2; i < size-1; i++)
                codepoint = 10 * codepoint + (text[i] - '0');
        }

        render_utf8_codepoint(r, codepoint, fn_append);
        return;
    } else {
        /* Named entity (e.g. "&nbsp;"). */
        const struct entity* ent;

        ent = entity_lookup(text, size);
        if(ent != NULL) {
            render_utf8_codepoint(r, ent->codepoints[0], fn_append);
            if(ent->codepoints[1])
                render_utf8_codepoint(r, ent->codepoints[1], fn_append);
            return;
        }
    }

    fn_append(r, text, size);
}

/**************************************
 ***  HTML renderer implementation  ***
 **************************************/

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MdTag* r = (MdTag*) userdata;
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter (r->ctx->buffer, &iter);
    r->tag_start = gtk_text_iter_get_offset (&iter);

    switch(type) {
        //case MD_BLOCK_QUOTE:    insert_text_all(r, "<blockquote>\n"); break;
        //case MD_BLOCK_UL:       insert_text_all(r, "<ul>\n"); break;
        //case MD_BLOCK_OL:       render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail); break;
        //case MD_BLOCK_LI:       render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail); break;
        //case MD_BLOCK_HR:       insert_text_all(r, "<hr>\n"); break;
        //case MD_BLOCK_CODE:     render_open_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail); break;
        case MD_BLOCK_P:        insert_text_all(r, "\n"); break;
        default:                break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    static const char *head[6] = { "h1", "h2", "h3", "h4", "h5", "h6" };
    MdTag* r = (MdTag*) userdata;

    switch(type) {
        //case MD_BLOCK_QUOTE:    insert_text_all(r, "</blockquote>\n"); break;
        //case MD_BLOCK_UL:       insert_text_all(r, "</ul>\n"); break;
        //case MD_BLOCK_OL:       insert_text_all(r, "</ol>\n"); break;
        //case MD_BLOCK_LI:       insert_text_all(r, "</li>\n"); break;
        //case MD_BLOCK_HR:       /*noop*/ break;
        //case MD_BLOCK_CODE:     insert_text_all(r, "</code></pre>\n"); break;
        case MD_BLOCK_P:        insert_text_all(r, "\n\n"); break;
        case MD_BLOCK_H:        tag_apply_style (r,
                                    head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]);
                                    insert_text_all (r, "\n");
                                    break;
        default:                break;
    }

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MdTag* r = (MdTag*) userdata;

    GtkTextIter iter;
    gtk_text_buffer_get_end_iter (r->ctx->buffer, &iter);
    r->tag_start = gtk_text_iter_get_offset (&iter);

    switch(type) {
        //case MD_SPAN_STRONG:            insert_text_all(r, "<strong>"); break;
        //case MD_SPAN_U:                 insert_text_all(r, "<u>"); break;
        //case MD_SPAN_A:                 render_open_a_span(r, (MD_SPAN_A_DETAIL*) detail); break;
        //case MD_SPAN_CODE:              insert_text_all(r, "<code>"); break;
        //case MD_SPAN_DEL:               insert_text_all(r, "<del>"); break;
        case MD_SPAN_IMG:       r->image_nesting_level++; break;
        default:                break;
    }

    return 0;
}

static GdkTexture *
load_texture_from_file (gchar *path, GError **err)
{
  GdkPixbuf *pixbuf = NULL;
  GdkTexture *texture = NULL;

  pixbuf = gdk_pixbuf_new_from_file (path, err);
  if (err && *err != NULL) {
    goto error_load_file;
  }

  texture = gdk_texture_new_for_pixbuf (pixbuf);

error_load_file:
  return texture;
}

static void
render_image (MdTag *r, const MD_SPAN_IMG_DETAIL *detail)
{
    char *image_path = g_strndup (detail->src.text, detail->src.size);
    char *full_image_path = g_build_filename (r->ctx->img_prefix, image_path, NULL);
    GdkTexture *texture;
    GtkTextIter iter;
    GError *err = NULL;

    texture = load_texture_from_file (full_image_path, &err);
    if (err != NULL) {
        g_printerr ("Failed to load image: %s\n", err->message);
        return;
    }

    gtk_text_buffer_get_end_iter (r->ctx->buffer, &iter);
    gtk_text_buffer_insert_paintable (r->ctx->buffer, &iter, GDK_PAINTABLE (texture));
    g_free (full_image_path);
    g_free (image_path);
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MdTag* r = (MdTag*) userdata;

    switch(type) {
        case MD_SPAN_EM:        tag_apply_style (r, "i"); break;
        case MD_SPAN_STRONG:    tag_apply_style (r, "b"); break;
        //case MD_SPAN_U:                 insert_text_all(r, "</u>"); break;
        //case MD_SPAN_A:                 insert_text_all(r, "</a>"); break;
        //case MD_SPAN_DEL:               insert_text_all(r, "</del>"); break;
        //case MD_SPAN_CODE:              insert_text_all(r, "</code>"); break;
        case MD_SPAN_IMG:       render_image(r, (MD_SPAN_IMG_DETAIL*) detail);
                                r->image_nesting_level--; break;
        default:                break;
    }

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MdTag* r = (MdTag*) userdata;

    if (r->image_nesting_level > 0 ) {
        return 0;
    }

    switch(type) {
        case MD_TEXT_BR:        insert_text_all (r, "\n"); break;
        case MD_TEXT_SOFTBR:    insert_text_all (r, "\n"); break;
        case MD_TEXT_ENTITY:    render_entity(r, text, size, insert_text_len); break;
        default:                insert_text_len(r, text, size); break;
    }

    return 0;
}


void
create_styles (GtkTextBuffer *buffer)
{
  /* spacing */
  gtk_text_buffer_create_tag (buffer,
                            "t",
                            "scale", PANGO_SCALE_SMALL,
                            NULL);
  /* styles */
  gtk_text_buffer_create_tag (buffer,
                            "b",
                            "weight", PANGO_WEIGHT_BOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "i",
                            "weight", PANGO_WEIGHT_NORMAL,
                            "style", PANGO_STYLE_ITALIC,
                            NULL);
  /* headers */
  gtk_text_buffer_create_tag (buffer,
                            "h1",
                            "scale", PANGO_SCALE_XX_LARGE * PANGO_SCALE_LARGE,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "h2",
                            "scale", PANGO_SCALE_XX_LARGE,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "h3",
                            "scale", PANGO_SCALE_X_LARGE,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "h4",
                            "scale", PANGO_SCALE_LARGE,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "h5",
                            "scale", PANGO_SCALE_MEDIUM,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
  gtk_text_buffer_create_tag (buffer,
                            "h6",
                            "scale", PANGO_SCALE_MEDIUM,
                            "weight", PANGO_WEIGHT_SEMIBOLD,
                            NULL);
}

GtkTextBuffer *
md2textbuffer(const char* input, const char *img_prefix)
{
    Context ctx;
    ctx.buffer = gtk_text_buffer_new (NULL);
    ctx.img_prefix = img_prefix;

    MdTag render = { &ctx, 0, 0, 0, { 0 } };

    MD_PARSER parser = {
        0,
        MD_FLAG_NOHTML,
        enter_block_callback,
        leave_block_callback,
        enter_span_callback,
        leave_span_callback,
        text_callback,
        NULL,
        NULL
    };

    /* prepare text view */
    create_styles (ctx.buffer);

    md_parse(input, strlen (input), &parser, (void*) &render);

    return ctx.buffer;
}
