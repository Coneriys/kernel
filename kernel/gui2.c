#include "../include/gui2.h"
#include "../include/wm2.h"
#include "../include/memory.h"
#include "../include/video.h"
#include "../include/mouse.h"
#include "../include/keyboard.h"

static gui2_context_t* g_gui_context = NULL;

static void gui2_clear_rect(gui2_context_t* ctx, gui2_rect_t rect, gui2_color_t color) {
    if (!ctx || !ctx->back_buffer) return;
    
    uint32_t pixel_color = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
    
    for (uint32_t y = rect.y; y < rect.y + rect.height && y < ctx->screen_height; y++) {
        for (uint32_t x = rect.x; x < rect.x + rect.width && x < ctx->screen_width; x++) {
            ctx->back_buffer[y * ctx->screen_width + x] = pixel_color;
        }
    }
}

static uint32_t gui2_blend_colors(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    
    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;
    
    uint8_t fg_r = (fg >> 16) & 0xFF;
    uint8_t fg_g = (fg >> 8) & 0xFF;
    uint8_t fg_b = fg & 0xFF;
    
    uint8_t out_r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    uint8_t out_g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    uint8_t out_b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;
    
    return (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

// Simple sqrt approximation for kernel
static float sqrtf(float x) {
    if (x <= 0) return 0;
    float guess = x / 2;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2;
    }
    return guess;
}

static float gui2_distance_to_rounded_rect(float x, float y, gui2_rect_t rect, float radius) {
    // Calculate distance from point to rounded rectangle
    float rx = rect.x + radius;
    float ry = rect.y + radius;
    float rw = rect.width - 2 * radius;
    float rh = rect.height - 2 * radius;
    
    float dx = 0, dy = 0;
    
    if (x < rx) dx = rx - x;
    else if (x > rx + rw) dx = x - (rx + rw);
    
    if (y < ry) dy = ry - y;
    else if (y > ry + rh) dy = y - (ry + rh);
    
    return sqrtf(dx * dx + dy * dy) - radius;
}

// Efficient rounded rect using pre-calculated corner masks
void gui2_draw_rounded_rect(gui2_context_t* ctx, gui2_rect_t rect, gui2_color_t color, float radius) {
    if (!ctx || !ctx->back_buffer || radius <= 0) {
        gui2_clear_rect(ctx, rect, color);
        return;
    }
    
    int r = (int)radius;
    uint32_t bg_color = (ctx->theme_bg.a << 24) | (ctx->theme_bg.r << 16) | (ctx->theme_bg.g << 8) | ctx->theme_bg.b;
    uint32_t fg_color = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
    
    // Draw main rectangle
    gui2_clear_rect(ctx, rect, color);
    
    // Optimized anti-aliasing - 2x2 sampling instead of 4x4
    if (r > 0 && r < 20) {
        for (int y = 0; y < r + 1; y++) {
            for (int x = 0; x < r + 1; x++) {
                // 2x2 sub-pixel sampling for lighter anti-aliasing
                int coverage = 0;
                for (int sy = 0; sy < 2; sy++) {
                    for (int sx = 0; sx < 2; sx++) {
                        int sample_x = x * 2 + sx;
                        int sample_y = y * 2 + sy;
                        int dx = r * 2 - sample_x;
                        int dy = r * 2 - sample_y;
                        
                        if (dx * dx + dy * dy > (r * 2) * (r * 2)) {
                            coverage++;
                        }
                    }
                }
                
                if (coverage > 0) {
                    uint8_t alpha = (coverage * 255) / 4;
                    
                    // Top-left corner
                    int px = rect.x + x;
                    int py = rect.y + y;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        if (coverage == 4) {
                            ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                        } else {
                            uint32_t existing = ctx->back_buffer[py * ctx->screen_width + px];
                            ctx->back_buffer[py * ctx->screen_width + px] = gui2_blend_colors(existing, bg_color, alpha);
                        }
                    }
                    
                    // Top-right corner
                    px = rect.x + rect.width - 1 - x;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        if (coverage == 4) {
                            ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                        } else {
                            uint32_t existing = ctx->back_buffer[py * ctx->screen_width + px];
                            ctx->back_buffer[py * ctx->screen_width + px] = gui2_blend_colors(existing, bg_color, alpha);
                        }
                    }
                    
                    // Bottom-left corner
                    px = rect.x + x;
                    py = rect.y + rect.height - 1 - y;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        if (coverage == 4) {
                            ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                        } else {
                            uint32_t existing = ctx->back_buffer[py * ctx->screen_width + px];
                            ctx->back_buffer[py * ctx->screen_width + px] = gui2_blend_colors(existing, bg_color, alpha);
                        }
                    }
                    
                    // Bottom-right corner
                    px = rect.x + rect.width - 1 - x;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        if (coverage == 4) {
                            ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                        } else {
                            uint32_t existing = ctx->back_buffer[py * ctx->screen_width + px];
                            ctx->back_buffer[py * ctx->screen_width + px] = gui2_blend_colors(existing, bg_color, alpha);
                        }
                    }
                }
            }
        }
    }
}

