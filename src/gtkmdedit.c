#include "gtkmdedit.h"
#include "mdedit_render.h"
#include "mdedit_serializer.h"
#include "textbuffer_utils.h"

static GObjectClass *parent_class = NULL;

struct _GtkMdEdit
{
  GtkWidget parent_instance;

  GtkWidget *textview;
  gboolean rendered;

  char *md_input;
  char *img_prefix;

  GList *blocks;
};

struct _GtkMdEditClass
{
  GtkWidgetClass parent_class;
};

G_DEFINE_TYPE (GtkMdEdit, gtk_md_edit, GTK_TYPE_WIDGET);

typedef enum
{
  PROP_0 = 0,
  PROP_MD_INPUT,
  PROP_IMG_PREFIX,
  N_PROPERTIES
} GtkMdEditProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gtk_md_edit_set_property (GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec)
{
  GtkMdEdit *self = GTK_MD_EDIT (object);

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
gtk_md_edit_get_property (GObject *object, guint property_id, GValue *value,
    GParamSpec *pspec)
{
  GtkMdEdit *self = GTK_MD_EDIT (object);

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
gtk_md_edit_init (GtkMdEdit *self)
{
  self->textview = NULL;
  self->rendered = FALSE;

  self->md_input = g_strdup ("");
  self->img_prefix = NULL;

  self->blocks = NULL;
}

static void
gtk_md_edit_constructed (GObject *obj)
{
  GtkMdEdit *self = GTK_MD_EDIT (obj);

  self->textview = gtk_text_view_new ();
  g_object_set (self->textview, "editable", TRUE, NULL);
  g_object_set (self->textview, "wrap-mode", GTK_WRAP_WORD, NULL);
  g_object_set (self->textview, "hexpand", TRUE, NULL);
  g_object_set (self->textview, "vexpand", TRUE, NULL);

  mdedit_render(self->textview, self->md_input, self->img_prefix, &self->blocks);

  gtk_widget_set_parent (self->textview, GTK_WIDGET (self));

  G_OBJECT_CLASS(parent_class)->constructed (obj);
}

static void
gtk_md_edit_finalize (GObject *obj)
{
  GtkMdEdit *self = GTK_MD_EDIT (obj);

  g_free (self->md_input);
  g_free (self->img_prefix);

  g_list_free_full(self->blocks, (GDestroyNotify) md_block2_free);

  G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
gtk_md_edit_dispose (GObject *obj)
{
  GtkMdEdit *self = GTK_MD_EDIT (obj);

  if (self->textview) {
    g_clear_pointer (&self->textview, gtk_widget_unparent);
  }

  G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
gtk_md_edit_class_init (GtkMdEditClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gtk_widget_class_set_layout_manager_type (GTK_WIDGET_CLASS (klass),
                                          GTK_TYPE_GRID_LAYOUT);

  object_class->set_property = gtk_md_edit_set_property;
  object_class->get_property = gtk_md_edit_get_property;
  object_class->constructed = gtk_md_edit_constructed;
  object_class->dispose = gtk_md_edit_dispose;
  object_class->finalize = gtk_md_edit_finalize;

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
gtk_md_edit_new (char *md_input, char *img_prefix)
{
  GtkMdEdit *md_edit;

  md_edit = g_object_new (GTK_TYPE_MD_EDIT,
      "md-input", md_input,
      "img-prefix", img_prefix,
      NULL);

  return GTK_WIDGET (md_edit);
}

char *
gtk_md_edit_serialize(GtkMdEdit *md_edit)
{
    char *a = md_edit_serialize(GTK_TEXT_VIEW(md_edit->textview), md_edit->blocks);
    printf("%s\n", a);
    return a;
}
