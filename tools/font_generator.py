#!/usr/bin/env python3
"""
Font Generator для MyKernel
Конвертирует TTF шрифты в C массивы с антиалиасингом
"""

try:
    from PIL import Image, ImageDraw, ImageFont
    import numpy as np
    import sys
    import os
except ImportError:
    print("Установите зависимости: pip install pillow numpy")
    sys.exit(1)

def generate_antialiased_font(font_path, size, output_file):
    """Генерирует антиалиасированный шрифт"""
    
    font = ImageFont.truetype(font_path, size)
    
    # Определяем размеры символов для лучшего качества Inter
    char_width = 14  # увеличиваем ширину для Inter
    char_height = 18  # увеличиваем высоту для лучшего качества
    
    font_data = []
    
    # ASCII символы 32-126 (95 символов)
    for char_code in range(32, 127):
        char = chr(char_code)
        
        # Создаем изображение для символа
        img = Image.new('L', (char_width, char_height), 0)
        draw = ImageDraw.Draw(img)
        
        # Рисуем символ
        try:
            # Получаем bbox символа для центрирования
            bbox = draw.textbbox((0, 0), char, font=font)
            text_width = bbox[2] - bbox[0]
            text_height = bbox[3] - bbox[1]
            
            # Центрируем символ
            x = (char_width - text_width) // 2
            y = (char_height - text_height) // 2 - bbox[1]
            
            draw.text((x, y), char, font=font, fill=255)
        except:
            pass  # Пропускаем символы, которые нельзя отрисовать
        
        # Конвертируем в массив с улучшенным антиалиасингом
        pixels = np.array(img)
        char_data = []
        
        for row in pixels:
            row_bytes = []
            for i in range(0, len(row), 8):
                byte_val = 0
                for j in range(8):
                    if i + j < len(row):
                        # Улучшенный алгоритм: используем более низкий порог для четкости
                        if row[i + j] > 64:  # снижаем порог для лучшей четкости
                            byte_val |= (1 << (7 - j))
                row_bytes.append(byte_val)
            char_data.extend(row_bytes)
        
        font_data.append(char_data)
    
    # Генерируем C код
    with open(output_file, 'w') as f:
        f.write("// Inter Regular font data (antialiased)\n")
        f.write(f"// Size: {char_width}x{char_height} pixels\n")
        f.write(f"// Generated from {os.path.basename(font_path)}\n\n")
        
        f.write(f"static const uint8_t inter_font_{size}x{char_height}[95][{len(font_data[0])}] = {{\n")
        
        for i, char_data in enumerate(font_data):
            char_code = i + 32
            char_name = chr(char_code) if chr(char_code).isprintable() and chr(char_code) != '"' and chr(char_code) != '\\' else f"#{char_code}"
            
            f.write(f"    // {char_name} ({char_code})\n")
            f.write("    {")
            
            for j, byte_val in enumerate(char_data):
                if j > 0:
                    f.write(", ")
                if j % 16 == 0 and j > 0:
                    f.write("\n     ")
                f.write(f"0x{byte_val:02X}")
            
            f.write("}")
            if i < len(font_data) - 1:
                f.write(",")
            f.write("\n")
        
        f.write("};\n\n")
        
        # Добавляем метаинформацию
        f.write(f"#define INTER_FONT_WIDTH {char_width}\n")
        f.write(f"#define INTER_FONT_HEIGHT {char_height}\n")
        f.write(f"#define INTER_FONT_SIZE {size}\n")

if __name__ == "__main__":
    font_path = "/tmp/Inter Desktop/Inter-Regular.otf"
    output_file = "/Users/connor/smthbig/mykernel/kernel/inter_font.h"
    size = 14  # увеличиваем размер для лучшего качества
    
    if not os.path.exists(font_path):
        print(f"Файл шрифта не найден: {font_path}")
        sys.exit(1)
    
    print(f"Генерируем шрифт Inter {size}px...")
    generate_antialiased_font(font_path, size, output_file)
    print(f"Готово! Файл создан: {output_file}")