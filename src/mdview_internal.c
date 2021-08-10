#include <stdio.h>
#include <string.h>

#include <md4c.h>

#include "md2gtktextbuf.h"
#include "entity.h"

typedef struct _MdSpan {
    unsigned start;
    unsigned end;
} MdSpan;

typedef struct _MdBlock {
    unsigned start;
    unsigned end;
    GList *spans;
    GList *images;
} MdBlock;

typedef struct {
    GtkBox *box;
    GtkTextBuffer *buffer;
    const char *img_prefix;
    MdBlock *block;
} MdContext;

static void
insert_text_internal (const char *output, unsigned int size, MdContext *ctx)
{
  GtkTextIter end;

  gtk_text_buffer_get_end_iter (ctx->buffer, &end);
  gtk_text_buffer_insert (ctx->buffer, &end, output, size);
}

static void
insert_text_len (MdContext* ctx, const MD_CHAR* text, MD_SIZE size)
{
    insert_text_internal (text, size, ctx);
}

static void
insert_text_all (MdContext* ctx, const MD_CHAR* text)
{
    insert_text_internal (text, strlen(text), ctx);
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

static void
span_apply_style(MdContext *ctx, gint start, gint end, const char *style)
{
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_start_iter (ctx->buffer, &start_iter);
    gtk_text_buffer_get_start_iter (ctx->buffer, &end_iter);
    gtk_text_iter_forward_chars (&start_iter, start);
    gtk_text_iter_forward_chars (&end_iter, end);

    /*
    gchar *slice = gtk_text_iter_get_slice (&start_span, &end_span);
    unsigned block_end = gtk_text_iter_get_offset (&end_span);
    g_print ("%s[%u -> %u]: %s\n", style, r->block_start, block_end, slice);
    g_free (slice);
    */

    gtk_text_buffer_apply_tag_by_name (ctx->buffer, style, &start_iter, &end_iter);
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
render_utf8_codepoint(MdContext *ctx, unsigned codepoint,
                      void (*fn_append)(MdContext *, const MD_CHAR*, MD_SIZE))
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
        fn_append(ctx, (char*)utf8, n);
    else
        fn_append(ctx, utf8_replacement_char, 3);
}


/* Translate entity to its UTF-8 equivalent, or output the verbatim one
 * if such entity is unknown (or if the translation is disabled). */
static void
render_entity(MdContext *ctx, const MD_CHAR* text, MD_SIZE size,
              void (*fn_append)(MdContext *, const MD_CHAR*, MD_SIZE))
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

        render_utf8_codepoint(ctx, codepoint, fn_append);
        return;
    } else {
        /* Named entity (e.g. "&nbsp;"). */
        const struct entity* ent;

        ent = entity_lookup(text, size);
        if(ent != NULL) {
            render_utf8_codepoint(ctx, ent->codepoints[0], fn_append);
            if(ent->codepoints[1])
                render_utf8_codepoint(ctx, ent->codepoints[1], fn_append);
            return;
        }
    }

    fn_append(ctx, text, size);
}

