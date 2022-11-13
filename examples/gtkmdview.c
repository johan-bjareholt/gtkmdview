#include <glib.h>
#include "../src/gtkmdview.h"
#include "../src/gtkmdedit.h"

char * example_input = "\
# Heading 1\n\
This is a paragraph. Testing testing 123\n\
test\n\
## Heading 2\n\
test\n\
### Heading 3\n\
test\n\
#### Heading 4\n\
test\n\
##### Heading 5\n\
test\n\
### Bold and italic test\n\
\n\
**bold**\n\
*italic*\n\
**only bold, *bold and italic**, only italic*\n\
\n\
#### Inline test\n\
\n\
This inline test will first create some **bold** text and then some *slightly slanted italic* text\n\
\n\
##### Single image test\n\
\n\
![this is an image](IMG20210521082058.jpg)\n\
\n\
##### Multiple image test\n\
\n\
Only shown side-by-side if it fits in the window\n\
![this is the first image](IMG20210521082058.jpg)\
![this is the second image](IMG20210521082058.jpg)\n\
\n\
test\n\
### Multiple titles after another\n\
\n\
### Multiple titles after another\n\
\n\
### Multiple titles after another\n\
";

typedef struct _RenderButtonData {
    GtkWidget *md_edit;
    GtkWidget *md_view;
} RenderButtonData;

static void
render_button_clicked(GtkButton *button, gpointer user_data)
{
    RenderButtonData *render_button_data = (RenderButtonData *) user_data;
    g_autofree gchar *md_text = gtk_md_edit_serialize(GTK_MD_EDIT(render_button_data->md_edit));
    g_object_set(render_button_data->md_view, "md-input", md_text, NULL);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window = NULL;
  GtkWidget *root_box = NULL;
  GtkWidget *md_view = NULL;
  GtkWidget *md_edit = NULL;

  root_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set(root_box, "hexpand", TRUE, NULL);

  // Create MdEdit
  {
    GtkWidget *viewport = NULL;
    GtkWidget *scrolled_window = NULL;

    md_edit = gtk_md_edit_new (example_input, "/home/johan/");

    viewport = gtk_viewport_new (NULL, NULL);
    /* setting vscroll-policy is necessary for GtkPicture to adapt its size
     * properly inside a GtkScrolledWindow*/
    g_object_set (GTK_VIEWPORT (viewport), "vscroll-policy", GTK_SCROLL_NATURAL, NULL);
    gtk_viewport_set_child (GTK_VIEWPORT (viewport), md_edit);

    scrolled_window = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
            GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window),
        viewport);
    g_object_set(scrolled_window, "hexpand", TRUE, NULL);
    gtk_box_append(GTK_BOX(root_box), scrolled_window);
  }

  // Create MdView
  {
    GtkWidget *viewport = NULL;
    GtkWidget *scrolled_window = NULL;

    md_view = gtk_md_view_new (example_input, "/home/johan/");

    viewport = gtk_viewport_new (NULL, NULL);
    /* setting vscroll-policy is necessary for GtkPicture to adapt its size
     * properly inside a GtkScrolledWindow*/
    g_object_set (GTK_VIEWPORT (viewport), "vscroll-policy", GTK_SCROLL_NATURAL, NULL);
    gtk_viewport_set_child (GTK_VIEWPORT (viewport), md_view);

    scrolled_window = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
            GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window),
        viewport);
    g_object_set(scrolled_window, "hexpand", TRUE, NULL);
    gtk_box_append(GTK_BOX(root_box), scrolled_window);
  }

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "gtkmdview demo");
  gtk_window_set_child (GTK_WINDOW (window), root_box);

  GtkWidget *headerbar = gtk_header_bar_new();
  GtkWidget *render_button = gtk_button_new_with_label("Update preview");
  RenderButtonData *render_button_data = g_malloc0(sizeof(render_button_data));
  render_button_data->md_edit = md_edit;
  render_button_data->md_view = md_view;
  g_signal_connect_data(render_button, "clicked", (GCallback) render_button_clicked, render_button_data, (GClosureNotify) g_free, 0);
  gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), render_button);
  gtk_window_set_titlebar(GTK_WINDOW (window), headerbar);

  gtk_window_present (GTK_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