// Optimized anti-aliased circles for traffic lights
static void gui2_draw_circle(gui2_context_t* ctx, int32_t center_x, int32_t center_y, float radius, gui2_color_t color) {
    if (!ctx || !ctx->back_buffer) return;
    
    uint32_t fg_color = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
    int r = (int)radius;
    
    // Light anti-aliasing with 2x2 sampling
    for (int y = center_y - r - 1; y <= center_y + r + 1; y++) {
        for (int x = center_x - r - 1; x <= center_x + r + 1; x++) {
            if (x >= 0 && x < (int)ctx->screen_width && y >= 0 && y < (int)ctx->screen_height) {
                
                // 2x2 sub-pixel sampling 
                int coverage = 0;
                for (int sy = 0; sy < 2; sy++) {
                    for (int sx = 0; sx < 2; sx++) {
                        int sample_x = x * 2 + sx;
                        int sample_y = y * 2 + sy;
                        int dx = sample_x - center_x * 2;
                        int dy = sample_y - center_y * 2;
                        int r2 = r * 2;
                        
                        if (dx * dx + dy * dy <= r2 * r2) {
                            coverage++;
                        }
                    }
                }
                
                if (coverage > 0) {
                    if (coverage == 4) {
                        // Fully inside
                        ctx->back_buffer[y * ctx->screen_width + x] = fg_color;
                    } else {
                        // Anti-aliased edge
                        uint8_t alpha = (coverage * 255) / 4;
                        uint32_t existing = ctx->back_buffer[y * ctx->screen_width + x];
                        ctx->back_buffer[y * ctx->screen_width + x] = gui2_blend_colors(existing, fg_color, alpha);
                    }
                }
            }
        }
    }
}

// Titlebar with only top corners rounded  
static void gui2_draw_titlebar_rounded(gui2_context_t* ctx, gui2_rect_t rect, gui2_color_t color, float radius) {
    if (!ctx || !ctx->back_buffer) return;
    
    // Draw main rectangle first
    gui2_clear_rect(ctx, rect, color);
    
    // Only round top corners if radius > 0
    if (radius > 0) {
        int r = (int)radius;
        uint32_t bg_color = (ctx->theme_bg.a << 24) | (ctx->theme_bg.r << 16) | (ctx->theme_bg.g << 8) | ctx->theme_bg.b;
        
        // Simple corner cutting - only top corners
        for (int y = 0; y < r; y++) {
            for (int x = 0; x < r; x++) {
                int dx = r - x;
                int dy = r - y;
                if (dx * dx + dy * dy > r * r) {
                    // Top-left corner
                    int px = rect.x + x;
                    int py = rect.y + y;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                    }
                    
                    // Top-right corner
                    px = rect.x + rect.width - 1 - x;
                    if (px >= 0 && px < (int)ctx->screen_width && py >= 0 && py < (int)ctx->screen_height) {
                        ctx->back_buffer[py * ctx->screen_width + px] = bg_color;
                    }
                }
            }
        }
    }
}

static void gui2_draw_border(gui2_context_t* ctx, gui2_rect_t rect, gui2_color_t color, uint32_t width) {
    if (!ctx || !ctx->back_buffer || width == 0) return;
    
    gui2_rect_t top = {rect.x, rect.y, rect.width, width};
    gui2_rect_t bottom = {rect.x, rect.y + rect.height - width, rect.width, width};
    gui2_rect_t left = {rect.x, rect.y, width, rect.height};
    gui2_rect_t right = {rect.x + rect.width - width, rect.y, width, rect.height};
    
    gui2_clear_rect(ctx, top, color);
    gui2_clear_rect(ctx, bottom, color);
    gui2_clear_rect(ctx, left, color);
    gui2_clear_rect(ctx, right, color);
}

