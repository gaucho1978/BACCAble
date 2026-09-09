#include "app/powertrain.h"
#if defined(BACCABLE_C1)
void dashboard_send_main() {
    uint8_t tmpStrLen = 0;
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // first char shall be a # to talk with slave canable connected to BH can bus
    char tmpfloatString[5]; // temp array
    // update records if required
    switch (dashboard_state.main_dashboard_page_index) {
    case 2: // READ_FAULTS_ENABLED
        if (settings_state.read_faults_enabled == 1) {
        }
        break;
    case 3:
        if (settings_state.clear_faults_enabled == 1) {
            if (diagnostics_state.clear_faults_request > 0) {
                memcpy(dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index],
                       "WAIT...     ", 12);
                dashboard_state.commands_menu_enabled = 0; // disable menu movement
            } else {
                memcpy(dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index],
                       "CLEAR FAULTS", 12);
                dashboard_state.commands_menu_enabled = 1; // enable menu movement
            }
        }
        break;
    case 4: // immo
        if (security_state.immobilizer_enabled) {
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                'N'; // on
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] = ' ';
        } else {
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                'F'; // off
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] = 'F';
        }
        break;
    case 5: // dyno
        if (dashboard_state.print_stop_the_car > 0) {
            dashboard_state.print_stop_the_car--;
            uint8_t stopTheCarMsg[13] = {
                BhBusIDparamString, 'S', 'T', 'O', 'P', ' ', 'T', 'H', 'E', ' ', 'C', 'A', 'R'};
            board_uart_send(stopTheCarMsg, 13); // print message "stop the car"
            return;
        }
        break;
    case 6: // ESC/TC
        break;
    case 7: // front brake
        if (dashboard_state.print_stop_the_car > 0) {
            dashboard_state.print_stop_the_car--;
            uint8_t stopTheCarMsg[13] = {
                BhBusIDparamString, 'S', 'T', 'O', 'P', ' ', 'T', 'H', 'E', ' ', 'C', 'A', 'R'};
            board_uart_send(stopTheCarMsg, 13); // print message "stop the car"
            return;
        }

        if (dashboard_state.print_enable_dyno > 0) {
            dashboard_state.print_enable_dyno--;
            uint8_t enableDynoMsg[12] = {
                BhBusIDparamString, 'E', 'N', 'A', 'B', 'L', 'E', ' ', 'D', 'Y', 'N', 'O'};
            board_uart_send(enableDynoMsg, 12); // print message "Enable Dyno"
            return;
        }
        // update the string
        if (chassis_state.front_brake_forced == 0) {
            // update text {'F','r','o','n','t',' ','B','r','a','k','e',' ','N','o','r','m','a','l' },
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][0] = 'F';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][1] = 'r';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][2] = 'o';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][3] = 'n';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] = 't';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] = ' ';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] = 'B';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] = 'r';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] = 'a';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] = 'k';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][10] = 'e';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][11] = ' ';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][12] =
                'N'; // normal
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][13] = 'o'; //
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] = 'r'; //
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] = 'm';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][16] = 'a';
            dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][17] = 'l';
        } else {
            if (chassis_state.launch_assist_enabled == 1) {
                if (telemetry_state.torque >
                    50) { // assist active and torque greather than minimum threshold. the minimum thr allows
                          // to view the menu "...assist" before the following string is printed
                    // prepare variables

                    // show torque
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][0] =
                        'L';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][1] =
                        'a';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][2] =
                        'u';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][3] =
                        'n';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] =
                        'c';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] =
                        'h';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] =
                        ' ';

                    format_number(tmpfloatString, (float)telemetry_state.torque, 0, 4);
                    switch (strlen(tmpfloatString)) {
                    case 1:
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] = ' ';
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] = ' ';
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] =
                            tmpfloatString[0];
                        break;
                    case 2:
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] = ' ';
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] =
                            tmpfloatString[0];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] =
                            tmpfloatString[1];
                        break;
                    default:
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] =
                            tmpfloatString[0];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] =
                            tmpfloatString[1];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] =
                            tmpfloatString[2];
                        break;
                    }

                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][10] =
                        'N';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][11] =
                        'm';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][12] =
                        '/';

                    format_number(tmpfloatString, (float)settings_state.launch_torque_threshold, 0, 4);
                    if (strlen(tmpfloatString) == 2) {
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][13] = ' ';
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                            tmpfloatString[0];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] =
                            tmpfloatString[1];
                    } else {
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][13] =
                            tmpfloatString[0];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                            tmpfloatString[1];
                        dashboard_state
                            .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] =
                            tmpfloatString[2];
                    }

                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][16] =
                        'N';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][17] =
                        'm';
                } else {
                    // update text
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][0] =
                        'F';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][1] =
                        'r';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][2] =
                        'o';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][3] =
                        'n';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] =
                        't';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] =
                        ' ';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] =
                        'B';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] =
                        'r';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] =
                        'a';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] =
                        'k';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][10] =
                        'e';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][11] =
                        ' ';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][12] =
                        'A'; // assist (launch control)
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][13] =
                        's'; //
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                        's'; //
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] =
                        'i';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][16] =
                        's';
                    dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][17] =
                        't';
                }
            } else { // launch assist not enabled
                // update text
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][0] = 'F';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][1] = 'r';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][2] = 'o';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][3] = 'n';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] = 't';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] = ' ';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] = 'B';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][7] = 'r';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][8] = 'a';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][9] = 'k';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][10] =
                    'e';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][11] =
                    ' ';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][12] =
                    'F'; // forced
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][13] =
                    'o'; //
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][14] =
                    'r'; //
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][15] =
                    'c';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][16] =
                    'e';
                dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][17] =
                    'd';
            }
        }
        break;
    case 8: // 4wd
        if (dashboard_state.print_stop_the_car > 0) {
            dashboard_state.print_stop_the_car--;
            uint8_t stopTheCarMsg[13] = {
                BhBusIDparamString, 'S', 'T', 'O', 'P', ' ', 'T', 'H', 'E', ' ', 'C', 'A', 'R'};
            board_uart_send(stopTheCarMsg, 13); // print message "stop the car"
            return;
        }
        // nothing to do
        break;
    default:
        // nothing to do
        break;
    }

    // add string to record
    switch (dashboard_state.main_dashboard_page_index) {
    case 0:
        tmpStrLen = strlen(FW_VERSION);
        if (tmpStrLen > DASHBOARD_MESSAGE_MAX_LENGTH)
            tmpStrLen = DASHBOARD_MESSAGE_MAX_LENGTH;
        memcpy(&runtime_state.uart_tx_msg[1], FW_VERSION, tmpStrLen);
        if (tmpStrLen < DASHBOARD_MESSAGE_MAX_LENGTH) { // if required pad with spaces
            memset(&runtime_state.uart_tx_msg[1 + tmpStrLen], ' ',
                   UART_BUFFER_SIZE - (1 + tmpStrLen)); // set to zero remaining chars
        }
        break;
    default:
        memcpy(&runtime_state.uart_tx_msg[1],
               dashboard_state.dashboard_main_menu_array[dashboard_state.main_dashboard_page_index],
               UART_BUFFER_SIZE - 1);
        break;
    }

    // send to slave baccable
    board_uart_send(runtime_state.uart_tx_msg, UART_BUFFER_SIZE);
}

