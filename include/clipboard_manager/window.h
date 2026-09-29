#ifndef CLIPBOARD_MANAGER_WINDOW_H
#define CLIPBOARD_MANAGER_WINDOW_H

#include <sqlite3.h>

#include "clipboard_manager/clipboard_monitor.h"

void window_show_main(sqlite3 *database, ClipboardMonitor *monitor);

#endif