#include "control_app.h"
#include "identification_app.h"
#include "step_response_app.h"

#define APP_TARGET \
    runControlApp  // runStepResponseApp // runIdentificationApp

int main() { APP_TARGET(); }
