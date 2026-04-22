#include "task_queue.h"

#include <Arduino.h>

// Global cooperative scheduler instance; uses millis() as its clock source.
TaskQueueType TaskQueue(millis);