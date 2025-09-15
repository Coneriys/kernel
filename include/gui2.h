#ifndef GUI2_H
#define GUI2_H

#include <stdint.h>
#include <stdbool.h>

// ===== NEW GUI FRAMEWORK v2.0 =====
// Clean, modern, modular design

// Forward declarations
typedef struct gui2_context gui2_context_t;
typedef struct gui2_window gui2_window_t;
typedef struct gui2_widget gui2_widget_t;
typedef struct gui2_event gui2_event_t;
typedef struct gui2_rect gui2_rect_t;
typedef struct gui2_color gui2_color_t;

// === Core Types ===

// Rectangle
struct gui2_rect {
    int32_t x, y;
    uint32_t width, height;
};

// Color (RGBA)
struct gui2_color {
    uint8_t r, g, b, a;
};

// Event types
typedef enum {
    GUI2_EVENT_NONE = 0,
    GUI2_EVENT_MOUSE_MOVE,
    GUI2_EVENT_MOUSE_DOWN,
    GUI2_EVENT_MOUSE_UP,
    GUI2_EVENT_KEY_DOWN,
    GUI2_EVENT_KEY_UP,
    GUI2_EVENT_WINDOW_CLOSE,
    GUI2_EVENT_WINDOW_RESIZE,
    GUI2_EVENT_PAINT,
    GUI2_EVENT_FOCUS_IN,
    GUI2_EVENT_FOCUS_OUT
} gui2_event_type_t;

// Event structure
struct gui2_event {
    gui2_event_type_t type;
    gui2_window_t* target_window;
    gui2_widget_t* target_widget;
    
    union {
        struct {
            int32_t x, y;
            uint32_t button; // 0=left, 1=right, 2=middle
        } mouse;
        
        struct {
            uint32_t keycode;
            uint32_t modifiers;
            char character;
        } key;
        
        struct {
            uint32_t new_width, new_height;
        } resize;
        
        struct {
            gui2_rect_t area;
        } paint;
    } data;
};

// Widget types
typedef enum {
    GUI2_WIDGET_CONTAINER = 0,
    GUI2_WIDGET_BUTTON,
    GUI2_WIDGET_LABEL,
    GUI2_WIDGET_TEXTBOX,
    GUI2_WIDGET_PANEL,
    GUI2_WIDGET_SCROLLVIEW,
    GUI2_WIDGET_MENUBAR,
    GUI2_WIDGET_MENU,
    GUI2_WIDGET_CUSTOM
} gui2_widget_type_t;

// Widget state flags
typedef enum {
    GUI2_WIDGET_VISIBLE = (1 << 0),
    GUI2_WIDGET_ENABLED = (1 << 1),
    GUI2_WIDGET_FOCUSED = (1 << 2),
    GUI2_WIDGET_HOVERED = (1 << 3),
    GUI2_WIDGET_PRESSED = (1 << 4),
    GUI2_WIDGET_SELECTED = (1 << 5)
} gui2_widget_flags_t;

// Event handler function pointer
typedef void (*gui2_event_handler_t)(gui2_widget_t* widget, gui2_event_t* event);

// Widget structure
struct gui2_widget {
    // Core properties
    gui2_widget_type_t type;
    uint32_t id;
    uint32_t flags; // gui2_widget_flags_t
    
    // Layout
    gui2_rect_t rect;
    gui2_rect_t content_rect; // rect minus padding/borders
    
    // Hierarchy
    gui2_widget_t* parent;
    gui2_widget_t* first_child;
    gui2_widget_t* next_sibling;
    
    // Properties
    char* text;
    gui2_color_t bg_color;
    gui2_color_t fg_color;
    gui2_color_t border_color;
    uint32_t border_width;
    uint32_t padding[4]; // top, right, bottom, left
    
    // Event handling
    gui2_event_handler_t event_handler;
    void* user_data;
    
    // Type-specific data
    void* widget_data;
};

// Window structure
struct gui2_window {
    uint32_t id;
    char* title;
    gui2_rect_t rect;
    uint32_t flags;
    
    // Window properties
    bool resizable;
    bool minimizable;
    bool closable;
    bool modal;
    
