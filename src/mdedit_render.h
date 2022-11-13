#pragma once

#include <gtk/gtk.h>

void mdedit_render (GtkWidget *textview, const char* input,
    const char *img_prefix, GList **blocks);