gui2_context_t* gui2_create_context(uint32_t screen_width, uint32_t screen_height, uint32_t* screen_buffer) {
    gui2_context_t* ctx = (gui2_context_t*)kmalloc(sizeof(gui2_context_t));
    if (!ctx) return NULL;
    
    ctx->windows = NULL;
    ctx->active_window = NULL;
    ctx->next_window_id = 1;
    ctx->next_widget_id = 1;
    
    ctx->screen_width = screen_width;
    ctx->screen_height = screen_height;
    ctx->screen_buffer = screen_buffer;
    
    // Allocate back buffer
    ctx->back_buffer = (uint32_t*)kmalloc(screen_width * screen_height * sizeof(uint32_t));
    if (!ctx->back_buffer) {
        kfree(ctx);
        return NULL;
    }
    
    ctx->mouse_x = screen_width / 2;
    ctx->mouse_y = screen_height / 2;
    ctx->mouse_buttons = 0;
    ctx->hovered_widget = NULL;
    ctx->focused_widget = NULL;
    ctx->show_cursor = true;
    
    for (int i = 0; i < 256; i++) {
        ctx->event_queue[i].type = GUI2_EVENT_NONE;
    }
    ctx->event_queue_head = 0;
    ctx->event_queue_tail = 0;
    ctx->event_queue_count = 0;
    
    ctx->theme_bg = gui2_make_color(45, 45, 48, 255);
    ctx->theme_fg = gui2_make_color(255, 255, 255, 255);
    ctx->theme_accent = gui2_make_color(0, 122, 255, 255);
    ctx->theme_border = gui2_make_color(76, 76, 76, 255);
    
    g_gui_context = ctx;
    return ctx;
}

void gui2_destroy_context(gui2_context_t* ctx) {
    if (!ctx) return;
    
    gui2_window_t* window = ctx->windows;
    while (window) {
        gui2_window_t* next = window->next;
        gui2_destroy_window(ctx, window);
        window = next;
    }
    
    if (ctx->back_buffer) {
        kfree(ctx->back_buffer);
    }
    
    kfree(ctx);
    if (g_gui_context == ctx) {
        g_gui_context = NULL;
    }
}

gui2_window_t* gui2_create_window(gui2_context_t* ctx, const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!ctx) return NULL;
    
    gui2_window_t* window = (gui2_window_t*)kmalloc(sizeof(gui2_window_t));
    if (!window) return NULL;
    
    window->id = ctx->next_window_id++;
    
    size_t title_len = 0;
    if (title) {
        const char* p = title;
        while (*p++) title_len++;
        
        window->title = (char*)kmalloc(title_len + 1);
        if (window->title) {
            char* dst = window->title;
            const char* src = title;
            while ((*dst++ = *src++));
        }
    } else {
        window->title = NULL;
    }
    
    window->rect = gui2_make_rect(x, y, width, height);
    window->flags = 0;
    
    window->resizable = true;
    window->minimizable = true;
    window->closable = true;
    window->modal = false;
    
    window->root_widget = gui2_create_widget(GUI2_WIDGET_CONTAINER, NULL);
    if (window->root_widget) {
        gui2_set_rect(window->root_widget, 0, 30, width, height - 30);
        window->root_widget->bg_color = ctx->theme_bg;
        window->root_widget->flags |= GUI2_WIDGET_VISIBLE;
    }
    window->focused_widget = NULL;
    
    window->framebuffer = NULL;
    window->fb_width = width;
    window->fb_height = height;
    window->needs_redraw = true;
    
    window->event_handler = NULL;
    window->user_data = NULL;
    
    window->next = ctx->windows;
    ctx->windows = window;
    
    return window;
}

void gui2_destroy_window(gui2_context_t* ctx, gui2_window_t* window) {
    if (!ctx || !window) return;
    
    if (ctx->active_window == window) {
        ctx->active_window = NULL;
    }
    
    gui2_window_t** current = &ctx->windows;
    while (*current) {
        if (*current == window) {
            *current = window->next;
            break;
        }
        current = &(*current)->next;
    }
    
    if (window->root_widget) {
        gui2_destroy_widget(window->root_widget);
    }
    
    if (window->title) {
        kfree(window->title);
    }
    
    kfree(window);
}

