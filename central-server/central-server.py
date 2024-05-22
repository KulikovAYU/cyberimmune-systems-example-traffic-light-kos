from bottle import run, get, post, request, route, response
from json import dumps

tl_modes = {"12345": 0}

@get('/test')
def process_test():
    return "This is dummy central server. Test passed\n"

@route('/mode/<tl_id>', method = 'GET')
def process_test(tl_id):
    print(f"requested mode for TL with Mode {tl_id}")
    mode = { "tl-mode": tl_modes[tl_id] }
    response.content_type = 'application/json'
    return dumps(mode)


def process_mode():
    tl_data = request.json

    # print the dictionary
    print(f"\n{tl_data}\n")

    # extract data and print nicely
    tl_id = tl_data["tl-id"]
    tl_mode = tl_data["tl-mode"]

    print(f"tl-id: {tl_id}\n"
          f"tl-mode: {tl_mode}\n")

    tl_modes[tl_id] = tl_mode
    return "TL diagnostics received ok\n"

@post('/set_normal_mode')
def process_normal_mode():
    return process_mode()

@post('/set_service_mode')
def process_service_mode():
    return process_mode()


run(host='0.0.0.0', port=8081, debug=True)