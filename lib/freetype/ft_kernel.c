// Minimal FreeType implementation for MyKernel
// This is a simplified version specifically for kernel use

#include "ft_kernel.h"
#include "../../include/memory.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Memory management wrapper
static void* ft_alloc(size_t size) {
    return kmalloc(size);
}

static void ft_free(void* ptr) {
    if (ptr) kfree(ptr);
}

// Simple glyph bitmap structure
typedef struct {
    uint8_t* buffer;
    int width;
    int height;
    int pitch;
    int left;
    int top;
    int advance_x;
} FT_GlyphBitmap;

// TrueType font structures
typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} TTF_TableEntry;

typedef struct {
    uint16_t num_tables;
    TTF_TableEntry* tables;
    
    // Important table offsets
    uint32_t cmap_offset;
    uint32_t glyf_offset;
    uint32_t head_offset;
    uint32_t hhea_offset;
    uint32_t hmtx_offset;
    uint32_t loca_offset;
    uint32_t maxp_offset;
    
    // Font metrics
    uint16_t units_per_em;
    int16_t ascender;
    int16_t descender;
    int16_t line_gap;
    uint16_t num_glyphs;
    bool is_long_loca;
} TTF_Font;

struct FT_Library_ {
    void* memory_user;
};

struct FT_Face_ {
    FT_Library library;
    void* font_data;
    size_t font_size;
    TTF_Font* ttf;
    
    // Face metrics
    int num_glyphs;
    int units_per_EM;
    int ascender;
    int descender;
    int height;
    
    // Current size
    int size_pixels;
};

// Read big-endian values
static uint16_t read_u16(const uint8_t* data) {
    return (data[0] << 8) | data[1];
}