gui2_widget_t* gui2_create_widget(gui2_widget_type_t type, gui2_widget_t* parent) {
    gui2_widget_t* widget = (gui2_widget_t*)kmalloc(sizeof(gui2_widget_t));
    if (!widget) return NULL;
    
    widget->type = type;
    widget->id = g_gui_context ? g_gui_context->next_widget_id++ : 1;
    widget->flags = GUI2_WIDGET_ENABLED;
    
    widget->rect = gui2_make_rect(0, 0, 100, 30);
    widget->content_rect = widget->rect;
    
    widget->parent = parent;
    widget->first_child = NULL;
    widget->next_sibling = NULL;
    
    widget->text = NULL;
    widget->bg_color = g_gui_context ? g_gui_context->theme_bg : gui2_make_color(45, 45, 48, 255);
    widget->fg_color = g_gui_context ? g_gui_context->theme_fg : gui2_make_color(255, 255, 255, 255);
    widget->border_color = g_gui_context ? g_gui_context->theme_border : gui2_make_color(76, 76, 76, 255);
    widget->border_width = 1;
    
    widget->padding[0] = widget->padding[1] = widget->padding[2] = widget->padding[3] = 4;
    
    widget->event_handler = NULL;
    widget->user_data = NULL;
    widget->widget_data = NULL;
    
    if (parent) {
        gui2_add_child(parent, widget);
    }
    
    return widget;
}

void gui2_destroy_widget(gui2_widget_t* widget) {
    if (!widget) return;
    
    gui2_widget_t* child = widget->first_child;
    while (child) {
        gui2_widget_t* next = child->next_sibling;
        gui2_destroy_widget(child);
        child = next;
    }
    
    if (widget->parent) {
        gui2_remove_child(widget->parent, widget);
    }
    
    if (widget->text) {
        kfree(widget->text);
    }
    
    if (widget->widget_data) {
        kfree(widget->widget_data);
    }
    
    kfree(widget);
}

void gui2_add_child(gui2_widget_t* parent, gui2_widget_t* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    child->next_sibling = parent->first_child;
    parent->first_child = child;
}

void gui2_remove_child(gui2_widget_t* parent, gui2_widget_t* child) {
    if (!parent || !child) return;
    
    if (parent->first_child == child) {
        parent->first_child = child->next_sibling;
    } else {
        gui2_widget_t* current = parent->first_child;
        while (current && current->next_sibling != child) {
            current = current->next_sibling;
        }
        if (current) {
            current->next_sibling = child->next_sibling;
        }
    }
    
    child->parent = NULL;
    child->next_sibling = NULL;
}

void gui2_set_rect(gui2_widget_t* widget, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!widget) return;
    
    widget->rect = gui2_make_rect(x, y, width, height);
    
    widget->content_rect.x = x + widget->border_width + widget->padding[3];
    widget->content_rect.y = y + widget->border_width + widget->padding[0];
    widget->content_rect.width = width - 2 * widget->border_width - widget->padding[1] - widget->padding[3];
    widget->content_rect.height = height - 2 * widget->border_width - widget->padding[0] - widget->padding[2];
}

void gui2_set_text(gui2_widget_t* widget, const char* text) {
    if (!widget) return;
    
    if (widget->text) {
        kfree(widget->text);
        widget->text = NULL;
    }
    
    if (text) {
        size_t len = 0;
        const char* p = text;
        while (*p++) len++;
        
        widget->text = (char*)kmalloc(len + 1);
        if (widget->text) {
            char* dst = widget->text;
            const char* src = text;
            while ((*dst++ = *src++));
        }
    }
}

void gui2_set_colors(gui2_widget_t* widget, gui2_color_t bg, gui2_color_t fg) {
    if (!widget) return;
    
    widget->bg_color = bg;
    widget->fg_color = fg;
}

void gui2_set_visible(gui2_widget_t* widget, bool visible) {
    if (!widget) return;
    
    if (visible) {
        widget->flags |= GUI2_WIDGET_VISIBLE;
    } else {
        widget->flags &= ~GUI2_WIDGET_VISIBLE;
    }
}

void gui2_set_enabled(gui2_widget_t* widget, bool enabled) {
    if (!widget) return;
    
    if (enabled) {
        widget->flags |= GUI2_WIDGET_ENABLED;
    } else {
        widget->flags &= ~GUI2_WIDGET_ENABLED;
    }
}

