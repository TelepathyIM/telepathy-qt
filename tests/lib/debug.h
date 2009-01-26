#undef DEBUG
#define DEBUG(format, ...) \
  g_debug ("%s: " format, G_STRFUNC, ##__VA_ARGS__)
