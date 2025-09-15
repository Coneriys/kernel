# byteOS SDK Developer Guide

## Overview

The MyKernel SDK provides a comprehensive set of APIs for developing modern, macOS-style applications for the MyKernel operating system. The SDK offers high-level abstractions for GUI development, system integration, and application lifecycle management.

## Features

- **Modern GUI Framework**: Create beautiful, macOS-style applications with minimal code
- **Rich Controls**: Buttons, text fields, labels, progress bars, list views, and more
- **Advanced Drawing**: Hardware-accelerated graphics with gradients, shadows, and anti-aliasing
- **Event Handling**: Mouse, keyboard, and window events with callback-based architecture
- **System Integration**: Notifications, clipboard, system information, and preferences
- **Color System**: Complete macOS color palette with color manipulation utilities

## Quick Start

### Basic Application Structure

```c
#include "sdk.h"

// Application state
typedef struct {
    gui_window_t* main_window;
} my_app_t;

// Paint callback
void on_paint(gui_window_t* window) {
    sdk_draw_filled_rect(window, 0, 0, 
                        window->content_rect.width, window->content_rect.height,
                        SDK_COLOR_BACKGROUND);
    sdk_draw_text(window, "Hello World!", 20, 20, SDK_COLOR_TEXT_PRIMARY);
}

// Main entry point
int my_app_main(int argc, char* argv[]) {
    my_app_t app = {0};
    
    // Create window
    app.main_window = sdk_window_create("My App", 100, 100, 400, 300);
    sdk_window_set_style(app.main_window, 
                        SDK_WINDOW_STYLE_TITLEBAR | SDK_WINDOW_STYLE_CLOSABLE);
    
    // Set callbacks
    sdk_window_set_paint_callback(app.main_window, on_paint);
    
    // Show and run
    sdk_window_show(app.main_window);
    sdk_app_run();
    
    return 0;
}
```

## API Reference

### Window Management

#### Creating Windows
```c
gui_window_t* sdk_window_create(const char* title, uint32_t x, uint32_t y, 
                               uint32_t width, uint32_t height);
void sdk_window_destroy(gui_window_t* window);
void sdk_window_show(gui_window_t* window);
void sdk_window_hide(gui_window_t* window);
```

#### Window Styling
```c
// Style flags
#define SDK_WINDOW_STYLE_TITLEBAR       (1 << 0)
#define SDK_WINDOW_STYLE_CLOSABLE       (1 << 1) 
#define SDK_WINDOW_STYLE_MINIMIZABLE    (1 << 2)
#define SDK_WINDOW_STYLE_RESIZABLE      (1 << 3)
#define SDK_WINDOW_STYLE_UTILITY        (1 << 4)
#define SDK_WINDOW_STYLE_BORDERLESS     (1 << 5)

void sdk_window_set_style(gui_window_t* window, uint32_t style_flags);
void sdk_window_center_on_screen(gui_window_t* window);
void sdk_window_set_alpha(gui_window_t* window, float alpha);
```

### Drawing API

#### Basic Drawing
```c
void sdk_draw_pixel(gui_window_t* window, uint32_t x, uint32_t y, color32_t color);
void sdk_draw_rect(gui_window_t* window, uint32_t x, uint32_t y, 
                   uint32_t width, uint32_t height, color32_t color);
void sdk_draw_filled_rect(gui_window_t* window, uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height, color32_t color);
void sdk_draw_text(gui_window_t* window, const char* text, uint32_t x, uint32_t y, color32_t color);
void sdk_draw_line(gui_window_t* window, uint32_t x1, uint32_t y1, 
                   uint32_t x2, uint32_t y2, color32_t color);
```

#### Advanced Drawing
```c
void sdk_draw_rounded_rect(gui_window_t* window, uint32_t x, uint32_t y, 
                          uint32_t width, uint32_t height, uint32_t radius, color32_t color);
void sdk_draw_shadow(gui_window_t* window, uint32_t x, uint32_t y, 
                    uint32_t width, uint32_t height, uint32_t blur, color32_t color);
void sdk_draw_gradient(gui_window_t* window, uint32_t x, uint32_t y, 
                      uint32_t width, uint32_t height, 
                      color32_t color1, color32_t color2, uint8_t vertical);
void sdk_draw_circle(gui_window_t* window, uint32_t cx, uint32_t cy, 
                    uint32_t radius, color32_t color);
```

### Color System