static void gui2_render_widget(gui2_context_t* ctx, gui2_widget_t* widget, int32_t offset_x, int32_t offset_y) {
    if (!ctx || !widget || !(widget->flags & GUI2_WIDGET_VISIBLE)) return;
    
    gui2_rect_t screen_rect = {
        widget->rect.x + offset_x,
        widget->rect.y + offset_y,
        widget->rect.width,
        widget->rect.height
    };
    
    // Special rendering for different widget types
    float radius = 0.0f;
    bool is_dock_icon = false;
    
    if (widget->type == GUI2_WIDGET_BUTTON) {
        // Check if this is a dock icon (has parent panel with specific size)
        if (widget->parent && widget->parent->type == GUI2_WIDGET_PANEL && 
            widget->rect.width == 44 && widget->rect.height == 44) {
            radius = 10.0f; // More rounded for dock icons
            is_dock_icon = true;
        } else {
            radius = 6.0f;
        }
    } else if (widget->type == GUI2_WIDGET_PANEL) {
        // Check if this is taskbar (floating dock)
        if (widget->rect.height == 60 && widget->rect.width == 400) {
            radius = 16.0f; // Very rounded for dock
        } else {
            radius = 4.0f;
        }
    }
    
    if (radius > 0) {
        gui2_draw_rounded_rect(ctx, screen_rect, widget->bg_color, radius);
        
        // Add special dock icon effects
        if (is_dock_icon) {
            // Add subtle inner shadow/highlight
            gui2_color_t highlight = gui2_make_color(255, 255, 255, 30);
            gui2_rect_t highlight_rect = {
                screen_rect.x + 1, 
                screen_rect.y + 1, 
                screen_rect.width - 2, 
                screen_rect.height / 3
            };
            gui2_draw_rounded_rect(ctx, highlight_rect, highlight, radius - 2);
        }
    } else {
        gui2_clear_rect(ctx, screen_rect, widget->bg_color);
    }
    
    if (widget->border_width > 0) {
        gui2_draw_border(ctx, screen_rect, widget->border_color, widget->border_width);
    }
    
    gui2_widget_t* child = widget->first_child;
    while (child) {
        gui2_render_widget(ctx, child, screen_rect.x, screen_rect.y);
        child = child->next_sibling;
    }
}

static void gui2_render_window_titlebar(gui2_context_t* ctx, gui2_window_t* window) {
    if (!ctx || !window) return;
    
    gui2_rect_t titlebar_rect = {
        window->rect.x,
        window->rect.y,
        window->rect.width,
        30
    };
    
    gui2_color_t titlebar_color = (ctx->active_window == window) ?
        gui2_make_color(70, 70, 75, 255) :
        gui2_make_color(50, 50, 55, 255);
    
    // Draw titlebar with only top corners rounded
    gui2_draw_titlebar_rounded(ctx, titlebar_rect, titlebar_color, 10.0f);
    
    if (window->closable) {
        gui2_draw_circle(ctx, window->rect.x + 17, window->rect.y + 15, 7.0f, gui2_make_color(255, 95, 86, 255));
    }
    
    if (window->minimizable) {
        gui2_draw_circle(ctx, window->rect.x + 37, window->rect.y + 15, 7.0f, gui2_make_color(255, 189, 46, 255));
    }
    
    if (window->resizable) {
        gui2_draw_circle(ctx, window->rect.x + 57, window->rect.y + 15, 7.0f, gui2_make_color(39, 201, 63, 255));
    }
}

static void gui2_draw_cursor(gui2_context_t* ctx) {
    if (!ctx || !ctx->back_buffer || !ctx->show_cursor) return;
    
    int32_t x = ctx->mouse_x;
    int32_t y = ctx->mouse_y;
    
    // Simple arrow cursor (white with black outline)
    uint32_t white = 0xFFFFFFFF;
    uint32_t black = 0xFF000000;
    
    // Cursor shape (11x16 pixels)
    static const int cursor_data[16][11] = {
        {1,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0},
        {1,2,2,2,2,2,2,2,2,1,0},
        {1,2,2,2,2,2,1,1,1,1,1},
        {1,2,2,1,2,2,1,0,0,0,0},
        {1,2,1,0,1,2,2,1,0,0,0},
        {1,1,0,0,1,2,2,1,0,0,0},
        {1,0,0,0,0,1,2,2,1,0,0},
        {0,0,0,0,0,1,1,1,1,0,0}
    };
    
    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 11; cx++) {
            int px = x + cx;
            int py = y + cy;
            
            if (px >= 0 && px < (int32_t)ctx->screen_width && 
                py >= 0 && py < (int32_t)ctx->screen_height) {
                
                uint32_t color = 0;
                switch(cursor_data[cy][cx]) {
                    case 1: color = black; break;  // Outline
                    case 2: color = white; break;  // Fill
                    default: continue;             // Transparent
                }
                
                ctx->back_buffer[py * ctx->screen_width + px] = color;
            }
        }
    }
}

