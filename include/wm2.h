#ifndef WM2_H
#define WM2_H

#include "gui2.h"

typedef struct wm2_context wm2_context_t;

typedef enum {
    WM2_THEME_DARK = 0,
    WM2_THEME_LIGHT
} wm2_theme_t;

struct wm2_context {
    gui2_context_t* gui;
    
    // Desktop management
    gui2_color_t desktop_bg;
    
    // Window management
    gui2_window_t* active_window;
    gui2_window_t* window_stack[32];
    int window_count;
    
    // Theme
    wm2_theme_t theme;
    
    // Desktop widgets
    gui2_widget_t* taskbar;
    gui2_widget_t* desktop;
    
    // Window controls
    bool dragging_window;
    gui2_window_t* drag_window;
    int32_t drag_offset_x;
    int32_t drag_offset_y;
};

// Window Manager API
wm2_context_t* wm2_create(uint32_t screen_width, uint32_t screen_height, uint32_t* screen_buffer);
void wm2_destroy(wm2_context_t* wm);

// Window management
gui2_window_t* wm2_create_window(wm2_context_t* wm, const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height);
void wm2_close_window(wm2_context_t* wm, gui2_window_t* window);
void wm2_minimize_window(wm2_context_t* wm, gui2_window_t* window);
void wm2_maximize_window(wm2_context_t* wm, gui2_window_t* window);
void wm2_focus_window(wm2_context_t* wm, gui2_window_t* window);

// Desktop management
void wm2_set_wallpaper(wm2_context_t* wm, gui2_color_t color);
void wm2_set_theme(wm2_context_t* wm, wm2_theme_t theme);

// Main loop
void wm2_update(wm2_context_t* wm);
void wm2_render(wm2_context_t* wm);

// Input handling
void wm2_handle_mouse_move(wm2_context_t* wm, int32_t x, int32_t y);
void wm2_handle_mouse_button(wm2_context_t* wm, uint32_t button, bool pressed);
void wm2_handle_key(wm2_context_t* wm, uint32_t keycode, char character, bool pressed);

// Utility functions
gui2_window_t* wm2_window_at_point(wm2_context_t* wm, int32_t x, int32_t y);

#endif // WM2_H