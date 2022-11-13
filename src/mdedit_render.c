#include <stdio.h>
#include <string.h>

#include <md4c.h>

#include "gtkmdedit.h"
#include "gtkmdpicture.h"
#include "textbuffer_utils.h"

typedef struct {
    GtkWidget *textview;
    GtkTextBuffer *buffer;
    const char *img_prefix;
    MdBlock2 *block;
    GList *block_stack;
    GList *span_stack;
} MdContext;

static int
enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    static const char *head[6] = { "h1", "h2", "h3", "h4", "h5", "h6" };
    MdContext *ctx = (MdContext *) userdata;
    MdBlock2 *block = NULL;
    GtkTextIter start_iter;

    block = md_block2_new(ctx->buffer, type, NULL, NULL);
    gtk_text_buffer_get_end_iter (ctx->buffer, &start_iter);
    block->start = gtk_text_buffer_create_mark (ctx->buffer, NULL, &start_iter, TRUE);
    ctx->block = block;


    switch(type) {
        case MD_BLOCK_P:        break;
        case MD_BLOCK_H:        md_block_set_style(ctx->block, head[((MD_BLOCK_H_DETAIL*)detail)->level - 1]);
                                break;
        default:                break;
    }

    return 0;
}

static int
leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MdContext* ctx = (MdContext *) userdata;
    GtkTextIter end_iter;

    if (ctx->block == NULL) {
        return 0;
    }

    gtk_text_buffer_get_end_iter (ctx->buffer, &end_iter);
    ctx->block->end = gtk_text_buffer_create_mark (ctx->buffer, NULL, &end_iter, TRUE);

    text_buffer_append_all (ctx->buffer, "\n\n");

    ctx->block_stack = g_list_append(ctx->block_stack, ctx->block);
    ctx->block = NULL;

    return 0;
}

static int
enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata)
{
    GtkTextIter start_iter;
    MdContext *ctx = (MdContext*) userdata;

    MdSpan *span;

    span = md_span_new (ctx->buffer);
    gtk_text_buffer_get_end_iter (ctx->buffer, &start_iter);
    span->start = gtk_text_buffer_create_mark (ctx->buffer, NULL, &start_iter, TRUE);
    ctx->span_stack = g_list_append (ctx->span_stack, span);

    switch(type) {
        case MD_SPAN_EM: {
            md_span_set_style(span, "i");
            break;
        }
        case MD_SPAN_STRONG: {
            md_span_set_style(span, "b");
            break;
        }
        case MD_SPAN_IMG:       break;
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


    span_link = g_list_last(ctx->span_stack);
    span = (MdSpan *) span_link->data;
    gtk_text_buffer_get_end_iter (ctx->buffer, &end_iter);
    span->end = gtk_text_buffer_create_mark (ctx->buffer, NULL, &end_iter, TRUE);

    switch(type) {
        case MD_SPAN_IMG: {
            GtkTextIter start;
            GtkTextIter end;

            gtk_text_buffer_get_iter_at_mark(ctx->buffer, &start, span->start);
            gtk_text_buffer_get_iter_at_mark(ctx->buffer, &end, span->end);

            g_autofree char* alt_text = gtk_text_buffer_get_text (ctx->buffer, &start, &end, FALSE);
            gtk_text_buffer_delete (ctx->buffer, &start, &end);

            MD_SPAN_IMG_DETAIL *img = (MD_SPAN_IMG_DETAIL *) detail;
            g_autofree gchar *img_path = g_strndup (img->src.text, img->src.size);
            GtkWidget *picture = NULL;
            GtkTextChildAnchor *anchor = NULL;
            GtkTextIter text_iter;

            g_autofree char *full_image_path = g_build_filename (ctx->img_prefix, img_path, NULL);
            picture = gtk_md_picture_new(full_image_path, alt_text);
            gtk_widget_set_size_request(picture, 200, 200);
            gtk_text_buffer_get_end_iter (ctx->buffer, &text_iter);
            anchor = gtk_text_buffer_create_child_anchor(ctx->buffer, &text_iter);
            gtk_text_view_add_child_at_anchor (GTK_TEXT_VIEW(ctx->textview), picture, anchor);

            md_span_set_image(span, img_path);
        }
        default:                break;
    }

    ctx->span_stack = g_list_delete_link (ctx->span_stack, span_link);
    ctx->block->spans = g_list_append(ctx->block->spans, span);

    return 0;
}

static int
text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MdContext *ctx = (MdContext *) userdata;

    switch(type) {
        case MD_TEXT_BR:        text_buffer_append_all (ctx->buffer, "\n"); break;
        case MD_TEXT_SOFTBR:    text_buffer_append_all (ctx->buffer, "\n"); break;
        case MD_TEXT_ENTITY:    text_buffer_render_entity(ctx->buffer, text, size); break;
        default:                text_buffer_append_len(ctx->buffer, text, size); break;
    }

    return 0;
}

