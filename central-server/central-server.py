from bottle import run, get, post, request, route, response
from json import dumps

@get('/test')
def process_test():
    return "This is dummy central server. Test passed\n"

@route('/mode/<tl_mode>', method = 'GET')
def process_test(tl_mode):
    print(f"requested mode for TL with Mode {tl_mode}")
    mode = { "id": int(tl_mode) }
    response.content_type = 'application/json'
    return dumps(mode)


run(host='0.0.0.0', port=8081, debug=True)