void dashboard_send_setup() {
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // first char shall be a # to talk with slave canable connected to BH can bus

    setup_render_page(setup_dashboardPageIndex);

    memcpy(&runtime_state.uart_tx_msg[1], &dashboard_setup_menu_array[setup_dashboardPageIndex],
           UART_BUFFER_SIZE - 1);
    // send to slave baccable
    board_uart_send(runtime_state.uart_tx_msg, UART_BUFFER_SIZE);
}

void dashboard_send_parameter_setup() {
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // first char shall be a # to talk with slave canable connected to BH can bus
    switch (parameter_setup_page_index) {
    case 0: //{'S','A','V','E','&','E','X','I','T',},
        runtime_state.uart_tx_msg[1] = 'S';
        runtime_state.uart_tx_msg[2] = 'A';
        runtime_state.uart_tx_msg[3] = 'V';
        runtime_state.uart_tx_msg[4] = 'E';
        runtime_state.uart_tx_msg[5] = ' ';
        runtime_state.uart_tx_msg[6] = '&';
        runtime_state.uart_tx_msg[7] = ' ';
        runtime_state.uart_tx_msg[8] = 'E';
        runtime_state.uart_tx_msg[9] = 'X';
        runtime_state.uart_tx_msg[10] = 'I';
        runtime_state.uart_tx_msg[11] = 'T';
        runtime_state.uart_tx_msg[12] = 0;
        runtime_state.uart_tx_msg[13] = 0;
        runtime_state.uart_tx_msg[14] = 0;
        runtime_state.uart_tx_msg[15] = 0;
        runtime_state.uart_tx_msg[16] = 0;
        runtime_state.uart_tx_msg[17] = 0;
        runtime_state.uart_tx_msg[18] = 0;

        break;
    default: // hidden or shown params
        runtime_state.uart_tx_msg[1] =
            dashboard_state.checkbox_symbols[parameter_page_visibility[parameter_setup_page_index - 1]];
        runtime_state.uart_tx_msg[2] = ' ';
        runtime_state.uart_tx_msg[3] = ' ';

        char tmpName[DASHBOARD_MESSAGE_MAX_LENGTH + 5];

        memcpy(
            tmpName, parameter_pages[settings_state.is_diesel_enabled][parameter_setup_page_index - 1].name,
            strlen(parameter_pages[settings_state.is_diesel_enabled][parameter_setup_page_index - 1].name) +
                1);

        uint8_t tmpStrLen = dashboard_remove_placeholders(tmpName); // remove special patterns from template
        if (tmpStrLen > UART_BUFFER_SIZE - 4)
            tmpStrLen = UART_BUFFER_SIZE - 4;
        memcpy(&runtime_state.uart_tx_msg[4], tmpName,
               tmpStrLen); // copy entire string, to fill it with 0 at the end.
        memset(&runtime_state.uart_tx_msg[4 + tmpStrLen], 0x20, UART_BUFFER_SIZE - 4 - tmpStrLen);

        // uint8_t tmpStrLenName=strlen((char
        // *)parameter_pages[function_is_diesel_enabled][parameter_setup_page_index-1].name); uint8_t
        // tmpStrLenUnits=strlen((char
        // *)parameter_definitions[parameter_pages[function_is_diesel_enabled][parameter_setup_page_index-1].parameter_ids[selected_parameter_element]].unit);
        // uint8_t tmpCharsToWrite=UART_BUFFER_SIZE-4-tmpStrLenName; //number of chars that we can still use
        // in the string if(tmpCharsToWrite>tmpStrLenUnits+1) tmpCharsToWrite= tmpStrLenUnits+1; //if we have
        // more space than what we have to write, set the total number of chars to write
        // if(tmpCharsToWrite>0){
        //	uartTxMsg[4+tmpStrLenName]=' '; //add a space
        //	tmpCharsToWrite--;
        // }
        // if(tmpCharsToWrite>0) memcpy(&uartTxMsg[4+tmpStrLenName+1],
        // &parameter_definitions[parameter_pages[function_is_diesel_enabled][parameter_setup_page_index-1].parameter_ids[selected_parameter_element]].unit,tmpCharsToWrite);

        break;
    }

    // send to slave baccable
    board_uart_send(runtime_state.uart_tx_msg, UART_BUFFER_SIZE);
}

