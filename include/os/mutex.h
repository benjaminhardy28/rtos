#pragma once

typedef struct os_mutex_t os_mutex_t;

int os_mutex_lock(os_mutex_t *mutex);
int os_mutex_unlock(os_mutex_t *mutex);
