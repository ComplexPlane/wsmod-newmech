.global texscroll_hook_asm
.extern texscroll_hook

texscroll_hook_asm:
    mr r4, r27 # Current itemgroup idx
    b texscroll_hook
