#include "../include/wm2.h"
#include "../include/memory.h"

// Global reference for desktop rendering
wm2_context_t* global_wm = NULL;

static void wm2_taskbar_event_handler(gui2_widget_t* widget, gui2_event_t* event);
static void wm2_window_event_handler(gui2_widget_t* widget, gui2_event_t* event);
static void wm2_create_taskbar(wm2_context_t* wm);
static void wm2_add_taskbar_apps(wm2_context_t* wm);
static void wm2_render_taskbar(wm2_context_t* wm);

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
    
    // Create beautiful dock-style taskbar
    wm2_create_taskbar(wm);
    
    wm2_set_theme(wm, WM2_THEME_DARK);
    
    // Set global reference
    global_wm = wm;
    
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
    
    if (global_wm == wm) {
        global_wm = NULL;
    }
    
    kfree(wm);
}

gui2_window_t* wm2_create_window(wm2_context_t* wm, const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!wm || !wm->gui) return NULL;
    
    // Clamp position to screen bounds - make sure titlebar is visible
    if (x < 0) x = 10;
    if (y < 30) y = 30;  // Make sure titlebar is below any potential menu bar
    if (x + (int32_t)width > (int32_t)wm->gui->screen_width) {
        x = wm->gui->screen_width - width - 10;
    }
    if (y + (int32_t)height > (int32_t)wm->gui->screen_height) {
        y = wm->gui->screen_height - height - 10;
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
                
                // Always pass events to GUI system for widget handling
                gui2_mouse_button(wm->gui, button, pressed);
                return;
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
    
    // Render taskbar on top of everything with special effects
    if (wm->taskbar && (wm->taskbar->flags & GUI2_WIDGET_VISIBLE)) {
        wm2_render_taskbar(wm);
    }
}

static void wm2_render_taskbar(wm2_context_t* wm) {
    if (!wm || !wm->taskbar) return;
    
    gui2_context_t* ctx = wm->gui;
    
    // Draw dock background with blur effect simulation
    gui2_rect_t dock_rect = {
        wm->taskbar->rect.x - 5,
        wm->taskbar->rect.y - 5, 
        wm->taskbar->rect.width + 10,
        wm->taskbar->rect.height + 10
    };
    
    // Draw shadow
    gui2_draw_rounded_rect(ctx, dock_rect, gui2_make_color(0, 0, 0, 60), 20.0f);
    
    // Draw main dock
    gui2_draw_rounded_rect(ctx, wm->taskbar->rect, wm->taskbar->bg_color, 16.0f);
    
    // Render dock icons with hover effects
    gui2_widget_t* child = wm->taskbar->first_child;
    while (child) {
        if (child->flags & GUI2_WIDGET_VISIBLE) {
            gui2_rect_t icon_rect = {
                wm->taskbar->rect.x + child->rect.x,
                wm->taskbar->rect.y + child->rect.y,
                child->rect.width,
                child->rect.height
            };
            
            // Draw icon with nice rounded corners
            gui2_draw_rounded_rect(ctx, icon_rect, child->bg_color, 10.0f);
            
            // Add shine effect
            gui2_color_t shine = gui2_make_color(255, 255, 255, 40);
            gui2_rect_t shine_rect = {
                icon_rect.x + 2,
                icon_rect.y + 2,
                icon_rect.width - 4,
                icon_rect.height / 3
            };
            gui2_draw_rounded_rect(ctx, shine_rect, shine, 8.0f);
        }
        child = child->next_sibling;
    }
}

static void wm2_app_icon_event_handler(gui2_widget_t* widget, gui2_event_t* event);

static void wm2_taskbar_event_handler(gui2_widget_t* widget, gui2_event_t* event) {
    (void)widget;
    (void)event;
    // Taskbar doesn't handle events directly - child widgets (icons) do
}

static void wm2_window_event_handler(gui2_widget_t* widget, gui2_event_t* event) {
    (void)widget;
    (void)event;
    // Handle window widget events
}

// App creation functions
static void wm2_create_finder_window(wm2_context_t* wm);
static void wm2_create_terminal_window(wm2_context_t* wm);
static void wm2_create_calculator_window(wm2_context_t* wm);
static void wm2_create_settings_window(wm2_context_t* wm);
static void wm2_create_installer_window(wm2_context_t* wm);

static void wm2_app_icon_event_handler(gui2_widget_t* widget, gui2_event_t* event) {
    if (!widget || !event) return;
    
    // Get window manager directly from user_data
    wm2_context_t* wm = (wm2_context_t*)widget->user_data;
    if (!wm) return;
    
    // Handle mouse down event only (not mouse up to avoid double creation)
    if (event->type == GUI2_EVENT_MOUSE_DOWN) {
        // Get app index from widget ID
        int app_index = (int)widget->id;
        
        // Create appropriate window based on app index
        switch (app_index) {
            case 0: // Finder
                wm2_create_finder_window(wm);
                break;
            case 1: // Terminal
                wm2_create_terminal_window(wm);
                break;
            case 2: // Calculator
                wm2_create_calculator_window(wm);
                break;
            case 3: // Settings
                wm2_create_settings_window(wm);
                break;
            case 4: // Installer
                wm2_create_installer_window(wm);
                break;
        }
    }
}

