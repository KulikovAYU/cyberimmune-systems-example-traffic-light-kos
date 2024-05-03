
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/IMode.idl.h>

#include <assert.h>

#define MODES_NUM 9

/* all possible combination mode */
#define ALLOWED_MODES_COMBINATIONS 32

static void SetMode(uint32_t mode0, uint32_t mode1, traffic_light_IMode_proxy* proxy){

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


/* Control system entity entry point. */
int main(int argc, const char *argv[])
{
    NkKosTransport transport;
    struct traffic_light_IMode_proxy proxy;

    fprintf(stderr, "Hello I'm ControlSystem\n");

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

    /* Test loop. */
    for(uint32_t modeO = 0; modeO < ALLOWED_MODES_COMBINATIONS; ++modeO){
        for(uint32_t mode1 = 0; mode1 < ALLOWED_MODES_COMBINATIONS; ++mode1){
            SetMode(modeO, mode1, &proxy);
            //sleep(1);
        }
    }

    return EXIT_SUCCESS;
}