static void gui2_swap_buffers(gui2_context_t* ctx) {
    if (!ctx || !ctx->screen_buffer || !ctx->back_buffer) return;
    
    // Copy back buffer to front buffer (screen)
    for (uint32_t i = 0; i < ctx->screen_width * ctx->screen_height; i++) {
        ctx->screen_buffer[i] = ctx->back_buffer[i];
    }
}

void gui2_render(gui2_context_t* ctx) {
    if (!ctx || !ctx->back_buffer) return;
    
    // Clear back buffer
    gui2_clear_rect(ctx, gui2_make_rect(0, 0, ctx->screen_width, ctx->screen_height), ctx->theme_bg);
    
    // Render desktop widgets first (like taskbar)
    extern wm2_context_t* global_wm;
    if (global_wm && global_wm->desktop && (global_wm->desktop->flags & GUI2_WIDGET_VISIBLE)) {
        gui2_render_widget(ctx, global_wm->desktop, 0, 0);
    }
    
    // Render all windows to back buffer
    gui2_window_t* window = ctx->windows;
    while (window) {
        if (window->flags & GUI2_WIDGET_VISIBLE) {
            // Draw window background with nice rounded corners
            gui2_draw_rounded_rect(ctx, window->rect, gui2_make_color(45, 45, 50, 255), 8.0f);
            
            // Draw titlebar
            gui2_render_window_titlebar(ctx, window);
            
            // Draw window content
            if (window->root_widget) {
                gui2_render_widget(ctx, window->root_widget, window->rect.x, window->rect.y);
            }
        }
        
        window = window->next;
    }
    
    // Draw cursor on top of everything
    gui2_draw_cursor(ctx);
    
    // Swap buffers - copy back buffer to screen
    gui2_swap_buffers(ctx);
}

void gui2_post_event(gui2_context_t* ctx, gui2_event_t* event) {
    if (!ctx || !event) return;
    
    if (ctx->event_queue_count >= 256) return;
    
    ctx->event_queue[ctx->event_queue_tail] = *event;
    ctx->event_queue_tail = (ctx->event_queue_tail + 1) % 256;
    ctx->event_queue_count++;
}

bool gui2_poll_event(gui2_context_t* ctx, gui2_event_t* event) {
    if (!ctx || !event || ctx->event_queue_count == 0) return false;
    
    *event = ctx->event_queue[ctx->event_queue_head];
    ctx->event_queue_head = (ctx->event_queue_head + 1) % 256;
    ctx->event_queue_count--;
    return true;
}

void gui2_set_event_handler(gui2_widget_t* widget, gui2_event_handler_t handler) {
    if (!widget) return;
    widget->event_handler = handler;
}

void gui2_mouse_move(gui2_context_t* ctx, int32_t x, int32_t y) {
    if (!ctx) return;
    
    ctx->mouse_x = x;
    ctx->mouse_y = y;
    
    gui2_widget_t* new_hovered = NULL;
    gui2_window_t* window = ctx->windows;
    
    while (window) {
        if (window->flags & GUI2_WIDGET_VISIBLE && gui2_rect_contains(window->rect, x, y)) {
            if (window->root_widget) {
                new_hovered = gui2_widget_at_point(window->root_widget, 
                    x - window->rect.x, y - window->rect.y - 30);
            }
            break;
        }
        window = window->next;
    }
    
    if (ctx->hovered_widget != new_hovered) {
        if (ctx->hovered_widget) {
            ctx->hovered_widget->flags &= ~GUI2_WIDGET_HOVERED;
        }
        
        ctx->hovered_widget = new_hovered;
        
        if (ctx->hovered_widget) {
            ctx->hovered_widget->flags |= GUI2_WIDGET_HOVERED;
        }
    }
    
    gui2_event_t event = {0};
    event.type = GUI2_EVENT_MOUSE_MOVE;
    event.target_window = window;
    event.target_widget = new_hovered;
    event.data.mouse.x = x;
    event.data.mouse.y = y;
    
    gui2_post_event(ctx, &event);
}

