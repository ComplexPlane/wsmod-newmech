.global segmented_beaten_bar
.extern draw_sprite_draw_request

segmented_beaten_bar:
    cmpwi r28, 5
    beq end
    b draw_sprite_draw_request

    end:
    blr
