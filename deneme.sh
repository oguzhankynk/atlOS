#!/bin/bash

echo "Hello, World!"

# ARCHDIR değişkenini tanımla
export ARCHDIR="/home/oguzhan/Calisma/kaynakOS/kernel/arch/i386"

# Tanımlanan değişkeni kullanarak bir komut çalıştırabilirsiniz
echo "ARCHDIR değişkeni: $ARCHDIR"

# ARCHDIR ve alt dizinlerinde .cpp uzantılı dosyaları bul
cpp_files=$(find "$ARCHDIR" -type f -name "*.cpp")

# ARCHDIR ve alt dizinlerinde .c uzantılı dosyaları bul
c_files=$(find "$ARCHDIR" -type f -name "*.c")

echo ".cpp dosyaları:"
echo "$cpp_files"
echo
echo ".c dosyaları:"
echo "$c_files"

# Dosya uzantılarını .o olarak değiştir
cpp_o_files=$(echo "$cpp_files" | sed 's/\.cpp/\.o/g')
c_o_files=$(echo "$c_files" | sed 's/\.c/\.o/g')

# Tüm .o dosyalarını birleştir
OUT_FILES="$cpp_o_files"$'\n'"$c_o_files"

echo ".o dosyaları:"
echo "$OUT_FILES"
