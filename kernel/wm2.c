#include "../include/wm2.h"
#include "../include/memory.h"

static void wm2_taskbar_event_handler(gui2_widget_t* widget, gui2_event_t* event);
static void wm2_window_event_handler(gui2_widget_t* widget, gui2_event_t* event);

wm2_context_t* wm2_create(uint32_t screen_width, uint32_t screen_height, uint32_t* screen_buffer) {
    wm2_context_t* wm = (wm2_context_t*)kmalloc(sizeof(wm2_context_t));
    if (!wm) return NULL;
    
    wm->gui = gui2_create_context(screen_width, screen_height, screen_buffer);
    if (!wm->gui) {
        kfree(wm);
        return NULL;
    }
    
    wm->desktop_bg = gui2_make_color(30, 30, 35, 255);
    wm->active_window = NULL;
    wm->window_count = 0;
    wm->theme = WM2_THEME_DARK;
    
    wm->dragging_window = false;
    wm->drag_window = NULL;
    wm->drag_offset_x = 0;
    wm->drag_offset_y = 0;
    
    // Create desktop widget (full screen background)
    wm->desktop = gui2_create_widget(GUI2_WIDGET_CONTAINER, NULL);
    if (wm->desktop) {
        gui2_set_rect(wm->desktop, 0, 0, screen_width, screen_height);
        wm->desktop->bg_color = wm->desktop_bg;
        wm->desktop->border_width = 0;
        gui2_set_visible(wm->desktop, true);
    }
    
    // Create taskbar as a special GUI2 window that's always on top
    wm->taskbar = NULL; // We'll handle taskbar differently for now
    
    wm2_set_theme(wm, WM2_THEME_DARK);
    
    return wm;
}

void wm2_destroy(wm2_context_t* wm) {
    if (!wm) return;
    
    if (wm->desktop) {
        gui2_destroy_widget(wm->desktop);
    }
    
    if (wm->gui) {
        gui2_destroy_context(wm->gui);
    }
    
    kfree(wm);
}

gui2_window_t* wm2_create_window(wm2_context_t* wm, const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!wm || !wm->gui) return NULL;
    
    // Clamp position to screen bounds
    if (x < 0) x = 50;
    if (y < 0) y = 50;
    if (x + (int32_t)width > (int32_t)wm->gui->screen_width) {
        x = wm->gui->screen_width - width - 50;
    }
    if (y + (int32_t)height > (int32_t)wm->gui->screen_height - 40) {
        y = wm->gui->screen_height - height - 90; // Account for taskbar
    }
    
    gui2_window_t* window = gui2_create_window(wm->gui, title, x, y, width, height);
    if (!window) return NULL;
    
    // Add to window stack
    if (wm->window_count < 32) {
        wm->window_stack[wm->window_count++] = window;
    }
    
    // Set event handler for window
    gui2_set_event_handler(window->root_widget, wm2_window_event_handler);
    window->root_widget->user_data = wm;
    window->user_data = wm;
    
    gui2_show_window(window);
    wm2_focus_window(wm, window);
    
    return window;
}

void wm2_close_window(wm2_context_t* wm, gui2_window_t* window) {
    if (!wm || !window) return;
    
    // Remove from window stack
    for (int i = 0; i < wm->window_count; i++) {
        if (wm->window_stack[i] == window) {
            for (int j = i; j < wm->window_count - 1; j++) {
                wm->window_stack[j] = wm->window_stack[j + 1];
            }
            wm->window_count--;
            break;
        }
    }
    
    if (wm->active_window == window) {
        wm->active_window = wm->window_count > 0 ? wm->window_stack[wm->window_count - 1] : NULL;
        if (wm->active_window) {
            gui2_focus_window(wm->gui, wm->active_window);
        }
    }
    
    gui2_destroy_window(wm->gui, window);
}

