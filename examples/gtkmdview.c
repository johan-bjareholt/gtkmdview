#include <glib.h>
#include "../src/gtkmdview.h"

char * example_input = "# Heading 1\n\
This is a paragraph. Testing testing 123\n\
test\n\
## Heading 2\n\
test\n\
### Heading 3\n\
test\n\
#### Bold and italic test\n\
\n\
**bold**\n\
*italic*\n\
\n\
##### Inline test\n\
\n\
This inline test will first create some **bold** text and then some *slightly slanted italic* text\n\
\n\
###### Single image test\n\
\n\
![this is an image](IMG20210521082058.jpg)\n\
\n\
###### Multiple image test\n\
\n\
Only shown side-by-side if it fits in the window\n\
![this is an image](IMG20210521082058.jpg)\
![this is an image](IMG20210521082058.jpg)\n\
\n\
test\n\
#### Multiple titles after another\n\
\n\
#### Multiple titles after another\n\
\n\
#### Multiple titles after another\n\
";

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window = NULL;
  GtkWidget *scrolled_window = NULL;
  GtkWidget *viewport = NULL;
  GtkWidget *md_view = NULL;

  md_view = gtk_md_view_new (example_input, "/home/johan/");

  /* setting vscroll-policy is necessary for GtkPicture to adapt its size
   * properly inside a GtkScrolledWindow*/
  viewport = gtk_viewport_new (NULL, NULL);
  g_object_set (GTK_VIEWPORT (viewport), "vscroll-policy", GTK_SCROLL_NATURAL, NULL);
  gtk_viewport_set_child (GTK_VIEWPORT (viewport), md_view);

  scrolled_window = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
          GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window),
      viewport);

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "md4c-gtk4 demo");
  gtk_window_set_child (GTK_WINDOW (window), scrolled_window);

  gtk_widget_show (window);
}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
