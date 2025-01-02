#include <windows.h>
#include "../blk_ecc.h"

#include <string.h>
#include <stdio.h>

const uint8_t magic_bytes[] = {
  0xD3, 0xB6, 0xF6, 0xC0, 0x3C, 0x99, 0x41, 0xAE, 0x1D, 0xD8, 0x94, 0xD3, 0xCA, 0x68, 0x2F, 0x36,
  0x2C, 0x70, 0x06, 0x67, 0x26, 0x36, 0x59, 0xB2, 0x32, 0xB6, 0xC5, 0x47, 0x74, 0x9D, 0x22, 0xA7,
  0xA7, 0xF4, 0x82, 0xCD, 0x2C, 0xB1, 0x29, 0xAC, 0xDB, 0xA0, 0x2E, 0xD6, 0x42, 0xC3, 0x6F, 0xBA,
  0x77, 0xB9, 0x08, 0x03, 0xB3, 0x12, 0x7C, 0x15, 0xC8, 0x25, 0xAC, 0xB2, 0x51, 0xBF, 0xC1, 0x1B,
  0x0E, 0x94, 0xDF, 0xDE, 0x57, 0x4E, 0xB7, 0xB1, 0xEA, 0xA6, 0xD6, 0x4E, 0xD4, 0xED, 0x4B, 0x1E,
  0xA1, 0x8A, 0x66, 0x12, 0xEF, 0x15, 0xAD, 0xE5, 0x7E, 0x66, 0x17, 0x25, 0x6A, 0x9D, 0x33, 0x86,
  0x03, 0x33, 0x83, 0xFE, 0x83, 0x25, 0x30, 0xAD, 0x6D, 0x6A, 0x66, 0x05, 0xFC, 0xFA, 0x37, 0xB3,
  0xFE, 0xD7, 0xA1, 0x70, 0xF8, 0x3B, 0x6E, 0x5E, 0x0C, 0x9A, 0xB5, 0x5E, 0x91, 0xEE, 0xE1, 0x3E,
  0xFC, 0xAC, 0xE6, 0xE3, 0xD6, 0xBD, 0xCE, 0xE3, 0xDA, 0xA6, 0x62, 0x88, 0xF6, 0xBB, 0x9A, 0xC9,
  0x51, 0x54, 0xA0, 0xBB, 0x9A, 0x87, 0xC8, 0xE1, 0x34, 0x73, 0x61, 0x00, 0x2A, 0x36, 0x7A, 0x63,
  0x9B, 0x6F, 0x68, 0x42, 0x20, 0x16, 0xF9, 0x89, 0xD5, 0xAF, 0xDE, 0xDF, 0xD2, 0x32, 0xAB, 0x06,
  0x81, 0x55, 0x92, 0x3B, 0x43, 0x3A, 0x3B, 0x37, 0xFC, 0x01, 0xAD, 0x95, 0x9C, 0x31, 0x6B, 0x34,
  0x80, 0x39, 0x85, 0x09, 0x33, 0x70, 0x98, 0xD7, 0x5D, 0x9F, 0x9B, 0x47, 0x83, 0xCC, 0x24, 0xE1,
  0xDF, 0x91, 0x64, 0xDB, 0xC4, 0x56, 0xAD, 0x8A, 0x8D, 0x9D, 0xB3, 0x36, 0x80, 0x43, 0x5E, 0xE2,
  0x93, 0xF8, 0xEA, 0x47, 0x8F, 0xB4, 0x34, 0xBE, 0x61, 0xA4, 0xF6, 0xE2, 0x34, 0x86, 0xA4, 0x7B,
  0x2C, 0x23, 0xE2, 0xC7, 0x3E, 0x05, 0xB4, 0xD7, 0xD6, 0xD7, 0x58, 0x14, 0xC8, 0x6C, 0x52, 0xDD,
  0x25, 0xCF, 0x5A, 0x91, 0xFC, 0xF1, 0x4B, 0xCC, 0x9B, 0x01, 0xC8, 0xB4, 0xEC, 0xC3, 0xAC, 0xA4,
  0x78, 0xB1, 0x96, 0x48, 0xB8, 0x6B, 0xE8, 0x7F, 0xB2, 0x12, 0xAA, 0xE0, 0x40, 0xB5, 0xA0, 0x12,
  0x6E, 0x1F, 0xAF, 0xBE, 0x7A, 0xF0, 0xBF, 0xD9, 0xEE, 0x3A, 0x7F, 0xC8, 0xF8, 0x42, 0x4D, 0xBE,
  0x8A, 0x5E, 0xB5, 0xFD, 0xE2, 0xF0, 0xA9, 0xE9, 0x49, 0x0A, 0xD9, 0x13, 0x15, 0x8F, 0xC2, 0x0C,
  0xB0, 0xA7, 0x1D, 0xED, 0x53, 0xF2, 0x11, 0xD2, 0xAE, 0x5E, 0xD9, 0x1B, 0xA6, 0xD5, 0xC3, 0x4A,
  0x7E, 0x00, 0x07, 0x55, 0x1F, 0x07, 0xAC, 0xF9, 0xAD, 0xB3, 0x99, 0x26, 0x21, 0xCF, 0x63, 0xA5,
  0xDA, 0xE8, 0x7B, 0x44, 0x12, 0x30, 0x7A, 0xD3, 0xAB, 0x9D, 0xFD, 0x4F, 0x82, 0xD0, 0x8C, 0x3D,
  0xEA, 0xE2, 0xD6, 0xE0, 0x34, 0x2F, 0x2A, 0xE1, 0xCF, 0xBA, 0x8E, 0x28, 0x75, 0xAB, 0x2F, 0x1A,
  0x19, 0x1C, 0x39, 0xAB, 0xBF, 0x38, 0xFE, 0x56, 0xC5, 0x8D, 0xDE, 0x52, 0x2B, 0x45, 0xC6, 0x8D,
  0xEF, 0xF9, 0xF8, 0x4C, 0xBE, 0x4C, 0x8A, 0xA8, 0x54, 0xD7, 0x83, 0x18, 0x38, 0x08, 0x4B, 0x80,
  0x62, 0xB4, 0xFA, 0x1E, 0xA1, 0x99, 0xC6, 0x9D, 0x4D, 0xAA, 0x85, 0xEF, 0xE7, 0x4B, 0x35, 0xDE,
  0x9A, 0xF1, 0xEC, 0xCD, 0x95, 0x12, 0x43, 0xFF, 0x37, 0x15, 0xAF, 0x78, 0xB7, 0x3C, 0x5C, 0x71,
  0x53, 0xC2, 0xC4, 0x19, 0x14, 0x9D, 0x9C, 0xF7, 0xBE, 0xC3, 0x6A, 0xB3, 0xEC, 0x6D, 0xD8, 0x82,
  0x1C, 0xB9, 0xCB, 0x77, 0x63, 0xE3, 0xE8, 0x6B, 0x34, 0x92, 0xE3, 0x8E, 0x82, 0x3A, 0xF9, 0x2B,
  0x5B, 0x73, 0x03, 0xD9, 0x99, 0xED, 0xE6, 0x0A, 0xA7, 0x13, 0x38, 0xF4, 0x4D, 0xB7, 0xED, 0x33,
  0xCC, 0xEA, 0x83, 0xF9, 0x79, 0x39, 0x31, 0x78, 0xC9, 0xE7, 0xF4, 0x17, 0xAC, 0xE6, 0xD2, 0x1A
};

