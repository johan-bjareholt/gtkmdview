#pragma once

#include <gtk/gtk.h>

#define GTK_MD_TYPE_VIEW (gtk_md_view_get_type())

G_DECLARE_FINAL_TYPE (GtkMdView, gtk_md_view, GTK_MD, VIEW, GtkTextView)

GtkWidget *
gtk_md_view_new (gchar *md_input);