    // Widget tree
    gui2_widget_t* root_widget;
    gui2_widget_t* focused_widget;
    
    // Rendering
    uint32_t* framebuffer;
    uint32_t fb_width, fb_height;
    bool needs_redraw;
    
    // Event handling
    gui2_event_handler_t event_handler;
    void* user_data;
    
    // Window management
    gui2_window_t* next;
};

// Main GUI context
struct gui2_context {
    // Window management
    gui2_window_t* windows;
    gui2_window_t* active_window;
    uint32_t next_window_id;
    uint32_t next_widget_id;
    
    // Display
    uint32_t screen_width, screen_height;
    uint32_t* screen_buffer;    // Front buffer (actual framebuffer)
    uint32_t* back_buffer;      // Back buffer for double buffering
    
    // Input state
    int32_t mouse_x, mouse_y;
    uint32_t mouse_buttons;
    gui2_widget_t* hovered_widget;
    gui2_widget_t* focused_widget;
    
    // Cursor
    bool show_cursor;
    
    // Event queue
    gui2_event_t event_queue[256];
    uint32_t event_queue_head;
    uint32_t event_queue_tail;
    uint32_t event_queue_count;
    
    // Theme
    gui2_color_t theme_bg;
    gui2_color_t theme_fg;
    gui2_color_t theme_accent;
    gui2_color_t theme_border;
};

// === Core API ===

// Context management
gui2_context_t* gui2_create_context(uint32_t screen_width, uint32_t screen_height, uint32_t* screen_buffer);
void gui2_destroy_context(gui2_context_t* ctx);
void gui2_update(gui2_context_t* ctx);
void gui2_render(gui2_context_t* ctx);

// Window management
gui2_window_t* gui2_create_window(gui2_context_t* ctx, const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height);
void gui2_destroy_window(gui2_context_t* ctx, gui2_window_t* window);
void gui2_show_window(gui2_window_t* window);
void gui2_hide_window(gui2_window_t* window);
void gui2_focus_window(gui2_context_t* ctx, gui2_window_t* window);

// Widget management
gui2_widget_t* gui2_create_widget(gui2_widget_type_t type, gui2_widget_t* parent);
void gui2_destroy_widget(gui2_widget_t* widget);
void gui2_add_child(gui2_widget_t* parent, gui2_widget_t* child);
void gui2_remove_child(gui2_widget_t* parent, gui2_widget_t* child);

// Widget properties
void gui2_set_rect(gui2_widget_t* widget, int32_t x, int32_t y, uint32_t width, uint32_t height);
void gui2_set_text(gui2_widget_t* widget, const char* text);
void gui2_set_colors(gui2_widget_t* widget, gui2_color_t bg, gui2_color_t fg);
void gui2_set_visible(gui2_widget_t* widget, bool visible);
void gui2_set_enabled(gui2_widget_t* widget, bool enabled);

// Event handling
void gui2_post_event(gui2_context_t* ctx, gui2_event_t* event);
bool gui2_poll_event(gui2_context_t* ctx, gui2_event_t* event);
void gui2_set_event_handler(gui2_widget_t* widget, gui2_event_handler_t handler);

// Input handling
void gui2_mouse_move(gui2_context_t* ctx, int32_t x, int32_t y);
void gui2_mouse_button(gui2_context_t* ctx, uint32_t button, bool pressed);
void gui2_key_event(gui2_context_t* ctx, uint32_t keycode, char character, bool pressed);

// Utility functions
gui2_rect_t gui2_make_rect(int32_t x, int32_t y, uint32_t width, uint32_t height);
gui2_color_t gui2_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
bool gui2_rect_contains(gui2_rect_t rect, int32_t x, int32_t y);
gui2_widget_t* gui2_widget_at_point(gui2_widget_t* root, int32_t x, int32_t y);

// Built-in widgets
gui2_widget_t* gui2_create_button(gui2_widget_t* parent, const char* text);
gui2_widget_t* gui2_create_label(gui2_widget_t* parent, const char* text);
gui2_widget_t* gui2_create_panel(gui2_widget_t* parent);

#endif // GUI2_H