#### System Colors
```c
#define SDK_COLOR_SYSTEM_BLUE       // macOS system blue
#define SDK_COLOR_SYSTEM_RED        // macOS system red
#define SDK_COLOR_SYSTEM_GREEN      // macOS system green
#define SDK_COLOR_SYSTEM_YELLOW     // macOS system yellow
#define SDK_COLOR_SYSTEM_ORANGE     // macOS system orange
#define SDK_COLOR_SYSTEM_PURPLE     // macOS system purple
#define SDK_COLOR_SYSTEM_PINK       // macOS system pink
#define SDK_COLOR_SYSTEM_GRAY       // macOS system gray

#define SDK_COLOR_TEXT_PRIMARY      // Primary text color
#define SDK_COLOR_TEXT_SECONDARY    // Secondary text color
#define SDK_COLOR_BACKGROUND        // Window background
#define SDK_COLOR_CONTROL           // Control background
```

#### Color Utilities
```c
color32_t sdk_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color32_t sdk_color_hsv(float h, float s, float v, uint8_t a);
color32_t sdk_color_blend(color32_t fg, color32_t bg, float alpha);
color32_t sdk_color_darker(color32_t color, float amount);
color32_t sdk_color_lighter(color32_t color, float amount);
```

### Event Handling

#### Event Callbacks
```c
void sdk_window_set_paint_callback(gui_window_t* window, void (*callback)(gui_window_t*));
void sdk_window_set_mouse_callback(gui_window_t* window, void (*callback)(gui_window_t*, int, int, uint8_t));
void sdk_window_set_key_callback(gui_window_t* window, void (*callback)(gui_window_t*, char));
```

#### Application Lifecycle
```c
typedef struct {
    void (*app_did_launch)(void);
    void (*app_will_terminate)(void);
    void (*app_did_become_active)(void);
    void (*app_will_resign_active)(void);
} sdk_app_delegate_t;

void sdk_app_set_delegate(sdk_app_delegate_t* delegate);
void sdk_app_run(void); // Main event loop
void sdk_app_terminate(void);
```

### System Integration

#### Notifications
```c
void sdk_notification_post(const char* title, const char* message, const char* sound);
void sdk_notification_set_badge(uint32_t count);
```

#### System Information
```c
const char* sdk_system_get_username(void);
const char* sdk_system_get_version(void);
uint64_t sdk_system_get_uptime(void);
uint32_t sdk_system_get_memory_usage(void);
```

#### Clipboard
```c
void sdk_clipboard_set_text(const char* text);
const char* sdk_clipboard_get_text(void);
```

#### Message Dialogs
```c
void sdk_show_info(const char* title, const char* message);
void sdk_show_warning(const char* title, const char* message);
void sdk_show_error(const char* title, const char* message);
int sdk_show_question(const char* title, const char* message); // Returns 1 for Yes, 0 for No
```

### File Operations

```c
typedef struct sdk_file sdk_file_t;
sdk_file_t* sdk_file_open(const char* path, const char* mode);
void sdk_file_close(sdk_file_t* file);
size_t sdk_file_read(void* buffer, size_t size, sdk_file_t* file);
size_t sdk_file_write(const void* buffer, size_t size, sdk_file_t* file);
```

### Memory Management

```c
void* sdk_malloc(size_t size);
void sdk_free(void* ptr);
void* sdk_realloc(void* ptr, size_t size);
```

## Example Applications

### 1. Hello World GUI App
See `examples/hello_gui_app.c` for a complete example showing:
- Window creation and management
- Basic drawing operations
- Mouse and keyboard event handling
- System color usage

### 2. Controls Demo
See `examples/controls_demo.c` for advanced examples showing:
- Custom control rendering
- Interactive UI elements
- List views and selection
- Advanced drawing techniques

## Building Applications

1. Include the SDK header: `#include "sdk.h"`
2. Implement your application structure with callbacks
3. Register your application with the system
4. Compile with the MyKernel toolchain

## Best Practices

1. **Memory Management**: Always free allocated resources in cleanup callbacks
2. **Event Handling**: Keep event handlers lightweight and responsive
3. **Drawing**: Use the paint callback for all drawing operations
4. **Color Design**: Use system colors for consistency with macOS design language
5. **Window Management**: Set appropriate window styles for your application type

## Future SDK Additions

Planned features for future SDK versions:
- Advanced control widgets (menus, toolbars, tabs)
- Animation and transition APIs
- 3D graphics support
- Audio and media APIs
- Network and communication APIs
- Plugin and extension system

---

For more information and updates, see the MyKernel documentation or visit the developer portal.