static void wm2_create_taskbar(wm2_context_t* wm) {
    if (!wm || !wm->gui) return;
    
    uint32_t taskbar_width = 400;
    uint32_t taskbar_height = 60;
    uint32_t taskbar_x = (wm->gui->screen_width - taskbar_width) / 2;
    uint32_t taskbar_y = wm->gui->screen_height - taskbar_height - 10;
    
    // Create taskbar as floating dock
    wm->taskbar = gui2_create_widget(GUI2_WIDGET_PANEL, wm->desktop);
    if (wm->taskbar) {
        gui2_set_rect(wm->taskbar, taskbar_x, taskbar_y, taskbar_width, taskbar_height);
        wm->taskbar->bg_color = gui2_make_color(40, 40, 45, 200); // Semi-transparent
        wm->taskbar->border_width = 0;
        gui2_set_visible(wm->taskbar, true);
        gui2_set_event_handler(wm->taskbar, wm2_taskbar_event_handler);
        wm->taskbar->user_data = wm;
        
        // Create app icons in taskbar
        wm2_add_taskbar_apps(wm);
    }
}

static void wm2_add_taskbar_apps(wm2_context_t* wm) {
    if (!wm || !wm->taskbar) return;
    
    // App icon data (simple colored squares for now)
    struct {
        const char* name;
        gui2_color_t color;
    } apps[] = {
        {"Finder", gui2_make_color(0, 122, 255, 255)},      // Blue
        {"Terminal", gui2_make_color(50, 50, 55, 255)},     // Dark gray
        {"Calculator", gui2_make_color(255, 149, 0, 255)},  // Orange
        {"Settings", gui2_make_color(142, 142, 147, 255)},  // Gray
        {"Installer", gui2_make_color(76, 175, 80, 255)}    // Green
    };
    
    int app_count = sizeof(apps) / sizeof(apps[0]);
    int icon_size = 44;
    int spacing = 8;
    int total_width = app_count * icon_size + (app_count - 1) * spacing;
    int start_x = (wm->taskbar->rect.width - total_width) / 2;
    int start_y = (wm->taskbar->rect.height - icon_size) / 2;
    
    for (int i = 0; i < app_count; i++) {
        gui2_widget_t* app_icon = gui2_create_widget(GUI2_WIDGET_BUTTON, wm->taskbar);
        if (app_icon) {
            // Position relative to taskbar coordinates
            int x = start_x + i * (icon_size + spacing);
            int y = start_y;
            gui2_set_rect(app_icon, x, y, icon_size, icon_size);
            app_icon->bg_color = apps[i].color;
            app_icon->border_width = 0;
            gui2_set_visible(app_icon, true);
            
            // Set app index and event handler - store window manager directly
            app_icon->user_data = wm;  // Store wm directly instead of index
            gui2_set_event_handler(app_icon, wm2_app_icon_event_handler);
            
            // Store app index in a different way - use widget ID
            app_icon->id = i;  // Use widget ID to store app index
        }
    }
}

// App window creation implementations
static void wm2_create_finder_window(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_window_t* window = wm2_create_window(wm, "Finder", 100, 100, 600, 400);
    if (window) {
        // Simple content - just set background color
        window->root_widget->bg_color = gui2_make_color(245, 245, 247, 255);
    }
}

static void wm2_create_terminal_window(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_window_t* window = wm2_create_window(wm, "Terminal", 150, 150, 650, 450);
    if (window) {
        // Simple dark background
        window->root_widget->bg_color = gui2_make_color(40, 44, 52, 255);
    }
}

static void wm2_create_calculator_window(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_window_t* window = wm2_create_window(wm, "Calculator", 200, 200, 320, 420);
    if (window) {
        // Simple light background
        window->root_widget->bg_color = gui2_make_color(248, 248, 248, 255);
    }
}

static void wm2_create_settings_window(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_window_t* window = wm2_create_window(wm, "System Settings", 250, 150, 550, 450);
    if (window) {
        // Simple settings background
        window->root_widget->bg_color = gui2_make_color(250, 250, 250, 255);
    }
}

