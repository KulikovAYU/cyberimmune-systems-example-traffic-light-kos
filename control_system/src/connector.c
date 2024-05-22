#include <assert.h>
#include <stdio.h>
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>
#include <traffic_light/ControlSystem.idl.h>
#include <traffic_light/ControlSystem.edl.h>
#include <nk/arena.h>
#include <mode.h>

typedef struct SetModeInfo {
    struct nk_arena res_arena;
    struct nk_arena req_arena;
    traffic_light_ControlSystem_entity_req req;
    traffic_light_ControlSystem_entity_res res;
    NkKosTransport transport;
    traffic_light_ControlSystem_entity entity;
    Handle handle;
} SetModeInfo;

/* Type of interface implementing object. */
typedef struct SetModeImpl {
    struct traffic_light_ControlSystem base;     /* Base interface of object */
} SetModeImpl;

/* Mode method implementation. */
static nk_err_t SetMode_impl(struct traffic_light_ControlSystem *self,
                             const struct traffic_light_ControlSystem_set_req *req,
                             const struct nk_arena *req_arena,
                             traffic_light_ControlSystem_set_res *res,
                             struct nk_arena *res_arena)
{
    // SetModeImpl *impl = (SetModeImpl *)self;
    /**
     * Increment value in control system request by
     * one step and include into result argument that will be
     * sent to the control system in the lights gpio response.
     */
    fprintf(stderr, "[ControlSystem] Accepted direction %d\n", req->value);
    res->result = req->value;
    return NK_EOK;
}

/**
 * IMode object constructor.
 * step is the number by which the input value is increased.
 */
static struct traffic_light_ControlSystem *CreateSetModeImpl()
{
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_ControlSystem_ops ops = {
            .set = SetMode_impl
    };

    /* Interface implementing object. */
    static struct SetModeImpl impl = {
            .base = {&ops}
    };

    return &impl.base;
}

static struct SetModeInfo* connector_init()
{
    const char req_buffer[traffic_light_ControlSystem_entity_req_arena_size];
    const char res_buffer[traffic_light_ControlSystem_entity_res_arena_size];

    static struct SetModeInfo info;

    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer,
                                                     req_buffer + sizeof(req_buffer));
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                                     res_buffer + sizeof(res_buffer));

    info.req_arena = req_arena;
    info.res_arena = res_arena;

    static traffic_light_ControlSystem_component component;
    traffic_light_ControlSystem_component_init(&component, CreateSetModeImpl());

    traffic_light_ControlSystem_entity_init(&info.entity, &component);

    ServiceId iid;
    info.handle = ServiceLocatorRegister("connector_control_system_connection", NULL, 0, &iid);
    assert(info.handle != INVALID_HANDLE);
    NkKosTransport_Init(&info.transport, info.handle, NK_NULL, 0);
    return &info;
}


static void connector_loop(struct SetModeInfo* info, struct Mode *mode)
{
    /* Flush request/response buffers. */
    nk_req_reset(&info->req);
    nk_arena_reset(&info->req_arena);
    nk_arena_reset(&info->res_arena);

    /* Wait for request to lights gpio entity. */
    if (nk_transport_recv(&info->transport.base,
                          &info->req.base_,
                          &info->req_arena) != NK_EOK) {
        fprintf(stderr, "nk_transport_recv error\n");
    } else {
        /**
         * Handle received request by calling implementation Mode_impl
         * of the requested Mode interface method.
         */
        traffic_light_ControlSystem_entity_dispatch(&info->entity, &info->req.base_, &info->req_arena,
                                                    &info->res.base_, &info->res_arena);

        set_mode(mode, info->res.ctrl_mode.set.result);
    }

    /* Send response. */
    if (nk_transport_reply(&info->transport.base,
                           &info->res.base_,
                           &info->res_arena) != NK_EOK) {
        fprintf(stderr, "nk_transport_reply error\n");
    }
}

int connector_work(void *context)
{
    struct Mode *mode = context;
    struct SetModeInfo* connector = connector_init();
    fprintf(stderr, "%s connector thread created\n", LogPrefix);
    while(1)
    {
        fprintf(stderr, "%s connector thread loop\n", LogPrefix);
        connector_loop(connector, mode);
    }
    return 0;
}