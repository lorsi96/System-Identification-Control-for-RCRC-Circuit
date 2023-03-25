#include "identification_app.h"
#include "step_response_app.h"
#include "pid_app.h"

#define APP_TARGET runPidApp // runPidApp  // runStepResponseApp

int main() { APP_TARGET(); }