int main()
{
  uint8_t buf[RAW_CHUNK_SZ + RS_CHUNK_SZ];
  FILE *fp;

  initialize_ecc();

  memset(buf, 0, sizeof(buf));
  
  memcpy(buf, magic_bytes, sizeof(magic_bytes));
  ecc_write_chunk(&buf[0], &buf[RAW_CHUNK_SZ]);

  HANDLE h = CreateFileA(
    "\\\\.\\PhysicalDrive1",
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );
  printf("h = %p\n", h);
  DWORD i, x;
  for(i = 0 ; i < sizeof(buf) ; i += 0x200)
  {
    WriteFile(h, &buf[i], 0x200, &x, NULL);
    printf("x = %d\n", x);
    if(!x)
    {
      printf("Error: %d\n", GetLastError());
    }
  }
  CloseHandle(h);

  fp = fopen("test0.bin", "wb");
  fwrite(buf, sizeof(buf), 1, fp);
  fclose(fp);

  ecc_read_chunk(&buf[0], &buf[RAW_CHUNK_SZ]);
  fp = fopen("test1.bin", "wb");
  fwrite(buf, sizeof(buf), 1, fp);
  fclose(fp);
  
  printf("%d | %d\n", sizeof(magic_bytes), memcmp(magic_bytes, buf, sizeof(magic_bytes)));
  
  return 0;
}
