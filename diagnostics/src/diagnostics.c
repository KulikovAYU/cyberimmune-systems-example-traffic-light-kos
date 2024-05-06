#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#define NK_USE_UNQUALIFIED_NAMES
#include <traffic_light/Diagnostics.edl.h>


#include <assert.h>


static const char EntityName[] = "Diagnostics";

static nk_err_t DMessageImpl(struct traffic_light_IDiagMessage              *self,
                             const traffic_light_IDiagMessage_DMessage_req  *req,
                             const struct nk_arena                          *reqArena,
                             struct traffic_light_IDiagMessage_DMessage_res *res,
                             struct nk_arena                                *resArena) {

    uint32_t code=req->code;

    nk_uint32_t msg_len = 0;
    nk_ptr_t *msg = nk_arena_get(nk_char_t, reqArena, &(req->msg), &msg_len);

    fprintf(stderr, "[%s] Code[%u], Message:\"%s\"\n", EntityName, code, msg);

    return NK_EOK;
}


static struct IDiagMessage *CreateIDiagnosticsImpl(void) {
    static const struct IDiagMessage_ops Ops = {
            .DMessage  = DMessageImpl
    };
    static IDiagMessage obj = {
            .ops = &Ops
    };

    return &obj;
}



int main(void) {

    ServiceId iid;
    NkKosTransport transport;

    fprintf(stderr, "Hello I'm %s\n", EntityName);

    Handle handle = ServiceLocatorRegister("diagnostics_connection", NULL, 0, &iid);
    assert(handle != INVALID_HANDLE);

    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    Diagnostics_entity_req  req;
    Diagnostics_entity_res res;

    char reqBuffer[Diagnostics_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(reqBuffer, reqBuffer + sizeof(reqBuffer));

    char res_buffer[Diagnostics_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer, res_buffer + sizeof(res_buffer));


    CDiagMessage_component  component;
    CDiagMessage_component_init(&component, CreateIDiagnosticsImpl());

    Diagnostics_entity entity;
    Diagnostics_entity_init(&entity, &component);

    while(true){
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
            Diagnostics_entity_dispatch(&entity, &req.base_, &req_arena,
                                        &res.base_, &res_arena);
        }

        /* Send response. */
        if (nk_transport_reply(&transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "nk_transport_reply error\n");
        }
    }


    return EXIT_SUCCESS;
}