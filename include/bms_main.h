#ifndef BMS_MAIN_H
#define BMS_MAIN_H

void bms_setup();

/**
 * @brief Drains available BMS UART bytes through the relay. Safety-critical:
 * must be called every loop() iteration, ahead of other background work, so
 * relaying to the main board never gets delayed behind WiFi/task-queue work.
 */
void bms_process_uart();

#endif /* BMS_MAIN_H */