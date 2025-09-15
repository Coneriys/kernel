int main(void) {
    volatile char* video = (volatile char*)0xB8000;
    const char* message = "Hello from userland program!";
    int color = 0x0F;
    
    for (int i = 0; i < 29; i++) {
        video[(80 * 10 + 25) * 2 + i * 2] = message[i];
        video[(80 * 10 + 25) * 2 + i * 2 + 1] = color;
    }
    
    return 42;
}