#include "App.h"

int main() {
#ifdef __EMSCRIPTEN__
    // the browser keeps calling the main loop after main() returns, so the app must outlive its frame
    static Viewer::App app;
#else
    Viewer::App app;
#endif
    if (!app.init()) {
        return 1;
    }
    app.run();
    return 0;
}