/* This function is necessary because while rendering, we want appends to the
 * textbuffer to not move existing markers at the end with them, while during
 * editing we want the end tags to have right gravity. */
void
make_end_marks_right_gravity(GList *blocks)
{
    for (GList *b_iter = blocks; b_iter != NULL; b_iter = b_iter->next) {
        MdBlock2 *block = (MdBlock2*) b_iter->data;
        GtkTextIter iter;

        gtk_text_buffer_get_iter_at_mark (block->buffer, &iter, block->end);
        gtk_text_buffer_delete_mark(block->buffer, block->end);
        block->end = gtk_text_buffer_create_mark(block->buffer, NULL, &iter, FALSE);

        for (GList *s_iter = block->spans; s_iter != NULL; s_iter = s_iter->next) {
            MdSpan *span = (MdSpan*) s_iter->data;
            GtkTextIter iter;

            gtk_text_buffer_get_iter_at_mark (span->buffer, &iter, span->end);
            gtk_text_buffer_delete_mark(span->buffer, span->end);
            span->end = gtk_text_buffer_create_mark(span->buffer, NULL, &iter, FALSE);
        }
    }
}

void
apply_styles(GtkTextBuffer *buffer, GList *blocks)
{
    for (GList *b_iter = blocks; b_iter != NULL; b_iter = b_iter->next) {
        MdBlock2 *block = (MdBlock2*) b_iter->data;

        switch(block->type) {
            //case MD_BLOCK_QUOTE:    text_buffer_append_all(ctx->buffer, "</blockquote>\n"); break;
            //case MD_BLOCK_UL:       text_buffer_append_all(ctx->buffer, "</ul>\n"); break;
            //case MD_BLOCK_OL:       text_buffer_append_all(ctx->buffer, "</ol>\n"); break;
            //case MD_BLOCK_LI:       text_buffer_append_all(ctx->buffer, "</li>\n"); break;
            //case MD_BLOCK_HR:       /*noop*/ break;
            //case MD_BLOCK_CODE:     text_buffer_append_all(ctx->buffer, "</code></pre>\n"); break;
            case MD_BLOCK_P:        break;
            case MD_BLOCK_H:        text_buffer_marks_apply_style (buffer,
                                        block->start, block->end,
                                        block->style);
                                    break;
            default:                break;
        }

        for (GList *s_iter = block->spans; s_iter != NULL; s_iter = s_iter->next) {
            MdSpan *span = (MdSpan*) s_iter->data;
            text_buffer_span_apply_style (buffer, span);
        }
    }
}

void
mdedit_render (GtkWidget *textview, const char* input,
    const char *img_prefix, GList **blocks)
{
    MdContext ctx;

    ctx.textview = textview;

    ctx.buffer = gtk_text_buffer_new (NULL);
    text_buffer_create_styles (ctx.buffer);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW (ctx.textview), ctx.buffer);

    ctx.img_prefix = img_prefix;

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

    make_end_marks_right_gravity(ctx.block_stack);

    apply_styles(ctx.buffer, ctx.block_stack);

    if (blocks != NULL) {
        *blocks = ctx.block_stack;
    } else {
        g_list_free_full(ctx.block->spans, (GDestroyNotify) md_span_free);
    }
}