static void wm2_create_installer_window(wm2_context_t* wm) {
    if (!wm) return;
    
    gui2_window_t* window = wm2_create_window(wm, "ByteOS System Installer", 200, 100, 720, 550);
    if (window) {
        // Set installer background
        window->root_widget->bg_color = gui2_make_color(245, 245, 247, 255);
        
        // Header section with OS logo/icon area
        gui2_widget_t* header = gui2_create_label(window->root_widget, "ByteOS Installation");
        gui2_set_rect(header, 20, 20, 680, 35);
        header->bg_color = gui2_make_color(245, 245, 247, 255);
        header->fg_color = gui2_make_color(34, 34, 34, 255);
        
        // System info section
        gui2_widget_t* sys_info = gui2_create_panel(window->root_widget);
        gui2_set_rect(sys_info, 20, 70, 680, 140);
        sys_info->bg_color = gui2_make_color(255, 255, 255, 255);
        sys_info->border_width = 1;
        sys_info->border_color = gui2_make_color(200, 200, 200, 255);
        
        gui2_widget_t* os_name = gui2_create_label(sys_info, "Operating System: ByteOS v1.0.0");
        gui2_set_rect(os_name, 15, 15, 650, 25);
        os_name->bg_color = gui2_make_color(255, 255, 255, 255);
        os_name->fg_color = gui2_make_color(51, 51, 51, 255);
        
        gui2_widget_t* target_disk = gui2_create_label(sys_info, "Target Disk: /dev/sda1 (Primary HDD - 250 GB available)");
        gui2_set_rect(target_disk, 15, 45, 650, 20);
        target_disk->bg_color = gui2_make_color(255, 255, 255, 255);
        target_disk->fg_color = gui2_make_color(102, 102, 102, 255);
        
        gui2_widget_t* install_type = gui2_create_label(sys_info, "Installation Type: Full System Installation");
        gui2_set_rect(install_type, 15, 70, 650, 20);
        install_type->bg_color = gui2_make_color(255, 255, 255, 255);
        install_type->fg_color = gui2_make_color(102, 102, 102, 255);
        
        gui2_widget_t* requirements = gui2_create_label(sys_info, "Requirements: 512 MB RAM, 2 GB Disk Space");
        gui2_set_rect(requirements, 15, 95, 650, 20);
        requirements->bg_color = gui2_make_color(255, 255, 255, 255);
        requirements->fg_color = gui2_make_color(102, 102, 102, 255);
        
        // Progress section
        gui2_widget_t* progress_label = gui2_create_label(window->root_widget, "Installation Progress:");
        gui2_set_rect(progress_label, 20, 230, 400, 25);
        progress_label->bg_color = gui2_make_color(245, 245, 247, 255);
        progress_label->fg_color = gui2_make_color(34, 34, 34, 255);
        
        // Progress bar background
        gui2_widget_t* progress_bg = gui2_create_panel(window->root_widget);
        gui2_set_rect(progress_bg, 20, 260, 680, 35);
        progress_bg->bg_color = gui2_make_color(230, 230, 230, 255);
        progress_bg->border_width = 1;
        progress_bg->border_color = gui2_make_color(180, 180, 180, 255);
        
        // Progress bar fill (60% complete)
        gui2_widget_t* progress_fill = gui2_create_panel(progress_bg);
        gui2_set_rect(progress_fill, 2, 2, 408, 31); // 60% of 680px = 408px
        progress_fill->bg_color = gui2_make_color(0, 122, 255, 255);
        
        // Current step indicator
        gui2_widget_t* status = gui2_create_label(window->root_widget, "Step 3 of 5: Installing system kernel...");
        gui2_set_rect(status, 20, 305, 600, 25);
        status->bg_color = gui2_make_color(245, 245, 247, 255);
        status->fg_color = gui2_make_color(102, 102, 102, 255);
        
        // Current file being installed
        gui2_widget_t* current_file = gui2_create_label(window->root_widget, "Installing: /boot/kernel.bin");
        gui2_set_rect(current_file, 20, 335, 600, 20);
        current_file->bg_color = gui2_make_color(245, 245, 247, 255);
        current_file->fg_color = gui2_make_color(0, 122, 255, 255);
        
        // Installation log section
        gui2_widget_t* log_label = gui2_create_label(window->root_widget, "Installation Log:");
        gui2_set_rect(log_label, 20, 365, 200, 25);
        log_label->bg_color = gui2_make_color(245, 245, 247, 255);
        log_label->fg_color = gui2_make_color(34, 34, 34, 255);
        
        gui2_widget_t* log_area = gui2_create_panel(window->root_widget);
        gui2_set_rect(log_area, 20, 395, 680, 70);
        log_area->bg_color = gui2_make_color(20, 20, 25, 255);
        log_area->border_width = 1;
        log_area->border_color = gui2_make_color(200, 200, 200, 255);
        
        gui2_widget_t* log_text = gui2_create_label(log_area, 
            "[14:23:15] Partitioning disk /dev/sda1...\n"
            "[14:23:18] Creating filesystem ext4...\n"
            "[14:23:22] Copying bootloader...");
        gui2_set_rect(log_text, 10, 5, 660, 45);
        log_text->fg_color = gui2_make_color(0, 255, 100, 255);
        
        // Installation buttons
        gui2_widget_t* cancel_btn = gui2_create_button(window->root_widget, "Cancel");
        gui2_set_rect(cancel_btn, 520, 480, 80, 35);
        cancel_btn->bg_color = gui2_make_color(220, 220, 220, 255);
        cancel_btn->fg_color = gui2_make_color(34, 34, 34, 255);
        
        gui2_widget_t* next_btn = gui2_create_button(window->root_widget, "Continue");
        gui2_set_rect(next_btn, 610, 480, 90, 35);
        next_btn->bg_color = gui2_make_color(0, 122, 255, 255);
        next_btn->fg_color = gui2_make_color(255, 255, 255, 255);
    }
}