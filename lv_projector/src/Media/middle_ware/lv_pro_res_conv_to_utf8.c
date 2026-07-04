#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lv_pro_res_conv_to_utf8.h"
#include "gb_2312.h"

typedef enum {
    UTF8_TYPE=1,
    UTF16_BE_TYPE,
    UTF16_LE_TYPE,
    TYPE_NULL,
} str_encoder_fmt_e;

int ComUniStrCopyChar(uint8_t *dest, uint8_t *src)
{
    unsigned int i;

    if((NULL == dest) || (NULL == src))
       return 0;
   
    for(i=0; !((src[i] == 0x0 && src[i+1] == 0x0)&&(i%2 == 0)) ;i++)
        dest[i] = src[i];
    if(i%2)
    {
       dest[i] = src[i];
       i++;
    }
    dest[i] = dest[i+1] = 0x0;

    return i/2;
}

/* Convert UTF-16 to UTF-8.  */
uint8_t *utf16_to_utf8_t(uint8_t *dest, const uint16_t *src, size_t size)
{
    uint32_t code_high = 0;

    while (size--) {
        uint32_t code = *src++;

        if (code_high) {
            if (code >= 0xDC00 && code <= 0xDFFF) {
                /* Surrogate pair.  */
                code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;

                *dest++ = (code >> 18) | 0xF0;
                *dest++ = ((code >> 12) & 0x3F) | 0x80;
                *dest++ = ((code >> 6) & 0x3F) | 0x80;
                *dest++ = (code & 0x3F) | 0x80;
            } else {
                /* Error...  */
                *dest++ = '?';
                /* *src may be valid. Don't eat it.  */
                src--;
            }

            code_high = 0;
        } else {
            if (code <= 0x007F) {
                *dest++ = code;
            } else if (code <= 0x07FF) {
                *dest++ = (code >> 6) | 0xC0;
                *dest++ = (code & 0x3F) | 0x80;
            } else if (code >= 0xD800 && code <= 0xDBFF) {
                code_high = code;
                continue;
            } else if (code >= 0xDC00 && code <= 0xDFFF) {
                /* Error... */
                *dest++ = '?';
            } else if (code < 0x10000) {
                *dest++ = (code >> 12) | 0xE0;
                *dest++ = ((code >> 6) & 0x3F) | 0x80;
                *dest++ = (code & 0x3F) | 0x80;
            } else {
                *dest++ = (code >> 18) | 0xF0;
                *dest++ = ((code >> 12) & 0x3F) | 0x80;
                *dest++ = ((code >> 6) & 0x3F) | 0x80;
                *dest++ = (code & 0x3F) | 0x80;
            }
        }
    }

    return dest;
}

uint32_t ComUniStrToMB(uint16_t* pwStr)
 {
     if(pwStr == NULL)
        return 0;
    uint32_t i=0;
    while(pwStr[i])
    {
        pwStr[i]=(uint16_t)(((pwStr[i]&0x00ff)<<8) | ((pwStr[i]&0xff00)>>8));
        i++;
    }
    return i;

 }

bool IsUTF8(const void* pBuffer, long size)
{
    bool IsUTF8 = true;
    int error_time=0;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end)
    {
        //printf("%x\n",*start);
        if (*start < 0x80) // (10000000): 值小于0x80的为ASCII字符
        {
            start++;
        }
        else if (*start < (0xC0)) // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
        {
            error_time++;
            start++;
            if(error_time>3)
            {
                IsUTF8 = false;
                break;
            }
        }
        else if (*start < (0xE0)) // (11100000): 此范围内为2字节UTF-8字符
        {
            if (start >= end - 1)
            {
                break;
            }


            if ((start[1] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }


            start += 2;
        }
        else if (*start < (0xF0)) // (11110000): 此范围内为3字节UTF-8字符
        {
            if (start >= end - 2)
            {
                break;
            }

            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }

            start += 3;
        }
        else
        {
            IsUTF8 = false;
            break;
        }
    }


    return IsUTF8;
}


int string_fmt_conv_to_utf8(unsigned char* buff, char* out_buff, const unsigned int max_size)
{
    uint16_t temp_buff[max_size];
    uint16_t conv_buff[max_size];
    temp_buff[max_size] = 0;
    conv_buff[max_size] = 0;
    /*buffer size include str '\0',+1,conv/temp buff size is 512 Byte*/ 
    int buff_size = strlen(buff) + 1;
    if(buff_size > max_size*2) {
        printf("%s,str too long,str size:%d\n", __func__, buff_size);
        return -1;
    }
    memcpy(temp_buff, buff, buff_size);
    str_encoder_fmt_e str_encoder_fmt = TYPE_NULL;
    if(buff[0] == 0xef && buff[1] == 0xbb) {
        str_encoder_fmt = UTF8_TYPE;
        memcpy(out_buff, buff, buff_size);
    } else if(buff[0] == 0xff && buff[1] == 0xfe) {
        str_encoder_fmt = UTF16_LE_TYPE;
        ComUniStrCopyChar( (uint8_t *)conv_buff, (uint8_t *)temp_buff);
        memset(temp_buff, 0, sizeof(temp_buff));
        utf16_to_utf8_t((uint8_t *)temp_buff, conv_buff, buff_size);
        memcpy(out_buff, (uint8_t *)temp_buff, sizeof(temp_buff));
    } else if(buff[0] == 0xfe && buff[1] == 0xff) {
        str_encoder_fmt = UTF16_BE_TYPE;
        ComUniStrToMB(temp_buff);
        ComUniStrCopyChar( (uint8_t *)conv_buff, (uint8_t *)temp_buff);
        memset(temp_buff, 0, sizeof(temp_buff));
        utf16_to_utf8_t((uint8_t *)temp_buff, conv_buff, buff_size);        
        memcpy(out_buff, (uint8_t *)temp_buff, sizeof(temp_buff));
    } else {
        str_encoder_fmt = TYPE_NULL;
        if(IsUTF8(buff, buff_size)) {
            memcpy(out_buff, buff, buff_size);
        } else {
            convert_gb2312_to_unicode((uint8_t *)temp_buff, buff_size, conv_buff, sizeof(conv_buff));
            ComUniStrToMB((uint16_t *)conv_buff);
            ComUniStrCopyChar(  (uint8_t *)temp_buff, (uint8_t *)conv_buff);
            memset(conv_buff, 0, sizeof(conv_buff));
            utf16_to_utf8_t((uint8_t *)conv_buff, temp_buff, buff_size);
            memcpy(out_buff, (uint8_t *)conv_buff, sizeof(conv_buff));
        }
    }
    return 0;
}

