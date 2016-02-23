
#include <glib.h>


GKeyFile * 
sway_load_config(const char * conffile) 
{
    GKeyFile * keyfile = g_key_file_new();
    gboolean success; 

    success = g_key_file_load_from_file(
        keyfile, conffile, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS
    );
    if (success) {
        return keyfile;
    }
    else {
        g_key_file_free(keyfile);
        return NULL;
    }
}

void 
sway_free_config(GKeyFile * keyfile) { 
    if (keyfile != NULL) {
        g_key_file_free(keyfile);
    }
}
