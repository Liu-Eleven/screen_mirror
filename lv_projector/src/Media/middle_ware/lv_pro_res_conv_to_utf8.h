#ifndef LV_PRO_RES_CONV_TO_UTF8_H
#define LV_PRO_RES_CONV_TO_UTF8_H

int ComUniStrCopyChar(uint8_t *dest, uint8_t *src);
uint8_t *utf16_to_utf8_t(uint8_t *dest, const uint16_t *src, size_t size);
uint32_t ComUniStrToMB(uint16_t* pwStr);
bool IsUTF8(const void* pBuffer, long size);
int string_fmt_conv_to_utf8(unsigned char* buff, char* out_buff, const unsigned int max_size);

#endif  /*LV_PRO_RES_CONV_TO_UTF8_H*/
