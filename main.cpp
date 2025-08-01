#include <stdio.h>
#include <stdlib.h>
#include "app/app_core.h"

int main(void) {
    Application app = {NULL, false, NULL, NULL};

    printf("TinyRequest starting...\n");

    if (app_core_init(&app) != 0) {
        fprintf(stderr, "Failed to initialize application\n");
        app_core_cleanup(&app);
        return 1;
    }

    app_core_run_main_loop(&app);

    app_core_cleanup(&app);
    printf("TinyRequest shutting down...\n");

    return 0;
}
