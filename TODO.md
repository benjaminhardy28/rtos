# TODO

- **Extend the Renode boot smoke test to cover scheduler/queue behavior.**
  `scripts/ci/boot_smoke_test.py` currently only asserts that boot reaches
  `os_app_main` and that the core isn't stuck in a fault handler, because
  `systick_enable()` is commented out in `kernel/task.c` (`os_start`) and
  ticks never fire. Once ticks are re-enabled, extend the test to assert
  on `app/app_main.c`'s debug globals (`g_error_count == 0`,
  `g_task1_sent`/`g_task2_recv` progressing) for real scheduler/queue
  regression coverage in CI.

- ~~Fix `GDB := aç` in the Makefile.~~ **Fixed** — was a stray typo for
  `arm-none-eabi-gdb`; corrected, `make debug`/`make renode-run` work now.
