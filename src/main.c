#include "identification_app.h"
#include "step_response_app.h"
#include "pid_app.h"

#define APP_TARGET  runPidApp // runStepResponseApp // runPidApp // runPidApp  // 

int main() { APP_TARGET(); }