void wm2_minimize_window(wm2_context_t* wm, gui2_window_t* window) {
    if (!wm || !window) return;
    
    gui2_hide_window(window);
    
    if (wm->active_window == window) {
        wm->active_window = NULL;
        // Find next visible window to focus
        for (int i = wm->window_count - 1; i >= 0; i--) {
            if (wm->window_stack[i] != window && (wm->window_stack[i]->flags & GUI2_WIDGET_VISIBLE)) {
                wm2_focus_window(wm, wm->window_stack[i]);
                break;
            }
        }
    }
}

void wm2_maximize_window(wm2_context_t* wm, gui2_window_t* window) {
    if (!wm || !window) return;
    
    // Maximize to full screen minus taskbar
    window->rect.x = 0;
    window->rect.y = 0;
    window->rect.width = wm->gui->screen_width;
    window->rect.height = wm->gui->screen_height - 40;
    
    if (window->root_widget) {
        gui2_set_rect(window->root_widget, 0, 30, window->rect.width, window->rect.height - 30);
    }
    
    window->needs_redraw = true;
}

void wm2_focus_window(wm2_context_t* wm, gui2_window_t* window) {
    if (!wm || !window) return;
    
    wm->active_window = window;
    gui2_focus_window(wm->gui, window);
    
    // Move window to top of stack
    for (int i = 0; i < wm->window_count; i++) {
        if (wm->window_stack[i] == window) {
            for (int j = i; j < wm->window_count - 1; j++) {
                wm->window_stack[j] = wm->window_stack[j + 1];
            }
            wm->window_stack[wm->window_count - 1] = window;
            break;
        }
    }
}

void wm2_set_wallpaper(wm2_context_t* wm, gui2_color_t color) {
    if (!wm) return;
    
    wm->desktop_bg = color;
    if (wm->desktop) {
        wm->desktop->bg_color = color;
    }
    if (wm->gui) {
        wm->gui->theme_bg = color;
    }
}

void wm2_set_theme(wm2_context_t* wm, wm2_theme_t theme) {
    if (!wm) return;
    
    wm->theme = theme;
    
    if (theme == WM2_THEME_DARK) {
        wm->gui->theme_bg = gui2_make_color(30, 30, 35, 255);
        wm->gui->theme_fg = gui2_make_color(255, 255, 255, 255);
        wm->gui->theme_accent = gui2_make_color(0, 122, 255, 255);
        wm->gui->theme_border = gui2_make_color(70, 70, 75, 255);
        wm->desktop_bg = gui2_make_color(30, 30, 35, 255);
    } else {
        wm->gui->theme_bg = gui2_make_color(240, 240, 245, 255);
        wm->gui->theme_fg = gui2_make_color(30, 30, 30, 255);
        wm->gui->theme_accent = gui2_make_color(0, 122, 255, 255);
        wm->gui->theme_border = gui2_make_color(200, 200, 200, 255);
        wm->desktop_bg = gui2_make_color(240, 240, 245, 255);
    }
    
    if (wm->desktop) {
        wm->desktop->bg_color = wm->desktop_bg;
    }
}

gui2_window_t* wm2_window_at_point(wm2_context_t* wm, int32_t x, int32_t y) {
    if (!wm) return NULL;
    
    // Check windows from top to bottom (reverse order in stack)
    for (int i = wm->window_count - 1; i >= 0; i--) {
        gui2_window_t* window = wm->window_stack[i];
        if (window->flags & GUI2_WIDGET_VISIBLE && gui2_rect_contains(window->rect, x, y)) {
            return window;
        }
    }
    
    return NULL;
}

static bool wm2_point_in_titlebar(gui2_window_t* window, int32_t x, int32_t y) {
    if (!window) return false;
    
    gui2_rect_t titlebar = {window->rect.x, window->rect.y, window->rect.width, 30};
    return gui2_rect_contains(titlebar, x, y);
}

static bool wm2_point_in_close_button(gui2_window_t* window, int32_t x, int32_t y) {
    if (!window || !window->closable) return false;
    
    gui2_rect_t close_btn = {window->rect.x + 10, window->rect.y + 8, 14, 14};
    return gui2_rect_contains(close_btn, x, y);
}

