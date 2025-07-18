SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_zanting1_png.c
    ui_img_zanting2_png.c
    ui_img_bofang1_png.c
    ui_img_bofang2_png.c
    ui_img_xiyi1_png.c
    ui_img_xiyi2_png.c
    ui_img_list_loop_png.c
    ui_img_yinliang_png.c
    ui_img_shangyi1_png.c
    ui_img_shangyi2_png.c
    ui_img_jiaopian_png.c
    ui_img_haibao_png.c
    ui_img_citou_png.c
    ui_img_fanhui_png.c
    ui_img_caidan_png.c
    fonts/ui_font_HONOR_Light12.c)

add_library(ui ${SOURCES})
