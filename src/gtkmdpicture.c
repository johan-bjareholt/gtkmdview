#include "gtkmdpicture.h"

static GObjectClass *parent_class = NULL;

struct _GtkMdPicture
{
  GtkBox parent_instance;

  char *path;
  char *alt_text;
};

struct _GtkMdPictureClass
{
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE (GtkMdPicture, gtk_md_picture, GTK_TYPE_BOX);

typedef enum
{
  PROP_0 = 0,
  PROP_PATH,
  PROP_ALT_TEXT,
  N_PROPERTIES
} GtkMdPictureProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gtk_md_picture_set_property (GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec)
{
  GtkMdPicture *self = GTK_MD_PICTURE (object);

  switch (property_id) {
    case PROP_PATH:
      g_free (self->path);
      self->path = g_strdup (g_value_get_string (value));
      break;
    case PROP_ALT_TEXT:
      g_free (self->alt_text);
      self->alt_text = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gtk_md_picture_get_property (GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  GtkMdPicture *self = GTK_MD_PICTURE (object);

  switch (property_id) {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;
    case PROP_ALT_TEXT:
      g_value_set_string (value, self->alt_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gtk_md_picture_init (GtkMdPicture *self)
{
  self->path = g_strdup ("");
}

static void
gtk_md_picture_render (GtkMdPicture *self)
{
  GtkWidget *box = NULL;
  GtkWidget *picture = NULL;

  //g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);
  //gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  // picture
  picture = gtk_picture_new_for_filename (self->path);
  g_object_set (picture, "keep-aspect-ratio", TRUE, NULL);

  gtk_box_append(GTK_BOX(box), picture);

  // alt-text
  if (self->alt_text && strlen(self->alt_text) > 0) {
    GtkWidget *alt_text = NULL;

    alt_text = gtk_label_new(self->alt_text);

    gtk_box_append(GTK_BOX(box), alt_text);
  }

  gtk_widget_set_parent(box, GTK_WIDGET(self));
}

static void
gtk_md_picture_constructed (GObject *obj)
{
  GtkMdPicture *self = GTK_MD_PICTURE (obj);

  gtk_md_picture_render(self);

  G_OBJECT_CLASS(parent_class)->constructed (obj);
}

static void
gtk_md_picture_finalize (GObject *obj)
{
  GtkMdPicture *self = GTK_MD_PICTURE (obj);

  g_free (self->path);

  G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
gtk_md_picture_class_init (GtkMdPictureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gtk_widget_class_set_layout_manager_type (GTK_WIDGET_CLASS (klass),
                                          GTK_TYPE_GRID_LAYOUT);

  object_class->set_property = gtk_md_picture_set_property;
  object_class->get_property = gtk_md_picture_get_property;
  object_class->constructed = gtk_md_picture_constructed;
  object_class->finalize = gtk_md_picture_finalize;

  obj_properties[PROP_PATH] =
    g_param_spec_string ("path",
                         "Path of source image",
                         "Path of source image",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  obj_properties[PROP_ALT_TEXT] =
    g_param_spec_string ("alt-text",
                         "Alternative text",
                         "Alternative descriptive text of the picture",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

GtkWidget *
gtk_md_picture_new (const char *path, const char *alt_text)
{
  GtkMdPicture *md_view;

  md_view = g_object_new (GTK_TYPE_MD_PICTURE,
      "path", path,
      "alt-text", alt_text,
      NULL);

  return GTK_WIDGET (md_view);
}