static bool wm2_point_in_minimize_button(gui2_window_t* window, int32_t x, int32_t y) {
    if (!window || !window->minimizable) return false;
    
    gui2_rect_t min_btn = {window->rect.x + 30, window->rect.y + 8, 14, 14};
    return gui2_rect_contains(min_btn, x, y);
}

static bool wm2_point_in_maximize_button(gui2_window_t* window, int32_t x, int32_t y) {
    if (!window || !window->resizable) return false;
    
    gui2_rect_t max_btn = {window->rect.x + 50, window->rect.y + 8, 14, 14};
    return gui2_rect_contains(max_btn, x, y);
}

void wm2_handle_mouse_move(wm2_context_t* wm, int32_t x, int32_t y) {
    if (!wm) return;
    
    if (wm->dragging_window && wm->drag_window) {
        // Update window position while dragging
        wm->drag_window->rect.x = x - wm->drag_offset_x;
        wm->drag_window->rect.y = y - wm->drag_offset_y;
        
        // Clamp to screen bounds
        if (wm->drag_window->rect.x < 0) wm->drag_window->rect.x = 0;
        if (wm->drag_window->rect.y < 0) wm->drag_window->rect.y = 0;
        if (wm->drag_window->rect.x + (int32_t)wm->drag_window->rect.width > (int32_t)wm->gui->screen_width) {
            wm->drag_window->rect.x = wm->gui->screen_width - wm->drag_window->rect.width;
        }
        if (wm->drag_window->rect.y + (int32_t)wm->drag_window->rect.height > (int32_t)wm->gui->screen_height - 40) {
            wm->drag_window->rect.y = wm->gui->screen_height - 40 - wm->drag_window->rect.height;
        }
        
        wm->drag_window->needs_redraw = true;
    }
    
    gui2_mouse_move(wm->gui, x, y);
}

void wm2_handle_mouse_button(wm2_context_t* wm, uint32_t button, bool pressed) {
    if (!wm) return;
    
    int32_t x = wm->gui->mouse_x;
    int32_t y = wm->gui->mouse_y;
    
    if (button == 0) { // Left click
        if (pressed) {
            gui2_window_t* window = wm2_window_at_point(wm, x, y);
            
            if (window) {
                wm2_focus_window(wm, window);
                
                // Check window control buttons
                if (wm2_point_in_close_button(window, x, y)) {
                    wm2_close_window(wm, window);
                    return;
                } else if (wm2_point_in_minimize_button(window, x, y)) {
                    wm2_minimize_window(wm, window);
                    return;
                } else if (wm2_point_in_maximize_button(window, x, y)) {
                    wm2_maximize_window(wm, window);
                    return;
                } else if (wm2_point_in_titlebar(window, x, y)) {
                    // Start dragging
                    wm->dragging_window = true;
                    wm->drag_window = window;
                    wm->drag_offset_x = x - window->rect.x;
                    wm->drag_offset_y = y - window->rect.y;
                    return;
                }
            }
        } else {
            // Stop dragging
            wm->dragging_window = false;
            wm->drag_window = NULL;
        }
    }
    
    gui2_mouse_button(wm->gui, button, pressed);
}

void wm2_handle_key(wm2_context_t* wm, uint32_t keycode, char character, bool pressed) {
    if (!wm) return;
    
    gui2_key_event(wm->gui, keycode, character, pressed);
}

void wm2_update(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_update(wm->gui);
}

void wm2_render(wm2_context_t* wm) {
    if (!wm) return;
    
    // Use the GUI2 render system which handles everything properly including double buffering
    gui2_render(wm->gui);
    
    // Taskbar is now rendered as part of GUI2 system, no need to draw it separately
}

static void wm2_taskbar_event_handler(gui2_widget_t* widget, gui2_event_t* event) {
    (void)widget;
    (void)event;
    // Handle taskbar events (app launching, etc.)
}

static void wm2_window_event_handler(gui2_widget_t* widget, gui2_event_t* event) {
    (void)widget;
    (void)event;
    // Handle window widget events
}