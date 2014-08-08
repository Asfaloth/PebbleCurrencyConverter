/* Copyright 2014 Andreas Muttscheller

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "helper.h"

char* itoa(int num) {
  static char buff[20] = {};
  int i = 0, temp_num = num, length = 0;
  char* string = buff;
  if (num >= 0) {
    // count how many characters in the number
    while (temp_num) {
      temp_num /= 10;
      length++;
    }

    // assign the number to the buffer starting at the end of the
    // number and going to the begining since we are doing the
    // integer to character conversion on the last number in the
    // sequence
    for (i = 0; i < length; i++) {
      buff[(length - 1) - i] = '0' + (num % 10);
      num /= 10;
    }
    buff[i] = '\0';  // can't forget the null byte to properly end our string
  } else
    return "Unsupported Number";
  return string;
}

void ftoa(char* str, double val, int precision) {
  int i;
  //  start with positive/negative
  if (val < 0) {
    *(str++) = '-';
    val = -val;
  }
  //  integer value
  snprintf(str, 12, "%d", (int)val);
  str += strlen(str);
  val -= (int)val;
  //  decimals
  if ((precision > 0) && (val >= .00001)) {
    //  add period
    *(str++) = '.';
    //  loop through precision
    for (i = 0; i < precision; i++)
      if (val > 0) {
        val *= 10;
        *(str++) = '0' + (int)(val + ((i == precision - 1) ? .5 : 0));
        val -= (int)val;
      } else
        break;
  }
  //  terminate
  *str = '\0';
}

double strtodbl(char* s) {
  double ret = 0;
  double n = 10;
  int precision = 0;

  while (*s != '\0') {
    if (precision == 0) {
      if (*s == '.' || *s == ',') {
        precision = 1;
        char c = *s++;
      } else {
        ret = ret * 10 + *s++ - '0';
      }
    } else {
      ret += (*s++ - '0') * (1.0 / n);
      n *= 10;
    }
  }

  return ret;
}
