#include "mdedit_serializer.h"
#include "textbuffer_utils.h"

static const char *
style_to_md_pre(const char *style)
{
    if (g_strcmp0(style, "i") == 0) {
        return "*";
    } else if (g_strcmp0(style, "b") == 0) {
        return "**";
    } else if (g_strcmp0(style, "h1") == 0) {
        return "# ";
    } else if (g_strcmp0(style, "h2") == 0) {
        return "## ";
    } else if (g_strcmp0(style, "h3") == 0) {
        return "### ";
    } else if (g_strcmp0(style, "h4") == 0) {
        return "#### ";
    } else if (g_strcmp0(style, "h5") == 0) {
        return "##### ";
    } else if (g_strcmp0(style, "h6") == 0) {
        return "###### ";
    } else {
        return "";
    }
}

static const char *
style_to_md_post(const char *style)
{
    if (g_strcmp0(style, "i") == 0) {
        return "*";
    } else if (g_strcmp0(style, "b") == 0) {
        return "**";
    } else {
        return "";
    }
}

char *
md_edit_serialize(GtkTextView *textview, GList *blocks)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GString *serialized_text = g_string_new("");

    for (GList *block_iter = blocks; block_iter != NULL; block_iter = block_iter->next) {
        MdBlock2 *block = (MdBlock2 *) block_iter->data;
        GtkTextIter prev_iter;
        GtkTextIter end_iter;
        gtk_text_buffer_get_iter_at_mark(buffer, &prev_iter, block->start);

        if (block->style != NULL) {
            g_string_append(serialized_text, style_to_md_pre(block->style));
        }

        for (GList *span_iter = block->spans; span_iter != NULL; span_iter = span_iter->next) {
            MdSpan *span = (MdSpan *) span_iter->data;
            GtkTextIter start_iter;
            GtkTextIter end_iter;

            gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, span->start);
            gtk_text_buffer_get_iter_at_mark (buffer, &end_iter, span->end);
            g_autofree char *text_prev = gtk_text_buffer_get_text(buffer, &prev_iter, &start_iter, FALSE);
            g_string_append(serialized_text, text_prev);
            g_autofree char *text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
            if (span->img_path != NULL) {
                g_string_append(serialized_text, "![");
            }
            if (span->style != NULL) {
                g_string_append(serialized_text, style_to_md_pre(span->style));
            }
            g_string_append(serialized_text, text);
            if (span->style != NULL) {
                g_string_append(serialized_text, style_to_md_post(span->style));
            }
            if (span->img_path != NULL) {
                g_string_append_printf(serialized_text, "](%s)\n", span->img_path);
            }
            gtk_text_buffer_get_iter_at_mark (buffer, &prev_iter, span->end);
        }

        gtk_text_buffer_get_iter_at_mark(buffer, &end_iter, block->end);
        g_autofree char *text = gtk_text_buffer_get_text(buffer, &prev_iter, &end_iter, FALSE);
        g_string_append(serialized_text, text);
        if (block->style != NULL) {
            g_string_append(serialized_text, style_to_md_post(block->style));
        }
        g_string_append(serialized_text, "\n\n");
    }

    return g_string_free (serialized_text, FALSE);
}
