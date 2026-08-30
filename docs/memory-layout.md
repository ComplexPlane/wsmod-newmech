# Memory Layout

## Layout

0x80000000: Game memory

* DOL module, DOL BSS, misc DOL stuff
* mainloop.rel + BSS. Has unused relocation data area (~900KB)
    * wsmod REL + BSS
        * Stage arena (fixed size) - reset when loading a new stage
            * Parsed stageconf
            * Gameplay arena (remaining space) - reset when retrying a stage
                * Custom mechanics allocations
    * wsmod arena (fills remaining mainloop.rel unused relocation data space)
        * Parsed wsmod config (from config.json) - variable size
        * Practice Mod heap - allocated from remaining wsmod arena after wsmod config is parsed
* Before mainloop.rel prolog is ran, use game arena for parsing wsmod config
* Game heaps (after mainloop.rel prolog ran, possibly merged)
    * Additional REL+BSS, game allocations, etc
    * Temporary nested heap created for parsing stageconf JSON
* Stack (8KB?)
    
## Requirements

* **Once wsmod is initialized, pracmod's heap is not used by wsmod.** This prevents wsmod from e.g. failing to load a level because too many savestates are allocated in pracmod.
* **wsmod config is allocated with a variable size while only using the space necessary.** Allocating parsed wsmod config info from wsmod arena solves this.
* **Variable per-stage memory can be allocated at runtime, while keeping wsmod memory usage constant.** Allocating per-stage memory in a fixed-size arena meets this.
* **wsmod code or config.json changes should not affect available per-stage memory**. Only the mechanics that stages use should affect per-stage memory, which makes it easier to test. Using a fixed-size arena for per-stage memory satisfies this.
* **JSON parsing memory should be ephemeral and not fragment any existing heaps.** Using temporary, nested heaps allocated from the game's heaps satisfies this.

