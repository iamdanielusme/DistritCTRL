#ifndef INPUT_H
#define INPUT_H

#include "button_driver.h"
#include "fader_driver.h"

void input_init(Button *btn, Fader *fad);
void input_task(Button *btn, Fader *fad);

#endif
