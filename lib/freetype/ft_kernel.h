#ifndef FT_KERNEL_H
#define FT_KERNEL_H

#include <stdint.h>
#include <stddef.h>

// Basic FreeType types
typedef unsigned char  FT_Byte;
typedef signed int     FT_Int;
typedef unsigned int   FT_UInt;
typedef signed long    FT_Long;
typedef unsigned long  FT_ULong;
typedef signed int     FT_Int32;
typedef unsigned int   FT_UInt32;
typedef int            FT_Error;

// FreeType handles
typedef struct FT_Library_* FT_Library;
typedef struct FT_Face_*    FT_Face;

// Load flags
#define FT_LOAD_DEFAULT        0x0
#define FT_LOAD_RENDER         0x4
#define FT_LOAD_MONOCHROME     0x1000
#define FT_LOAD_NO_HINTING     0x2
#define FT_LOAD_NO_BITMAP      0x8

// Pixel modes
#define FT_PIXEL_MODE_MONO     1
#define FT_PIXEL_MODE_GRAY     2

// Core FreeType API functions
FT_Error FT_Init_FreeType(FT_Library* library);
FT_Error FT_Done_FreeType(FT_Library library);

FT_Error FT_New_Memory_Face(FT_Library library,
                           const FT_Byte* file_base,
                           FT_Long file_size,
                           FT_Long face_index,
                           FT_Face* face);

FT_Error FT_Done_Face(FT_Face face);

FT_Error FT_Set_Pixel_Sizes(FT_Face face,
                           FT_UInt pixel_width,
                           FT_UInt pixel_height);

FT_UInt FT_Get_Char_Index(FT_Face face, FT_ULong charcode);

FT_Error FT_Load_Glyph(FT_Face face,
                      FT_UInt glyph_index,
                      FT_Int32 load_flags);

FT_Error FT_Load_Char(FT_Face face,
                     FT_ULong charcode,
                     FT_Int32 load_flags);

// Simplified render function for kernel use
FT_Error FT_Render_Glyph(FT_Face face,
                        FT_ULong charcode,
                        unsigned char** bitmap,
                        int* width,
                        int* height,
                        int* left,
                        int* top,
                        int* advance_x);

#endif // FT_KERNEL_H