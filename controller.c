/**********************************************/
// File     :     controller.c
// Author   :     John Bliss
// Date     :     September 27th 2019
/**********************************************/

#include "controller.h"


enum joy_status joy1_mode = NONE;
enum joy_status joy2_mode = NONE;


static SDL_GameController *controller1 = NULL;
static SDL_GameController *controller2 = NULL;
static bool old_clock = false;
static bool writing = false;
static uint16_t controller1_state = 0;
static uint16_t controller2_state = 0;
static uint8_t clock_count = 0;


bool controller_latch, controller_clock;
bool controller_data1, controller_data2;

bool controller_init(){
  int controller1_number = 0;
  //Try to get first controller, if it is not set to 1
  if(joy1_mode != NONE){
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      if (SDL_IsGameController(i)) {
        controller1 = SDL_GameControllerOpen(i);
        if (controller1) {
          controller1_number = i;
          break;
        } else {
          fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
        }
      }
    }
  }
  if(joy2_mode != NONE){
    for (int i=0; i < SDL_NumJoysticks(); ++i) {
      if (SDL_IsGameController(i) && controller1_number != i) {
        controller2 = SDL_GameControllerOpen(i);
        if (controller2) {
          break;
        } else {
          fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
        }
      }
    }
  }
  writing = false;
  return true;
}

void controller_step(){
  if(joy1_mode == NONE){
    return;
  }
  if(!writing){ //if we are not already writing, check latch to
                //see if we need to start
    handle_latch(controller_latch, controller_clock);
  }

  //if we have started writing controller data and the latch has dropped,
  // we need to start the next bit
  if(writing & !controller_latch){
    //check if clock has changed
    if(controller_clock != old_clock){
      if(old_clock){
        old_clock = controller_clock;
      }else{  //only write next bit when the new clock is high
        clock_count +=1;
        old_clock = controller_clock;
        if(clock_count < 16){ // write out the next 15 bits
          controller_data1 = controller1_state & 1;
          controller_data2 = controller2_state & 1;
          controller1_state = controller1_state >> 1;
          controller2_state = controller2_state >> 1;
        }else{
          //Done writing controller data
          //reset flag and set count to 0
          writing = false;
          clock_count = 0;
          controller_data1 = 0;
          controller_data2 = 0;
        }
      }
    }
  }




}

bool handle_latch(bool latch, bool clock){
  if(latch){
    clock_count = 0;
    //get the 16-representation to put to the VIA
    controller1_state = get_controller_state(controller1);
    controller2_state = get_controller_state(controller2);
    //set writing flag to true to signal we will start writing controller data
    writing = true;
    old_clock = clock;
    //preload the first bit onto VIA
    controller_data1 = controller1_state & 1;
    controller_data2 = controller2_state & 1;
    controller1_state = controller1_state >> 1;
    controller2_state = controller2_state >> 1;
  }

  return latch;
}

//get current state from SDL controller
//Should replace this with SDL events, so we do not miss inputs when polling
uint16_t get_controller_state(SDL_GameController *control){
  if(joy1_mode == NES){
    bool a_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_A);
    bool b_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_X);
    bool select_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_BACK);
    bool start_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_START);
    bool up_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_UP);
    bool down_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    bool left_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    bool right_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    return
      !a_pressed |
      !b_pressed << 1 |
      !select_pressed << 2 |
      !start_pressed << 3 |
      !up_pressed << 4 |
      !down_pressed << 5 |
      !left_pressed << 6 |
      !right_pressed << 7 |
      0x0000;
  }
  if(joy1_mode == SNES){
    bool b_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_A);
    bool y_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_X);
    bool select_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_BACK);
    bool start_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_START);
    bool up_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_UP);
    bool down_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    bool left_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    bool right_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    bool a_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_B);
    bool x_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_Y);
    bool l_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    bool r_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);


    return
      !b_pressed |
      !y_pressed << 1 |
      !select_pressed << 2 |
      !start_pressed << 3 |
      !up_pressed << 4 |
      !down_pressed << 5 |
      !left_pressed << 6 |
      !right_pressed << 7 |
      !a_pressed << 8 |
      !x_pressed << 9 |
      !l_pressed << 10 |
      !r_pressed << 11 |

       0xF000;
  }

  return 0x8FFF;
}
