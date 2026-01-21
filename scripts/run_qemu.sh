cat > scripts/debug_qemu.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
qemu-system-arm -M mps2-an385 -kernel build/rtos.elf -nographic -S -gdb tcp::1234
EOF
chmod +x scripts/debug_qemu.sh

