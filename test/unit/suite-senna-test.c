/* -*- c-file-style: "gnu" -*- */

#include <groonga.h>

void groonga_test_warmup(void);
void groonga_test_cooldown(void);

void
groonga_test_warmup(void)
{
  sen_init();
}

void
groonga_test_cooldown(void)
{
  sen_fin();
}