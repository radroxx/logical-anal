#include "logical_anal.h"


static void draw_screen(Canvas* canvas, char* buffer, unsigned char* data_buffer, unsigned char index_data ){

    canvas_reset(canvas);
    canvas_set_color(canvas, ColorXOR);
    canvas_draw_str(canvas, 2, 7, buffer);
    canvas_draw_str(canvas, 4, 17, "I       A       I     B    I       C       I          I");
    canvas_draw_str(canvas, 4, 24, "I  7  6  4  I  3  2  I  3   I   O  I HEXI");
    canvas_draw_str(canvas, 4, 29, "--------------------");


    canvas_draw_str(canvas, 4, 34, CHAR_TO_BIN[data_buffer[index_data]]);
    canvas_draw_str(canvas, 4, 41, CHAR_TO_BIN[data_buffer[index_data - 1]]);
    canvas_draw_str(canvas, 4, 48, CHAR_TO_BIN[data_buffer[index_data - 2]]);
    canvas_draw_str(canvas, 4, 55, CHAR_TO_BIN[data_buffer[index_data - 3]]);
    canvas_draw_str(canvas, 4, 62, CHAR_TO_BIN[data_buffer[index_data - 4]]);
    
    canvas_commit(canvas);
}


static void exit_handler(const void* value, void* ctx) {
    furi_assert(value);
    furi_assert(ctx);

    App* app = ctx;
    const InputEvent* event = value;

    //if(event->key == InputKeyBack && event->type == InputTypeLong) {
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        app->is_runing = false;
    }
}


static App* app_alloc(){
    App* app = malloc(sizeof(App));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);
    app->input = furi_record_open(RECORD_INPUT_EVENTS);
    app->canvas = gui_direct_draw_acquire(app->gui);

    app->input_subscription = furi_pubsub_subscribe(app->input, exit_handler, app);

    app->is_runing = true;

    // Init GPIO Interface
    for(size_t io = 0; io < COUNT(gpios); io++) {
        furi_hal_gpio_init(gpios[io], GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    }

    // Init USB uart
    
    // Change usb to dual mode
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_cdc_dual, NULL));

    //furi_hal_uart_init(UART_RORT, 115200);

    app->test = 0;

    return app;
}


static void app_free(App* app) {

    // Change usb to single mode
    furi_check(furi_hal_usb_set_config(&usb_cdc_single, NULL));

    // Free pubsub
    furi_pubsub_unsubscribe(app->input, app->input_subscription);

    app->canvas = NULL;
    gui_direct_draw_release(app->gui);


    // Close gui record
    furi_record_close(RECORD_INPUT_EVENTS);
    furi_record_close(RECORD_GUI);

    // Delete app
    free(app);
}


int32_t logical_anal_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Start app");

    App* app = app_alloc();

    vTaskPrioritySet(furi_thread_get_current_id(), FuriThreadPriorityNormal);

    char head_string_buffer[30] = {0};

    unsigned int rps = 0; // Read gpio ports per sec
    unsigned int cps = 0; // Changed data in gpio ports per sec

    uint8_t cur_state = 0;
    uint8_t index_change = 0;
    uint8_t pos_send_data_to_usb = 0;
    uint8_t show_buffer[255] = {0};

    uint8_t do_in_sec = 0;
    uint32_t rps_ticks = 0;
    
    // Main loop
    while(app->is_runing)
    {
        rps = rps + 1;

        // Read port state
        cur_state = (GPIOA->IDR & 0xC0) | ((GPIOA->IDR & 0x10) << 1) | ((GPIOB->IDR & 0x0C) << 1) |
                  ((GPIOC->IDR & 0x08) >> 1) | (GPIOC->IDR & 0x03);


        if(show_buffer[index_change] != cur_state){
            cps = cps + 1;
            index_change = index_change + 1;
            show_buffer[index_change] = cur_state;
            if( REAL_TIME == false )
            {
                if(index_change % 15){
                    while(pos_send_data_to_usb != index_change)
                    {
                        furi_hal_cdc_send(CBC_NUM, show_buffer + pos_send_data_to_usb + 1, 1);
                        pos_send_data_to_usb = pos_send_data_to_usb + 1;
                    }
                }
            }
        }
        if(REAL_TIME)
        {
            furi_hal_cdc_send(CBC_NUM, show_buffer + index_change, 1);
        }

        if(rps % 10000 == 0 && rps_ticks < furi_get_tick()){
            rps_ticks = furi_get_tick() + furi_ms_to_ticks(200);
            // Run evry 200 ms

            if(REAL_TIME == false)
            {
                while(pos_send_data_to_usb != index_change){
                    furi_hal_cdc_send(CBC_NUM, show_buffer + pos_send_data_to_usb + 1, 1);
                    pos_send_data_to_usb = pos_send_data_to_usb + 1;
                }
            }

            do_in_sec = do_in_sec + 1;
            if( do_in_sec > 4 )
            {
                // Run evry sec
                do_in_sec = 0;

                snprintf(head_string_buffer, 30, "RPS:%.7i   CS:%.7i", rps, cps);
                rps = 0;
                cps = 0;
                furi_thread_yield();
            }

            // Rewrite screen evry 200ms
            draw_screen(app->canvas, head_string_buffer, show_buffer, index_change);
        }

    }

    app_free(app);

    FURI_LOG_I(TAG, "Stop app");

    return 0;
}