void dashboard_send_values() {
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // first char shall be a # to talk with slave canable connected to BH can bus

    char stringToPrint[25];
    dashboard_format_values(
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].name,
        displayed_parameter_values,
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].parameter_ids,
        stringToPrint); // build string to print

    uint8_t tmpStrLen = strlen(stringToPrint);
    if (tmpStrLen > DASHBOARD_MESSAGE_MAX_LENGTH)
        tmpStrLen = DASHBOARD_MESSAGE_MAX_LENGTH;                     // truncate it. no space left
    memcpy(&runtime_state.uart_tx_msg[1], &stringToPrint, tmpStrLen); // prepare name of parameter

    if (tmpStrLen < DASHBOARD_MESSAGE_MAX_LENGTH) { // if required pad with zeros
        memset(&runtime_state.uart_tx_msg[1 + tmpStrLen], ' ',
               UART_BUFFER_SIZE - (1 + tmpStrLen)); // set to zero remaining chars
    }
    board_uart_send(runtime_state.uart_tx_msg, UART_BUFFER_SIZE);
}

void dashboard_clear() {
    // prepare empty message
    runtime_state.uart_tx_msg[0] =
        BhBusIDparamString; // # to send message to baccable slave connected to BH can bus
    for (uint8_t i = 1; i < UART_BUFFER_SIZE; i++) {
        runtime_state.uart_tx_msg[i] = 0x20; // space char
    }
    // send it
    board_uart_send(runtime_state.uart_tx_msg, UART_BUFFER_SIZE);
}
#endif