static GtkWidget *
render_image (MdContext *ctx, gchar *image_path, gchar *alt_text)
{
    char *full_image_path = g_build_filename (ctx->img_prefix, image_path, NULL);

    GtkWidget *picture = NULL;

    picture = gtk_picture_new_for_filename (full_image_path);
    g_object_set (picture, "keep-aspect-ratio", TRUE, NULL);

    g_free (full_image_path);

    return picture;
}

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MdContext *ctx = (MdContext *) userdata;
    GtkTextIter iter;

    gtk_text_buffer_get_end_iter (ctx->buffer, &iter);
    ctx->block->start = gtk_text_iter_get_offset (&iter);

    switch(type) {
        //case MD_BLOCK_QUOTE:    insert_text_all(r, "<blockquote>\n"); break;
        //case MD_BLOCK_UL:       insert_text_all(r, "<ul>\n"); break;
        //case MD_BLOCK_OL:       render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail); break;
        //case MD_BLOCK_LI:       render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail); break;
        //case MD_BLOCK_HR:       insert_text_all(r, "<hr>\n"); break;
        //case MD_BLOCK_CODE:     render_open_code_block(r, (const MD_BLOCK_CODE_DETAIL*) detail); break;
        case MD_BLOCK_P:        break;
        case MD_BLOCK_H:        break;
        default:                break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    static const char *head[6] = { "h1", "h2", "h3", "h4", "h5", "h6" };
    MdContext* ctx = (MdContext *) userdata;
    GtkWidget *text = NULL;
    GtkTextIter iter;

    gtk_text_buffer_get_end_iter (ctx->buffer, &iter);
    ctx->block->end = gtk_text_iter_get_offset (&iter);

    switch(type) {
        //case MD_BLOCK_QUOTE:    insert_text_all(r, "</blockquote>\n"); break;
        //case MD_BLOCK_UL:       insert_text_all(r, "</ul>\n"); break;
        //case MD_BLOCK_OL:       insert_text_all(r, "</ol>\n"); break;
        //case MD_BLOCK_LI:       insert_text_all(r, "</li>\n"); break;
        //case MD_BLOCK_HR:       /*noop*/ break;
        //case MD_BLOCK_CODE:     insert_text_all(r, "</code></pre>\n"); break;
        case MD_BLOCK_P:        break;
        case MD_BLOCK_H:        span_apply_style (ctx,
                                    ctx->block->start, ctx->block->end,
                                    head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]);
                                    break;
        default:                break;
    }

    /* if the block ends with an image or is the last block in the document,
     * create the TextView */
    if (ctx->block->images != NULL || type == MD_BLOCK_DOC) {
        text = gtk_text_view_new_with_buffer (ctx->buffer);
        g_object_set (text, "editable", FALSE, NULL);
        g_object_set (text, "wrap-mode", GTK_WRAP_WORD, NULL);
        g_object_set (text, "hexpand", TRUE, NULL);
        g_object_set (text, "vexpand", TRUE, NULL);

        gtk_box_append (GTK_BOX (ctx->box), text);
        ctx->buffer = NULL;
    } else {
        /* Each text block should have a newline as spacing to the next
         * text block */
        if (type != MD_BLOCK_DOC) {
            insert_text_all (ctx, "\n\n");
        }
    }

    if (ctx->buffer == NULL && type != MD_BLOCK_DOC) {
        ctx->buffer = gtk_text_buffer_new (NULL);
        create_styles (ctx->buffer);
    }

    if (ctx->block->images != NULL) {
        GtkWidget *flowbox = gtk_flow_box_new ();
        gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (flowbox), GTK_SELECTION_NONE);
        for (GList *iter = ctx->block->images; iter != NULL; iter = iter->next) {
            gchar *path = NULL;
            GtkWidget *picture = NULL;

            path = iter->data;
            picture = render_image(ctx, path, "");
            gtk_flow_box_insert (GTK_FLOW_BOX (flowbox), picture, -1);
        }
        gtk_box_append (ctx->box, flowbox);
        g_list_free (ctx->block->images);
        ctx->block->images = NULL;
    }

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    GtkTextIter iter;
    MdContext *ctx = (MdContext*) userdata;

    MdSpan *span;

    span = g_malloc0 (sizeof (MdSpan));
    gtk_text_buffer_get_end_iter (ctx->buffer, &iter);
    span->start = gtk_text_iter_get_offset (&iter);
    ctx->block->spans = g_list_append (ctx->block->spans, span);

    switch(type) {
        //case MD_SPAN_STRONG:            insert_text_all(r, "<strong>"); break;
        //case MD_SPAN_U:                 insert_text_all(r, "<u>"); break;
        //case MD_SPAN_A:                 render_open_a_span(r, (MD_SPAN_A_DETAIL*) detail); break;
        //case MD_SPAN_CODE:              insert_text_all(r, "<code>"); break;
        //case MD_SPAN_DEL:               insert_text_all(r, "<del>"); break;
        case MD_SPAN_IMG: {
            MD_SPAN_IMG_DETAIL *img = (MD_SPAN_IMG_DETAIL *) detail;
            gchar *img_path = g_strndup (img->src.text, img->src.size);
            ctx->block->images = g_list_append(ctx->block->images, img_path);
            break;
        }
        default:                break;
    }

    return 0;
}

static int
leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    MdContext * ctx = (MdContext *) userdata;
    GList *span_link = NULL;
    MdSpan *span = NULL;
    GtkTextIter end_iter;


    span_link = g_list_last(ctx->block->spans);
    span = (MdSpan *) span_link->data;
    gtk_text_buffer_get_end_iter (ctx->buffer, &end_iter);
    span->end = gtk_text_iter_get_offset (&end_iter);

    switch(type) {
        case MD_SPAN_EM: {
            span_apply_style (ctx, span->start, span->end, "i");
            break;
        }
        case MD_SPAN_STRONG: {
            span_apply_style (ctx, span->start, span->end, "b");
            break;
        }
        //case MD_SPAN_U:                 insert_text_all(r, "</u>"); break;
        //case MD_SPAN_A:                 insert_text_all(r, "</a>"); break;
        //case MD_SPAN_DEL:               insert_text_all(r, "</del>"); break;
        //case MD_SPAN_CODE:              insert_text_all(r, "</code>"); break;
        case MD_SPAN_IMG:       break;
        default:                break;
    }

    ctx->block->spans = g_list_delete_link (ctx->block->spans, span_link);
    g_free (span);

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MdContext *ctx = (MdContext *) userdata;

    if (ctx->block->images != NULL) {
        return 0;
    }

    switch(type) {
        case MD_TEXT_BR:        insert_text_all (ctx, "\n"); break;
        case MD_TEXT_SOFTBR:    insert_text_all (ctx, "\n"); break;
        case MD_TEXT_ENTITY:    render_entity(ctx, text, size, insert_text_len); break;
        default:                insert_text_len(ctx, text, size); break;
    }

    return 0;
}


GtkWidget *
mdview_internal_new (const char* input, const char *img_prefix)
{
    MdContext ctx;

    ctx.box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

    ctx.img_prefix = img_prefix;

    MdBlock block = { 0, 0, NULL, NULL };
    ctx.block = &block;
    ctx.buffer = gtk_text_buffer_new (NULL);
    create_styles (ctx.buffer);

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

    md_parse(input, strlen (input), &parser, (void*) &ctx);

    return GTK_WIDGET (ctx.box);
}
