VALGRIND = valgrind --tool=memcheck \
    --leak-check=full \
    --leak-resolution=high \
    --child-silent-after-fork=yes \
    --num-callers=20 \
    --gen-suppressions=all

# other potentially interesting options:
# --show-reachable=yes   reachable objects (many!)
# --read-var-info=yes    better diagnostics from DWARF3 info
# --track-origins=yes    better diagnostics for uninit values (slow)
