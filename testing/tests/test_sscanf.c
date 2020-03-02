#include <string.h>
#include <stdio.h>

#if __STDC_VERSION__ < 201710
  #error Too low C standard
#endif

int main(int argc, const char* argv[])
{
  
  printf("__STDC_VERSION__ = '%ld'\n", __STDC_VERSION__);

  const char* line = "Ifs.simi.c_x_0 = 0.785347\n";
  float value;
  const int matched = sscanf(line, "Ifs.simi.c_x_0 = %f\n", &value);
  printf("matched = %d, value = '%f'\n", matched, value);

  return 0;
}
