#pragma once

#include <gtk/gtk.h>

#define GTK_TYPE_MD_VIEW (gtk_md_view_get_type())

G_DECLARE_FINAL_TYPE (GtkMdView, gtk_md_view, GTK, MD_VIEW, GtkTextView);

GtkWidget *
gtk_md_view_new (char *md_input, char *img_prefix);
