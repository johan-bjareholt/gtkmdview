#pragma once

#include <gtk/gtk.h>
#include <md4c.h>

typedef struct _MdSpan {
    GtkTextBuffer *buffer;
    GtkTextMark *start;
    GtkTextMark *end;
    gchar *style;
    gchar *img_path;
} MdSpan;

MdSpan * md_span_new(GtkTextBuffer *buffer);
void md_span_set_style(MdSpan *span, gchar *style);
void md_span_set_image(MdSpan *span, gchar *path);
void md_span_free(MdSpan *span);

typedef struct _MdBlock2 {
    GtkTextBuffer *buffer;
    MD_BLOCKTYPE type;
    char *style;
    GtkTextMark *start;
    GtkTextMark *end;
    GList *spans;
} MdBlock2;

MdBlock2 * md_block2_new(GtkTextBuffer *buffer, MD_BLOCKTYPE type,
        GtkTextMark *start, GtkTextMark *end);
void md_block2_free(MdBlock2 *block);
void md_block_set_style(MdBlock2 *block, const char *style);

void text_buffer_append_len(GtkTextBuffer *buffer, const char *output,
        unsigned int size);

void text_buffer_append_all(GtkTextBuffer *buffer, const char *output);

void text_buffer_render_entity(GtkTextBuffer *buffer, const char* text,
        size_t size);

void text_buffer_create_styles(GtkTextBuffer *buffer);

void text_buffer_apply_style(GtkTextBuffer *buffer, gint start, gint end,
        const char *style);

void text_buffer_block_apply_style(GtkTextBuffer *buffer, MdBlock2 *block);
void text_buffer_span_apply_style(GtkTextBuffer *buffer, MdSpan *span);
void text_buffer_apply_style(GtkTextBuffer *buffer, gint start, gint end,
        const char *style);

void text_buffer_marks_apply_style(GtkTextBuffer *buffer,
        GtkTextMark *start, GtkTextMark *end, const char *style);
