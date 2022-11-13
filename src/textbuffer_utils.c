#include "textbuffer_utils.h"
#include "entity.h"

MdSpan *
md_span_new(GtkTextBuffer *buffer)
{
    MdSpan *span = (MdSpan *) g_malloc0 (sizeof (MdSpan));
    span->buffer = buffer;
    span->start = NULL;
    span->end = NULL;
    span->style = NULL;
    span->img_path = NULL;
    return span;
}

void
md_span_set_style(MdSpan *span, gchar *style)
{
    g_free (span->style);
    span->style = g_strdup (style);
}

void
md_span_set_image(MdSpan *span, gchar *path)
{
    g_free(span->img_path);
    span->img_path = g_strdup(path);
}

void
md_span_free(MdSpan *span)
{
    gtk_text_buffer_delete_mark(span->buffer, span->start);
    gtk_text_buffer_delete_mark(span->buffer, span->end);
    g_free(span->style);
    g_free(span->img_path);
    g_free(span);
}

MdBlock2 *
md_block2_new(GtkTextBuffer *buffer, MD_BLOCKTYPE type, GtkTextMark *start,
        GtkTextMark *end)
{
    MdBlock2* block = g_malloc0(sizeof(MdBlock2));
    block->buffer = buffer;
    block->type = type;
    block->style = NULL;
    block->start = start;
    block->end = end;
    block->spans = NULL;
    return block;
}

void
md_block2_free(MdBlock2 *block)
{
    g_free (block);
}

void
md_block_set_style(MdBlock2 *block, const char *style)
{
    g_free (block->style);
    block->style = g_strdup (style);
}

void
text_buffer_append_len(GtkTextBuffer *buffer, const char *output, unsigned int size)
{
  GtkTextIter end;

  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_insert (buffer, &end, output, size);
}

void
text_buffer_append_all(GtkTextBuffer *buffer, const char *output)
{
    text_buffer_append_len(buffer, output, strlen(output));
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
render_utf8_codepoint(GtkTextBuffer *buffer, unsigned codepoint)
{
    static const char utf8_replacement_char[] = { 0xef, 0xbf, 0xbd };

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
        text_buffer_append_len(buffer, (char*)utf8, n);
    else
        text_buffer_append_len(buffer, utf8_replacement_char, 3);
}

/* Translate entity to its UTF-8 equivalent, or output the verbatim one
 * if such entity is unknown (or if the translation is disabled). */
void
text_buffer_render_entity(GtkTextBuffer *buffer, const char* text, size_t size)
{
    /* We assume UTF-8 output is what is desired. */
    if(size > 3 && text[1] == '#') {
        unsigned codepoint = 0;

        if(text[2] == 'x' || text[2] == 'X') {
            /* Hexadecimal entity (e.g. "&#x1234abcd;")). */
            size_t i;
            for(i = 3; i < size-1; i++)
                codepoint = 16 * codepoint + hex_val(text[i]);
        } else {
            /* Decimal entity (e.g. "&1234;") */
            size_t i;
            for(i = 2; i < size-1; i++)
                codepoint = 10 * codepoint + (text[i] - '0');
        }

        render_utf8_codepoint(buffer, codepoint);
        return;
    } else {
        /* Named entity (e.g. "&nbsp;"). */
        const struct entity* ent;

        ent = entity_lookup(text, size);
        if(ent != NULL) {
            render_utf8_codepoint(buffer, ent->codepoints[0]);
            if(ent->codepoints[1])
                render_utf8_codepoint(buffer, ent->codepoints[1]);
            return;
        }
    }

    text_buffer_append_len(buffer, text, size);
}

void
text_buffer_create_styles (GtkTextBuffer *buffer)
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

void
text_buffer_block_apply_style(GtkTextBuffer *buffer, MdBlock2 *block)
{
    if (block->style != NULL) {
        GtkTextIter start_iter;
        GtkTextIter end_iter;
        gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, block->start);
        gtk_text_buffer_get_iter_at_mark(buffer, &end_iter, block->end);

        gtk_text_buffer_apply_tag_by_name (buffer, block->style, &start_iter, &end_iter);
    }
}

void
text_buffer_span_apply_style(GtkTextBuffer *buffer, MdSpan *span)
{
    if (span->style != NULL) {
        GtkTextIter start_iter;
        GtkTextIter end_iter;
        gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, span->start);
        gtk_text_buffer_get_iter_at_mark(buffer, &end_iter, span->end);

        gtk_text_buffer_apply_tag_by_name (buffer, span->style, &start_iter, &end_iter);
    }
}

void
text_buffer_apply_style(GtkTextBuffer *buffer, gint start, gint end,
        const char *style)
{
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_start_iter (buffer, &start_iter);
    gtk_text_buffer_get_start_iter (buffer, &end_iter);
    gtk_text_iter_forward_chars (&start_iter, start);
    gtk_text_iter_forward_chars (&end_iter, end);

    gtk_text_buffer_apply_tag_by_name (buffer, style, &start_iter, &end_iter);
}


void
text_buffer_marks_apply_style(GtkTextBuffer *buffer, GtkTextMark *start,
        GtkTextMark *end, const char *style)
{
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, start);
    gtk_text_buffer_get_iter_at_mark(buffer, &end_iter, end);

    gtk_text_buffer_apply_tag_by_name (buffer, style, &start_iter, &end_iter);
}
