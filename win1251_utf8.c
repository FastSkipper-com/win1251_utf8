#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILE_NAME 1024
#define NAME_POSTFIX "_utf8"

#define SwapTwoBytes(x) (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

struct TableStruct {
	uint8_t win;
	uint32_t utf;
};

struct Utf8Struct {
	uint8_t octets;
	uint32_t value;
};

const struct TableStruct table[] = {
	{ 184, 0xD191 }, // ё
	{ 168, 0xD081 }, // Ё
	{ 176, 0xC2B0 }  // °
};

long tableLength;

// Возвращает индекс последнего вхождения символа в строку, либо -1
long lastChar(char* s, char c) {
	long i = 0;
	long j = -1;
	while (*s) {
		if (*s == c)
		 	j = i;
		i++;
		s++;
	}
	return j;
}

struct Utf8Struct GetUtf8(uint8_t value) {
	struct Utf8Struct utf8char;
	if ((value > 191) && (value < 240)) {
		utf8char.octets = 2;
		utf8char.value = value + 0xCFD0;
	} else if ((value > 239) && (value <= 255)) {
		utf8char.octets = 2;
		utf8char.value = value + 0xD090;
 } else if (value < 127) {
		utf8char.octets = 1;
		utf8char.value = value;
	} else {
		for (long i = 0; i < tableLength; i++) {
			if (value == table[i].win) {
				utf8char.value = table[i].utf;
				utf8char.octets = 2;
			}
		}
	}

	if (utf8char.octets == 2)
		utf8char.value = SwapTwoBytes(utf8char.value);

	return utf8char;
}

int main(int argc, char* argv[]) {
	FILE* source = fopen(argv[1], "rb");
	if (!source) {
		printf("Can't open the source file.\n");
		return 1;
	}
	fseek(source, 0, SEEK_END);
	long fileSize = ftell(source);
	fseek(source, 0, SEEK_SET);

	char fileName[MAX_FILE_NAME] = "";
	long slash = lastChar(argv[1], '/');
	long dot = lastChar(argv[1], '.');
	strncat(fileName, argv[1], slash + 1); // путь до папки
	strncat(fileName, argv[1] + slash + 1, dot - slash - 1); // имя файла без расширения
	strncat(fileName, NAME_POSTFIX, strlen(NAME_POSTFIX)); // постфикс к имени файла
	strncat(fileName, argv[1] + dot, strlen(argv[1]) - dot); // расширение

	FILE* destination = fopen(fileName, "wb");
	if (!destination) {
		printf("Can't open the destination file.\n");
		return 1;
	}

	tableLength = sizeof(table) / sizeof(struct TableStruct);

	uint8_t bom[] = {0xef, 0xbb, 0xbf};
	fwrite(&bom, 1, 3, destination);

	for (long i = 0; i < fileSize; i++) {
		struct Utf8Struct symbol = GetUtf8(getc(source));
		if (symbol.octets)
			fwrite(&symbol.value, symbol.octets, 1, destination);
	}

	fclose(destination);
	fclose(source);
  return 0;
}
