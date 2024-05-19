from models import Trafficlight, Mode, M


def print_error(err):
    return ', '.join([v for v, e in zip(['R', 'Y', 'G'], err) if e])


def print_errors(errs):
    data = [f"{i['time']} [{i['severity']}] lamps broken: {print_error(i['error'])}" for i in errs]
    return '<br>'.join(data)


def print_mode(mode):
    return {
        "blink": mode.blink,
        "colors": [n for n, v in zip(['red', 'yellow', 'green'], [mode.r, mode.y, mode.g]) if v]
    }
