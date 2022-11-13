#pragma once

#include <gtk/gtk.h>

#define GTK_TYPE_MD_PICTURE (gtk_md_picture_get_type())

G_DECLARE_FINAL_TYPE (GtkMdPicture, gtk_md_picture, GTK, MD_PICTURE, GtkBox);

GtkWidget *
gtk_md_picture_new (const char *path, const char *alt_text);