void gui2_mouse_button(gui2_context_t* ctx, uint32_t button, bool pressed) {
    if (!ctx) return;
    
    uint32_t button_mask = (1 << button);
    
    if (pressed) {
        ctx->mouse_buttons |= button_mask;
    } else {
        ctx->mouse_buttons &= ~button_mask;
    }
    
    gui2_event_t event = {0};
    event.type = pressed ? GUI2_EVENT_MOUSE_DOWN : GUI2_EVENT_MOUSE_UP;
    event.target_widget = ctx->hovered_widget;
    event.data.mouse.x = ctx->mouse_x;
    event.data.mouse.y = ctx->mouse_y;
    event.data.mouse.button = button;
    
    if (ctx->hovered_widget) {
        gui2_window_t* window = ctx->windows;
        while (window) {
            if (window->root_widget && 
                gui2_widget_at_point(window->root_widget, ctx->mouse_x - window->rect.x, ctx->mouse_y - window->rect.y - 30)) {
                event.target_window = window;
                break;
            }
            window = window->next;
        }
    }
    
    if (pressed && ctx->hovered_widget) {
        ctx->hovered_widget->flags |= GUI2_WIDGET_PRESSED;
        ctx->focused_widget = ctx->hovered_widget;
        ctx->hovered_widget->flags |= GUI2_WIDGET_FOCUSED;
    } else if (!pressed && ctx->hovered_widget) {
        ctx->hovered_widget->flags &= ~GUI2_WIDGET_PRESSED;
    }
    
    gui2_post_event(ctx, &event);
}

void gui2_key_event(gui2_context_t* ctx, uint32_t keycode, char character, bool pressed) {
    if (!ctx) return;
    
    gui2_event_t event = {0};
    event.type = pressed ? GUI2_EVENT_KEY_DOWN : GUI2_EVENT_KEY_UP;
    event.target_widget = ctx->focused_widget;
    event.data.key.keycode = keycode;
    event.data.key.character = character;
    event.data.key.modifiers = 0;
    
    gui2_post_event(ctx, &event);
}

static void gui2_dispatch_event(gui2_context_t* ctx, gui2_event_t* event) {
    if (!ctx || !event) return;
    
    if (event->target_widget && event->target_widget->event_handler) {
        event->target_widget->event_handler(event->target_widget, event);
    }
    
    if (event->target_window && event->target_window->event_handler) {
        event->target_window->event_handler(event->target_widget, event);
    }
}

void gui2_update(gui2_context_t* ctx) {
    if (!ctx) return;
    
    gui2_event_t event;
    while (gui2_poll_event(ctx, &event)) {
        gui2_dispatch_event(ctx, &event);
    }
    
    gui2_window_t* window = ctx->windows;
    while (window) {
        if (window->needs_redraw) {
            window->needs_redraw = false;
        }
        window = window->next;
    }
}

gui2_rect_t gui2_make_rect(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    gui2_rect_t rect = {x, y, width, height};
    return rect;
}

gui2_color_t gui2_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    gui2_color_t color = {r, g, b, a};
    return color;
}

bool gui2_rect_contains(gui2_rect_t rect, int32_t x, int32_t y) {
    return (x >= rect.x && x < rect.x + (int32_t)rect.width &&
            y >= rect.y && y < rect.y + (int32_t)rect.height);
}

gui2_widget_t* gui2_widget_at_point(gui2_widget_t* root, int32_t x, int32_t y) {
    if (!root || !(root->flags & GUI2_WIDGET_VISIBLE)) return NULL;
    
    if (!gui2_rect_contains(root->rect, x, y)) return NULL;
    
    gui2_widget_t* child = root->first_child;
    while (child) {
        gui2_widget_t* found = gui2_widget_at_point(child, x - root->rect.x, y - root->rect.y);
        if (found) return found;
        child = child->next_sibling;
    }
    
    return root;
}

void gui2_show_window(gui2_window_t* window) {
    if (!window) return;
    window->flags |= GUI2_WIDGET_VISIBLE;
    window->needs_redraw = true;
}

void gui2_hide_window(gui2_window_t* window) {
    if (!window) return;
    window->flags &= ~GUI2_WIDGET_VISIBLE;
}

void gui2_focus_window(gui2_context_t* ctx, gui2_window_t* window) {
    if (!ctx || !window) return;
    ctx->active_window = window;
}

gui2_widget_t* gui2_create_button(gui2_widget_t* parent, const char* text) {
    gui2_widget_t* button = gui2_create_widget(GUI2_WIDGET_BUTTON, parent);
    if (button) {
        gui2_set_text(button, text);
        button->bg_color = g_gui_context ? g_gui_context->theme_accent : gui2_make_color(0, 122, 255, 255);
        button->flags |= GUI2_WIDGET_VISIBLE;
    }
    return button;
}

