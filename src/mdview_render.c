#include <stdio.h>
#include <string.h>

#include <md4c.h>

#include "gtkmdview.h"
#include "mdview_render.h"
#include "textbuffer_utils.h"

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
        //case MD_BLOCK_QUOTE:    text_buffer_append_all(r->buffer, "<blockquote>\n"); break;
        //case MD_BLOCK_UL:       text_buffer_append_all(r->buffer, "<ul>\n"); break;
        //case MD_BLOCK_OL:       render_open_ol_block(r, (const MD_BLOCK_OL_DETAIL*)detail); break;
        //case MD_BLOCK_LI:       render_open_li_block(r, (const MD_BLOCK_LI_DETAIL*)detail); break;
        //case MD_BLOCK_HR:       text_buffer_append_all(r->buffer, "<hr>\n"); break;
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
        //case MD_BLOCK_QUOTE:    text_buffer_append_all(r->buffer, "</blockquote>\n"); break;
        //case MD_BLOCK_UL:       text_buffer_append_all(r->buffer, "</ul>\n"); break;
        //case MD_BLOCK_OL:       text_buffer_append_all(r->buffer, "</ol>\n"); break;
        //case MD_BLOCK_LI:       text_buffer_append_all(r->buffer, "</li>\n"); break;
        //case MD_BLOCK_HR:       /*noop*/ break;
        //case MD_BLOCK_CODE:     text_buffer_append_all(r->buffer, "</code></pre>\n"); break;
        case MD_BLOCK_P:        break;
        case MD_BLOCK_H:        text_buffer_apply_style (ctx->buffer,
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

        gtk_box_append (GTK_BOX (ctx->box), text);
        ctx->buffer = NULL;
    } else {
        /* Each text block should have a newline as spacing to the next
         * text block */
        if (type != MD_BLOCK_DOC) {
            text_buffer_append_all (ctx->buffer, "\n\n");
        }
    }

    if (ctx->buffer == NULL && type != MD_BLOCK_DOC) {
        ctx->buffer = gtk_text_buffer_new (NULL);
        text_buffer_create_styles (ctx->buffer);
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
        g_list_free_full (ctx->block->images, g_free);
        ctx->block->images = NULL;
    }

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

    ctx->block->spans = g_list_append (ctx->block->spans, span);

    switch(type) {
        case MD_SPAN_EM: {
            md_span_set_style(span, "i");
            break;
        }
        case MD_SPAN_STRONG: {
            md_span_set_style(span, "b");
            break;
        }
        //case MD_SPAN_U:                 text_buffer_append_all(r->buffer, "<u>"); break;
        //case MD_SPAN_A:                 render_open_a_span(r, (MD_SPAN_A_DETAIL*) detail); break;
        //case MD_SPAN_CODE:              text_buffer_append_all(r->buffer, "<code>"); break;
        //case MD_SPAN_DEL:               text_buffer_append_all(r->buffer, "<del>"); break;
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
    span->end = gtk_text_buffer_create_mark (ctx->buffer, NULL, &end_iter, TRUE);

    switch(type) {
        case MD_SPAN_EM: {
            text_buffer_span_apply_style (ctx->buffer, span);
            break;
        }
        case MD_SPAN_STRONG: {
            text_buffer_span_apply_style (ctx->buffer, span);
            break;
        }
        //case MD_SPAN_U:                 text_buffer_append_all(r->buffer, "</u>"); break;
        //case MD_SPAN_A:                 text_buffer_append_all(r->buffer, "</a>"); break;
        //case MD_SPAN_DEL:               text_buffer_append_all(r->buffer, "</del>"); break;
        //case MD_SPAN_CODE:              text_buffer_append_all(r->buffer, "</code>"); break;
        case MD_SPAN_IMG:       break;
        default:                break;
    }

    ctx->block->spans = g_list_delete_link (ctx->block->spans, span_link);
    md_span_free (span);

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
        case MD_TEXT_BR:        text_buffer_append_all (ctx->buffer, "\n"); break;
        case MD_TEXT_SOFTBR:    text_buffer_append_all (ctx->buffer, "\n"); break;
        case MD_TEXT_ENTITY:    text_buffer_render_entity(ctx->buffer, text, size); break;
        default:                text_buffer_append_len (ctx->buffer, text, size); break;
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
    text_buffer_create_styles (ctx.buffer);

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
