#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <mode.h>

#define NK_USE_UNQUALIFIED_NAMES

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/IMode.idl.h>

#include <assert.h>

#define STATES_NUM 10
#define SERVICE_STATES_NUM 1
#define SERVICE_MODE 1

static void do_set_mode(uint32_t mode0, uint32_t mode1, traffic_light_IMode_proxy* proxy){

    /* Request and response structures */
    traffic_light_IMode_FMode_req req = {
            .mode = { .dir0 = mode0, .dir1 = mode1 }
    };
    traffic_light_IMode_FMode_res res;

    /*
      * Call Mode interface method.
      * Lights GPIO will be sent a request for calling Mode interface method
      * mode_comp.mode_impl with the value argument. Calling thread is locked
      * until a response is received from the lights gpio.
    */

    nk_err_t result = traffic_light_IMode_FMode(&(proxy->base), &req, NULL, &res, NULL);
    if (result != rcOk){
        fprintf(stderr, "[FAIL]\tFailed to call traffic_light.Mode.Mode()\t dir0 = 0x%02x; dir1 = 0x%02x\n", (int) (req.mode.dir0  & 0xFF), (int) (req.mode.dir1 & 0xFF));
        return;
    }

    /*
       * Print result value from response
       * (result is the output argument of the Mode method).
    */
    fprintf(stderr, "[OK]\ttraffic_light.Mode.Mode() called successfully\t dir0 = 0x%02x; dir1 = 0x%02x\n", (int) (res. result.dir0  & 0xFF), (int) (res.res_.result.dir1  & 0xFF));

}
volatile unsigned char current_mode = 0;

void update_mode(struct Mode *context) {
    current_mode = get_mode(context);

}
struct tl_state{
    int32_t duration;
    uint32_t dir0;
    uint32_t dir1;
} ;

int gpio_work(void* context){
    NkKosTransport transport;
    struct traffic_light_IMode_proxy proxy;


    static const struct tl_state tl_state1[STATES_NUM] = {
            {20,  IMode_WorkGreen, IMode_WorkRed},
            {3,  IMode_WorkGreenBlink, IMode_WorkRed},
            {3,  IMode_WorkYellow, IMode_WorkRed},
            {10,  IMode_WorkRed, IMode_WorkRed},
            {3,  IMode_WorkRed, IMode_WorkRed + IMode_WorkYellow},
            {25,  IMode_WorkRed, IMode_WorkGreen},
            {3,  IMode_WorkRed, IMode_WorkGreenBlink},
            {3,  IMode_WorkRed, IMode_WorkYellow},
            {10,  IMode_WorkRed, IMode_WorkRed},
            {3,  IMode_WorkRed + IMode_WorkYellow, IMode_WorkRed},
    };

    static const struct tl_state tl_state2[SERVICE_STATES_NUM]=
            {
                    {-1,  IMode_WorkYellowBlink, IMode_WorkYellowBlink}
            };

    /**
        * Get the LightsGPIO IPC handle of the connection named
        * "lights_gpio_connection".
        */
    Handle handle = ServiceLocatorConnect("lights_gpio_connection");
    assert(handle != INVALID_HANDLE);

    /* Initialize IPC transport for interaction with the lights gpio entity. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
    * Get Runtime Interface ID (RIID) for interface traffic_light.Mode.mode.
    * Here mode is the name of the traffic_light.Mode component instance,
    * traffic_light.Mode.mode is the name of the Mode interface implementation.
    */
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "lightsGpio.mode");
    assert(riid != INVALID_RIID);

    /**
    * Initialize proxy object by specifying transport (&transport)
    * and lights gpio interface identifier (riid). Each method of the
    * proxy object will be implemented by sending a request to the lights gpio.
    */
    traffic_light_IMode_proxy_init(&proxy, &transport.base, riid);

    /* Request and response structures */
    traffic_light_IMode_FMode_req req;
    traffic_light_IMode_FMode_res res;

    while (1){

        update_mode(context);
        unsigned char mode = current_mode;
        struct tl_state *state = mode == SERVICE_MODE ? tl_state2 : tl_state1;
        int modes_cnt = mode == SERVICE_MODE ? SERVICE_STATES_NUM : STATES_NUM;

        for (int i = 0; i < modes_cnt; ++i) {

            do_set_mode(state[i].dir0, state[i].dir1, &proxy);

            update_mode(context);
            if (current_mode != mode) {
                break;
            }

            if(mode != SERVICE_MODE)
                sleep(state[i].duration);
        }
    }
}