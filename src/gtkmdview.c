#include "md2gtktextbuf.h"
#include "gtkmdview.h"

struct _GtkMdView
{
  GtkTextView parent_instance;

  GtkTextBuffer *buffer;

  gchar *md_input;
};

struct _GtkMdViewClass
{
  GtkTextViewClass parent_class;
};

G_DEFINE_TYPE (GtkMdView, gtk_md_view, GTK_TYPE_TEXT_VIEW);

static void
gtk_md_view_class_init (GtkMdViewClass *klass)
{
}

static void
gtk_md_view_init (GtkMdView *self)
{
}

GtkWidget *
gtk_md_view_new (gchar *md_input)
{
  /* TODO: make md_input a property */
  GtkMdView *md_view;
  GtkTextBuffer *buffer;

  md_view = g_object_new (GTK_MD_TYPE_VIEW, NULL);
  md_view->md_input = g_strdup (md_input);

  g_object_set (md_view, "valign", GTK_ALIGN_FILL, NULL);
  g_object_set (md_view, "halign", GTK_ALIGN_FILL, NULL);
  g_object_set (md_view, "hexpand", TRUE, NULL);
  g_object_set (md_view, "vexpand", TRUE, NULL);
  g_object_set (md_view, "editable", FALSE, NULL);
  g_object_set (md_view, "wrap-mode", GTK_WRAP_WORD, NULL);

  /* md to textview */
  buffer = md2textbuffer (md_view->md_input);
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (md_view), buffer);

  return GTK_WIDGET (md_view);
}
