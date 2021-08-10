#include "mdview_internal.h"
#include "gtkmdview.h"

static GObjectClass *parent_class = NULL;

struct _GtkMdView
{
  GtkTextView parent_instance;

  GtkWidget *box;
  gboolean rendered;

  char *md_input;
  char *img_prefix;
};

struct _GtkMdViewClass
{
  GtkTextViewClass parent_class;
};

G_DEFINE_TYPE (GtkMdView, gtk_md_view, GTK_TYPE_WIDGET);

typedef enum
{
  PROP_0 = 0,
  PROP_MD_INPUT,
  PROP_IMG_PREFIX,
  N_PROPERTIES
} GtkMdViewProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gtk_md_view_set_property (GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec)
{
  GtkMdView *self = GTK_MD_VIEW (object);

  switch (property_id) {
    case PROP_MD_INPUT:
      g_free (self->md_input);
      self->md_input = g_strdup (g_value_get_string (value));
      break;
    case PROP_IMG_PREFIX:
      g_free (self->img_prefix);
      self->img_prefix = g_strdup (g_value_get_string (value));
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
gtk_md_view_init (GtkMdView *self)
{
  self->rendered = FALSE;
  self->md_input = g_strdup ("");
}

static void
gtk_md_view_constructed (GObject *obj)
{
  GtkMdView *self = GTK_MD_VIEW (obj);

  self->box = mdview_internal_new (self->md_input, self->img_prefix);

  gtk_widget_set_parent (self->box, GTK_WIDGET (self));

  G_OBJECT_CLASS(parent_class)->constructed (obj);
}

static void
gtk_md_view_dispose (GObject *obj)
{
  GtkMdView *self = GTK_MD_VIEW (obj);

  if (self->box) {
    g_clear_pointer (&self->box, gtk_widget_unparent);
  }

  G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
gtk_md_view_class_init (GtkMdViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gtk_widget_class_set_layout_manager_type (GTK_WIDGET_CLASS (klass),
                                          GTK_TYPE_GRID_LAYOUT);

  object_class->set_property = gtk_md_view_set_property;
  object_class->get_property = gtk_md_view_get_property;
  object_class->constructed = gtk_md_view_constructed;
  object_class->dispose = gtk_md_view_dispose;

  obj_properties[PROP_MD_INPUT] =
    g_param_spec_string ("md-input",
                         "Markdown input",
                         "Markdown input string to render",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  obj_properties[PROP_IMG_PREFIX] =
    g_param_spec_string ("img-prefix",
                         "Image path prefix",
                         "Prefix for the paths of the images in the markdown",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
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
