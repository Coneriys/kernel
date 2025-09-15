#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <symbol_name>\n", argv[0]);
        return 1;
    }
    
    FILE* input = fopen(argv[1], "rb");
    if (!input) {
        perror("Failed to open input file");
        return 1;
    }
    
    FILE* output = fopen(argv[2], "w");
    if (!output) {
        perror("Failed to create output file");
        fclose(input);
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    fprintf(output, "#include <stddef.h>\n");
    fprintf(output, "#include <stdint.h>\n\n");
    fprintf(output, "const size_t %s_size = %ld;\n", argv[3], size);
    fprintf(output, "const uint8_t %s_data[] = {\n", argv[3]);
    
    int byte;
    int count = 0;
    while ((byte = fgetc(input)) != EOF) {
        if (count % 16 == 0) {
            fprintf(output, "    ");
        }
        fprintf(output, "0x%02x", byte);
        count++;
        if (count < size) {
            fprintf(output, ",");
        }
        if (count % 16 == 0) {
            fprintf(output, "\n");
        } else {
            fprintf(output, " ");
        }
    }
    
    if (count % 16 != 0) {
        fprintf(output, "\n");
    }
    fprintf(output, "};\n");
    
    fclose(input);
    fclose(output);
    
    printf("Embedded %ld bytes into %s\n", size, argv[2]);
    return 0;
}