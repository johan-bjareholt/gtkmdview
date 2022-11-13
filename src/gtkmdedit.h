#pragma once

#include <gtk/gtk.h>

#define GTK_TYPE_MD_EDIT (gtk_md_edit_get_type())

G_DECLARE_FINAL_TYPE (GtkMdEdit, gtk_md_edit, GTK, MD_EDIT, GtkWidget);

GtkWidget *
gtk_md_edit_new (char *md_input, char *img_prefix);

char *
gtk_md_edit_serialize(GtkMdEdit *md_edit);