gui2_widget_t* gui2_create_label(gui2_widget_t* parent, const char* text) {
    gui2_widget_t* label = gui2_create_widget(GUI2_WIDGET_LABEL, parent);
    if (label) {
        gui2_set_text(label, text);
        label->bg_color = gui2_make_color(0, 0, 0, 0);
        label->border_width = 0;
        label->flags |= GUI2_WIDGET_VISIBLE;
    }
    return label;
}

gui2_widget_t* gui2_create_panel(gui2_widget_t* parent) {
    gui2_widget_t* panel = gui2_create_widget(GUI2_WIDGET_PANEL, parent);
    if (panel) {
        panel->flags |= GUI2_WIDGET_VISIBLE;
    }
    return panel;
}

int gui2_main_loop(void) {
    extern video_driver_t* video_get_driver(void);
    extern wm2_context_t* global_wm;
    
    video_driver_t* driver = video_get_driver();
    if (!driver || !driver->framebuffer) {
        return 1;
    }
    
    global_wm = wm2_create(driver->width, driver->height, (uint32_t*)driver->framebuffer);
    if (!global_wm) {
        return 1;
    }
    
    // Create a demo window
    gui2_window_t* demo_window = wm2_create_window(global_wm, "Demo Window", 100, 100, 400, 300);
    if (demo_window) {
        // Add some demo widgets
        gui2_widget_t* label = gui2_create_label(demo_window->root_widget, "Hello GUI2!");
        if (label) {
            gui2_set_rect(label, 20, 20, 200, 30);
        }
        
        gui2_widget_t* button = gui2_create_button(demo_window->root_widget, "Click Me");
        if (button) {
            gui2_set_rect(button, 20, 60, 100, 30);
        }
    }
    
    // Create another demo window
    gui2_window_t* demo_window2 = wm2_create_window(global_wm, "Second Window", 200, 150, 300, 250);
    if (demo_window2) {
        gui2_widget_t* panel = gui2_create_panel(demo_window2->root_widget);
        if (panel) {
            gui2_set_rect(panel, 10, 10, 280, 200);
            panel->bg_color = gui2_make_color(60, 60, 65, 255);
            
            gui2_widget_t* label2 = gui2_create_label(panel, "This is a second window");
            if (label2) {
                gui2_set_rect(label2, 10, 10, 260, 30);
            }
        }
    }
    
    extern mouse_state_t* mouse_get_state(void);
    
    // Main GUI loop
    while (1) {
        // Handle mouse input
        mouse_state_t* mouse = mouse_get_state();
        if (mouse) {
            static int16_t last_mouse_x = -1, last_mouse_y = -1;
            static uint8_t last_buttons = 0;
            
            // Mouse movement
            if (mouse->x != last_mouse_x || mouse->y != last_mouse_y) {
                wm2_handle_mouse_move(global_wm, mouse->x, mouse->y);
                last_mouse_x = mouse->x;
                last_mouse_y = mouse->y;
            }
            
            // Mouse buttons
            uint8_t button_changes = mouse->buttons ^ last_buttons;
            if (button_changes) {
                for (int i = 0; i < 3; i++) {
                    uint8_t button_mask = (1 << i);
                    if (button_changes & button_mask) {
                        bool pressed = (mouse->buttons & button_mask) ? true : false;
                        wm2_handle_mouse_button(global_wm, i, pressed);
                    }
                }
                last_buttons = mouse->buttons;
            }
        }
        
        // Handle keyboard input
        if (keyboard_available()) {
            char c = keyboard_getchar();
            
            if (c == 27) { // Escape key
                break; // Exit GUI
            }
            
            wm2_handle_key(global_wm, c, c, true);
        }
        
        // Update and render
        wm2_update(global_wm);
        wm2_render(global_wm);
        
        // Simple frame limiting
        for (volatile int i = 0; i < 10000; i++);
    }
    
    // Cleanup
    wm2_destroy(global_wm);
    global_wm = NULL;
    
    // Return to text mode - do NOT restart system
    extern void terminal_initialize(void);
    video_set_mode(VIDEO_MODE_TEXT);
    terminal_initialize();
    
    extern void terminal_writestring(const char* data);
    terminal_writestring("GUI2 system exited\n");
    terminal_writestring("System halted. Reboot to restart.\n");
    
    // Halt forever instead of returning (which would restart)
    while(1) {
        asm("hlt");
    }
}