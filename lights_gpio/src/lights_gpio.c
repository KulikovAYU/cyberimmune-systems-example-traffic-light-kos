
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <rtl/string.h>
#define NK_USE_UNQUALIFIED_NAMES

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* EDL description of the LightsGPIO entity. */
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

#include <assert.h>

static uint32_t lights_mode[2] = {0};

struct check_lights_result{
    uint32_t code;
    char msg[256];
};

/* Type of interface implementing object. */
typedef struct IModeImpl {
    struct traffic_light_IMode base;     /* Base interface of object */
    rtl_uint32_t step;                   /* Extra parameters */
} IModeImpl;

/* Mode method implementation. */
static nk_err_t FMode_impl(struct traffic_light_IMode *self,
                          const struct traffic_light_IMode_FMode_req *req,
                          const struct nk_arena *req_arena,
                          traffic_light_IMode_FMode_res *res,
                          struct nk_arena *res_arena)
{
    IModeImpl *impl = (IModeImpl *)self;
    /**
     * Increment value in control system request by
     * one step and include into result argument that will be
     * sent to the control system in the lights gpio response.
     */
    //res->result = req->mode + impl->step;
    /* Запомнить состояние*/
    lights_mode[0] = req->mode.dir0 & 0xFF;
    lights_mode[1] = req->mode.dir1 & 0xFF;

    res->result = req->mode;
    return NK_EOK;
}

/**
 * IMode object constructor.
 * step is the number by which the input value is increased.
 */
static struct traffic_light_IMode *CreateIModeImpl(rtl_uint32_t step)
{
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_IMode_ops ops = {
        .FMode = FMode_impl
    };

    /* Interface implementing object. */
    static struct IModeImpl impl = {
        .base = {&ops}
    };

    impl.step = step;

    return &impl.base;
}


#define LIGHTS_OK 42
#define LIGHTS_FORBIDDEN 13
#define LIGHTS_ALLOFF 99
#define LIGHST_DOUBTFUL 63


struct check_lights_result check_lights(){

    struct check_lights_result clr = {.code = LIGHTS_OK};
    //memset(clr.msg, 0, 256);
    strncpy(clr.msg,"all ok",256);

    /* Если  два зелёных на в обоих направлениях - запрещенное */
    if ((lights_mode[0] & 0x4) == 0x4 && (lights_mode[1] & 0x4) == 0x4){
        clr.code = LIGHTS_FORBIDDEN;
        strncpy(clr.msg,"forbidden state",256);
        return clr;
    }

    /* если вместе с зелёным горит чтото еще в каклм-то из направлений - сомнительное */
    if (((lights_mode[0] & 0x4) == 0x4 && (lights_mode[0] & 0xB)) ||
        ((lights_mode[1] & 0x4) == 0x4 && (lights_mode[1] & 0xB))){
        clr.code = LIGHST_DOUBTFUL;
        strncpy(clr.msg,"seems doubtful state",256);
        return clr;
    }

    /* если всё выключено */
    if (!(lights_mode[0] & 0xFFF) &&
        !(lights_mode[1] & 0xFFF)){
        clr.code = LIGHTS_ALLOFF;
        strncpy(clr.msg,"lights all off",256);
        return clr;
    }

    /* нерегулируемый */
    if (lights_mode[0] == 0x0A &&
        lights_mode[1] == 0x0A){
        clr.code = LIGHTS_ALLOFF;
        strncpy(clr.msg,"blinking yellow",256);
        return clr;
    }

    return clr;
}

static const char EntityName[] = "LightsGPIO";

/* Lights GPIO entry point. */
int main(void)
{
    NkKosTransport transport;
    ServiceId iid;

    /* Get lights gpio IPC handle of "lights_gpio_connection". */
    Handle handle = ServiceLocatorRegister("lights_gpio_connection", NULL, 0, &iid);
    assert(handle != INVALID_HANDLE);

    /* Initialize transport to control system. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
     * Prepare the structures of the request to the lights gpio entity: constant
     * part and arena. Because none of the methods of the lights gpio entity has
     * sequence type arguments, only constant parts of the
     * request and response are used. Arenas are effectively unused. However,
     * valid arenas of the request and response must be passed to
     * lights gpio transport methods (nk_transport_recv, nk_transport_reply) and
     * to the lights gpio method.
     */
    traffic_light_LightsGPIO_entity_req req;
    char req_buffer[traffic_light_LightsGPIO_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer,
                                        req_buffer + sizeof(req_buffer));

    /* Prepare response structures: constant part and arena. */
    traffic_light_LightsGPIO_entity_res res;
    char res_buffer[traffic_light_LightsGPIO_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                        res_buffer + sizeof(res_buffer));

    /**
     * Initialize mode component dispatcher. 3 is the value of the step,
     * which is the number by which the input value is increased.
     */
    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(0x1000000));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_LightsGPIO_entity entity;
    traffic_light_LightsGPIO_entity_init(&entity, &component);


    /*Init diagnostic connection*/
    struct IDiagMessage_proxy d_proxy;
    Handle d_handle = ServiceLocatorConnect("diagnostics_connection");
    assert(d_handle != INVALID_HANDLE);

    NkKosTransport d_transport;
    NkKosTransport_Init(&d_transport, d_handle, NK_NULL, 0);

    nk_iid_t d_riid = ServiceLocatorGetRiid(d_handle, "diagnostics.dmessage");
    assert(d_riid != INVALID_HANDLE);

    IDiagMessage_proxy_init(&d_proxy, &d_transport.base, d_riid);

    IDiagMessage_DMessage_req d_req;
    IDiagMessage_DMessage_res d_res;

    char d_reqBuffer[IDiagMessage_DMessage_req_arena_size];
    struct nk_arena d_arena = NK_ARENA_INITIALIZER(d_reqBuffer, d_reqBuffer + sizeof(d_reqBuffer));

    fprintf(stderr, "Hello I'm LightsGPIO\n");

    nk_err_t rc;
    /* Dispatch loop implementation. */
    do
    {
        /* Flush request/response buffers. */
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&transport.base,
                              &req.base_,
                              &req_arena) != NK_EOK) {
            fprintf(stderr, "nk_transport_recv error\n");
        } else {
            /**
             * Handle received request by calling implementation Mode_impl
             * of the requested Mode interface method.
             */
            traffic_light_LightsGPIO_entity_dispatch(&entity, &req.base_, &req_arena,
                                        &res.base_, &res_arena);
        }

        /* Send response. */
        if (nk_transport_reply(&transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "nk_transport_reply error\n");
        }


        struct check_lights_result clr = check_lights();
        d_req.code = clr.code;

        nk_req_reset(&d_req);
        nk_arena_reset(&d_arena);

        rtl_size_t msg_len = rtl_strlen(clr.msg)+1;
        nk_char_t *str = nk_arena_alloc(
                nk_char_t,
                &d_arena,
                &d_req.msg,
                msg_len);
        if (str == RTL_NULL)
        {
            fprintf(
                    stderr,
                    "[%s]: Error: can`t allocate memory in arena!\n",
                    EntityName);
        }

        rtl_strncpy(str, clr.msg, (rtl_size_t) msg_len);

        if ((rc = IDiagMessage_DMessage(&d_proxy.base,
                                        &d_req,
                                        &d_arena,
                                        &d_res,
                                        NULL)
            ) != NK_EOK){
            fprintf(stderr, "[%s] IDiagMessage_DMessage error %d\n", EntityName, rc);
        }

    }
    while (true);

    return EXIT_SUCCESS;
}
