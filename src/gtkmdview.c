#include "md2gtktextbuf.h"
#include "gtkmdview.h"

struct _GtkMdView
{
  GtkTextView parent_instance;

  GtkTextBuffer *buffer;
  gboolean rendered;

  char *md_input;
  char *img_prefix;
};

struct _GtkMdViewClass
{
  GtkTextViewClass parent_class;
};

G_DEFINE_TYPE (GtkMdView, gtk_md_view, GTK_TYPE_TEXT_VIEW);

typedef enum
{
  PROP_0 = 0,
  PROP_MD_INPUT,
  PROP_IMG_PREFIX,
  N_PROPERTIES
} GtkMdViewProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void reload_md (GtkMdView *self);

static void
gtk_md_view_set_property (GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec)
{
  GtkMdView *self = GTK_MD_VIEW (object);

  switch (property_id) {
    case PROP_MD_INPUT:
      g_free (self->md_input);
      self->md_input = g_strdup (g_value_get_string (value));
      reload_md (self);
      break;
    case PROP_IMG_PREFIX:
      g_free (self->img_prefix);
      self->img_prefix = g_strdup (g_value_get_string (value));
      reload_md (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gtk_md_view_get_property (GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  GtkMdView *self = GTK_MD_VIEW (object);

  switch (property_id) {
    case PROP_MD_INPUT:
      g_value_set_string (value, self->md_input);
      break;
    case PROP_IMG_PREFIX:
      g_value_set_string (value, self->img_prefix);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gtk_md_view_class_init (GtkMdViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gtk_md_view_set_property;
  object_class->get_property = gtk_md_view_get_property;

  obj_properties[PROP_MD_INPUT] =
    g_param_spec_string ("md-input",
                         "Markdown input",
                         "Markdown input string to render",
                         "",
                         G_PARAM_READWRITE);
  obj_properties[PROP_IMG_PREFIX] =
    g_param_spec_string ("img-prefix",
                         "Image path prefix",
                         "Prefix for the paths of the images in the markdown",
                         "",
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
reload_md (GtkMdView *self)
{
  GtkTextBuffer *buffer;

  if (self->rendered) {
    buffer = md2textbuffer (self->md_input, self->img_prefix);
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (self), buffer);
  }
}

static void
gtk_md_view_map (GtkMdView *self)
{
  self->rendered = TRUE;
  reload_md (self);
}

static void
gtk_md_view_unmap (GtkMdView *self)
{
  self->rendered = FALSE;
}

static void
gtk_md_view_init (GtkMdView *self)
{
  self->rendered = FALSE;
  self->buffer = NULL;
  self->md_input = g_strdup ("");

  g_object_set (self, "valign", GTK_ALIGN_FILL, NULL);
  g_object_set (self, "halign", GTK_ALIGN_FILL, NULL);
  g_object_set (self, "hexpand", TRUE, NULL);
  g_object_set (self, "vexpand", TRUE, NULL);
  g_object_set (self, "editable", FALSE, NULL);
  g_object_set (self, "wrap-mode", GTK_WRAP_WORD, NULL);

  /* render md_input when widget is loaded */
  g_assert (g_signal_connect (self, "map", G_CALLBACK (gtk_md_view_map), NULL) > 0);
  g_assert (g_signal_connect (self, "unmap", G_CALLBACK (gtk_md_view_unmap), NULL) > 0);
}

GtkWidget *
gtk_md_view_new (char *md_input, char *img_prefix)
{
  GtkMdView *md_view;

  md_view = g_object_new (GTK_TYPE_MD_VIEW,
      "md-input", md_input,
      "img-prefix", img_prefix,
      NULL);

  return GTK_WIDGET (md_view);
}
