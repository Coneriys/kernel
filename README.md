# byteOS

Простой kernel написанный на C с нуля.

## Возможности

- Система окон
- Управиление памятью
- Графическое окружение

## Требования

- i686-elf-gcc (cross-compiler)
- i686-elf-as (assembler)
- QEMU (для тестирования)

## Установка cross-compiler

### macOS (с Homebrew)
```bash
brew install i686-elf-gcc
```

### Linux
```bash
# Ubuntu/Debian
sudo apt install gcc-multilib

# Или скомпилировать binutils и gcc для i686-elf
```

## Сборка

```bash
make
```

## Запуск

```bash
make test
```

## Структура проекта

- `boot/` - bootloader код
- `kernel/` - основной kernel код
- `include/` - заголовочные файлы
- `drivers/` - драйверы устройств
- `lib/` - библиотеки