static uint32_t read_u32(const uint8_t* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static int16_t read_i16(const uint8_t* data) {
    return (int16_t)read_u16(data);
}

// Initialize FreeType library
FT_Error FT_Init_FreeType(FT_Library* library) {
    *library = ft_alloc(sizeof(struct FT_Library_));
    if (!*library) return -1;
    
    (*library)->memory_user = NULL;
    return 0;
}

// Clean up library
FT_Error FT_Done_FreeType(FT_Library library) {
    if (library) ft_free(library);
    return 0;
}

// Parse TrueType tables
static FT_Error parse_ttf_tables(FT_Face face) {
    const uint8_t* data = (const uint8_t*)face->font_data;
    TTF_Font* ttf = ft_alloc(sizeof(TTF_Font));
    if (!ttf) return -1;
    
    face->ttf = ttf;
    
    // Skip header and read number of tables
    ttf->num_tables = read_u16(data + 4);
    
    // Parse table directory
    const uint8_t* table_dir = data + 12;
    for (int i = 0; i < ttf->num_tables; i++) {
        uint32_t tag = read_u32(table_dir);
        uint32_t offset = read_u32(table_dir + 8);
        
        // Store important table offsets
        if (tag == 0x636D6170) ttf->cmap_offset = offset; // 'cmap'
        else if (tag == 0x676C7966) ttf->glyf_offset = offset; // 'glyf'
        else if (tag == 0x68656164) ttf->head_offset = offset; // 'head'
        else if (tag == 0x68686561) ttf->hhea_offset = offset; // 'hhea'
        else if (tag == 0x686D7478) ttf->hmtx_offset = offset; // 'hmtx'
        else if (tag == 0x6C6F6361) ttf->loca_offset = offset; // 'loca'
        else if (tag == 0x6D617870) ttf->maxp_offset = offset; // 'maxp'
        
        table_dir += 16;
    }
    
    // Parse head table
    if (ttf->head_offset) {
        const uint8_t* head = data + ttf->head_offset;
        ttf->units_per_em = read_u16(head + 18);
        ttf->is_long_loca = (read_i16(head + 50) != 0);
        
        face->units_per_EM = ttf->units_per_em;
    }
    
    // Parse hhea table
    if (ttf->hhea_offset) {
        const uint8_t* hhea = data + ttf->hhea_offset;
        ttf->ascender = read_i16(hhea + 4);
        ttf->descender = read_i16(hhea + 6);
        ttf->line_gap = read_i16(hhea + 8);
        
        face->ascender = ttf->ascender;
        face->descender = ttf->descender;
        face->height = ttf->ascender - ttf->descender + ttf->line_gap;
    }
    
    // Parse maxp table
    if (ttf->maxp_offset) {
        const uint8_t* maxp = data + ttf->maxp_offset;
        ttf->num_glyphs = read_u16(maxp + 4);
        face->num_glyphs = ttf->num_glyphs;
    }
    
    return 0;
}

// Create face from memory
FT_Error FT_New_Memory_Face(FT_Library library, const FT_Byte* file_base,
                           FT_Long file_size, FT_Long face_index, FT_Face* face) {
    if (!library || !file_base || !face) return -1;
    
    *face = ft_alloc(sizeof(struct FT_Face_));
    if (!*face) return -1;
    
    (*face)->library = library;
    (*face)->font_data = (void*)file_base;
    (*face)->font_size = file_size;
    (*face)->size_pixels = 0;
    
    // Parse font tables
    return parse_ttf_tables(*face);
}

// Clean up face
FT_Error FT_Done_Face(FT_Face face) {
    if (face) {
        if (face->ttf) ft_free(face->ttf);
        ft_free(face);
    }
    return 0;
}

// Set character size
FT_Error FT_Set_Pixel_Sizes(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height) {
    if (!face) return -1;
    face->size_pixels = pixel_height ? pixel_height : pixel_width;
    return 0;
}

// Get glyph index from character code
static uint16_t get_glyph_index(FT_Face face, uint32_t charcode) {
    if (!face || !face->ttf || !face->ttf->cmap_offset) return 0;
    
    const uint8_t* data = (const uint8_t*)face->font_data;
    const uint8_t* cmap = data + face->ttf->cmap_offset;
    
    uint16_t num_tables = read_u16(cmap + 2);
    const uint8_t* table = cmap + 4;
    
    // Look for Unicode BMP encoding (platform 3, encoding 1)
    for (int i = 0; i < num_tables; i++) {
        uint16_t platform = read_u16(table);
        uint16_t encoding = read_u16(table + 2);
        uint32_t offset = read_u32(table + 4);
        
        if (platform == 3 && encoding == 1) {
            const uint8_t* subtable = cmap + offset;
            uint16_t format = read_u16(subtable);
            
            if (format == 4 && charcode <= 0xFFFF) {
                // Format 4: segment mapping to delta values
                uint16_t seg_count = read_u16(subtable + 6) / 2;
                const uint8_t* end_codes = subtable + 14;
                const uint8_t* start_codes = end_codes + seg_count * 2 + 2;
                const uint8_t* id_deltas = start_codes + seg_count * 2;
                const uint8_t* id_offsets = id_deltas + seg_count * 2;
                
                // Find segment containing charcode
                for (int j = 0; j < seg_count; j++) {
                    uint16_t end_code = read_u16(end_codes + j * 2);
                    if (charcode <= end_code) {
                        uint16_t start_code = read_u16(start_codes + j * 2);
                        if (charcode >= start_code) {
                            uint16_t id_offset = read_u16(id_offsets + j * 2);
                            if (id_offset == 0) {
                                int16_t id_delta = read_i16(id_deltas + j * 2);
                                return (charcode + id_delta) & 0xFFFF;
                            }
                        }
                        break;
                    }
                }
            }
        }
        table += 8;
    }
    
    return 0;
}

// Get character index
FT_UInt FT_Get_Char_Index(FT_Face face, FT_ULong charcode) {
    return get_glyph_index(face, charcode);
}

// Simple glyph rasterizer
static void rasterize_glyph(FT_Face face, uint32_t glyph_index, FT_GlyphBitmap* bitmap) {
    // For now, create a simple placeholder glyph
    // In a full implementation, this would parse the glyf table
    
    int size = face->size_pixels;
    int width = (size * 2) / 3;
    int height = size;
    
    bitmap->width = width;
    bitmap->height = height;
    bitmap->pitch = width;
    bitmap->left = 0;
    bitmap->top = height * 3 / 4;
    bitmap->advance_x = width + 2;
    
    bitmap->buffer = ft_alloc(width * height);
    if (bitmap->buffer) {
        // Draw a simple box for now
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (y < 2 || y >= height - 2 || x < 2 || x >= width - 2) {
                    bitmap->buffer[y * width + x] = 255;
                } else {
                    bitmap->buffer[y * width + x] = 0;
                }
            }
        }
    }
}

// Load glyph
FT_Error FT_Load_Glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags) {
    // Simplified implementation
    return 0;
}

// Load character
FT_Error FT_Load_Char(FT_Face face, FT_ULong charcode, FT_Int32 load_flags) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, charcode);
    return FT_Load_Glyph(face, glyph_index, load_flags);
}

// Render glyph to bitmap
FT_Error FT_Render_Glyph(FT_Face face, FT_ULong charcode, 
                        unsigned char** bitmap, int* width, int* height,
                        int* left, int* top, int* advance_x) {
    if (!face || !bitmap || !width || !height) return -1;
    
    FT_GlyphBitmap glyph_bitmap;
    uint32_t glyph_index = get_glyph_index(face, charcode);
    
    rasterize_glyph(face, glyph_index, &glyph_bitmap);
    
    *bitmap = glyph_bitmap.buffer;
    *width = glyph_bitmap.width;
    *height = glyph_bitmap.height;
    *left = glyph_bitmap.left;
    *top = glyph_bitmap.top;
    *advance_x = glyph_bitmap.advance_x;
    
    return 0;
}