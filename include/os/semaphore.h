#pragma once

#include <stdint.h>

typedef struct os_sem_t os_sem_t;

int os_sem_take(os_sem_t *sem);
int os_sem_give(os_sem_t *sem);
