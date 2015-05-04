#!/usr/bin/python3
# Copyright 2015 Jetperch LLC
# This file is licensed under the MIT License
# http://opensource.org/licenses/MIT

# Requires Python 3.x with packages:
# pip3 install cherrypy ws4py requests Jinja2
#

import cherrypy
from cherrypy.lib import auth_digest
import requests
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.websocket import WebSocket
import yaml
import os
from jinja2 import Environment, PackageLoader
from collections import OrderedDict


MYDIR = os.path.dirname(os.path.abspath(__file__))
AUTH_FILE = os.path.join(MYDIR, 'auth.yaml')
STATIC_DIR = os.path.join(MYDIR, 'mcu_server', 'www', 'static')


def load_auth():
    with open(AUTH_FILE, 'r') as f:
        return yaml.load(f)
AUTH = load_auth()


# See http://www.ralph-heinkel.com/blog/category/web/
class Publisher(WebSocket):
    SUBSCRIBERS = set()

    def __init__(self, *args, **kwargs):
        WebSocket.__init__(self, *args, **kwargs)
        print('WebSocket created')
        self.SUBSCRIBERS.add(self)
        
    def received_message(self, msg):
        if msg.is_text:
            text = msg.data.decode('utf-8')
            print('received_message: %s' % text)
        else:
            print('received_message: [binary_data]')

    def closed(self, code, reason=None):
        self.SUBSCRIBERS.remove(self)
    
    @staticmethod
    def publish(msg):
        print('publish(%s)' % (msg))
        for subscriber in Publisher.SUBSCRIBERS:
            subscriber.send(msg)


def electricimp(mode='OFF'):
    """Control the Electric Imp device.
    
    :param mode: The mode which is defined by the agent and is either
        "OFF" or "rotate_hue".
    """
    url = 'https://agent.electricimp.com/%s' % AUTH['electric_imp']['agent']
    args = {'setMode': mode}
    print('post to %s' % url)
    requests.post(url, data=args)

    
def spark(mode='OFF'):
    """Control the Spark Core device.
    
    :param mode: The mode which is defined by the Spark code and is either
        "OFF" or "ROTATE_HUE".
    """
    url = 'https://api.spark.io/v1/devices/%s/mode' % AUTH['spark']['device']
    args = {'access_token': AUTH['spark']['token'],
            'params': mode}
    print('post to %s' % url)
    requests.post(url, data=args)

    
class McuProtoServer(object):
    def __init__(self):
        # Map device identifier to a dict of configuration properties
        #   name: The user-meaningful device name.
        #   ide_name: The user-meaningful IDE name.
        #   ide_url: The URL for the IDE suitable for a web browser.
        #   permission: The current public permission: true is enabled.
        #   OFF: The callable to turn off the device.
        #   ON: The callable to turn on the device.
        self.devices = OrderedDict([
            ('ElectricImp', {
                'name': 'Electric Imp',
                'ide_name': 'IDE',
                'ide_url': 'https://ide.electricimp.com',
                'permission': False, 
                'OFF': lambda: electricimp('OFF'),
                'ON': lambda: electricimp('rotate_hue')
            }),
            ('SparkCore', {
                'name': 'Spark Core',
                'ide_name': 'IDE',
                'ide_url': 'https://build.spark.io/build',
                'permission': False,
                'OFF': lambda: spark('OFF'),
                'ON': lambda: spark('ROTATE_HUE')
            }),
            ('CC3200', {
                'name': 'CC3200', 
                'ide_name': 'Energia',
                'ide_url': 'http://energia.nu/',
                'permission': False,
                'OFF': lambda: self.publish('CC3200_OFF'),
                'ON': lambda: self.publish('CC3200_ON')
            }),
            ('mbed', {
                'name': 'FRDM-K64F',
                'ide_name': 'mbed',
                'ide_url': 'http://developer.mbed.org/compiler/',
                'permission': False,
                'OFF': lambda: self.publish('mbed_OFF'),
                'ON': lambda: self.publish('mbed_ON')
            }),
        ])
        self.env = Environment(loader=PackageLoader('mcu_server', 'www'),
                               trim_blocks=True,
                               line_statement_prefix='@')
    
    def render(self, page, **kwargs):
        template = self.env.get_template(page)
        return template.render(**kwargs)
    
    @cherrypy.expose
    def index(self):
        return self.render('index.html', status='', devices=self.devices, auth=False)
    
    @cherrypy.expose
    def ws(self):
        pass # delegate to handler

    @cherrypy.expose
    def control(self, device, action):
        if device not in self.devices:
            return 'device not found'
        d = self.devices[device]
        if not d['permission']:
            return 'access denied'
        if action not in d:
            return 'control not found'
        d[action]()
        return 'done!'

    @cherrypy.expose
    def publish(self, msg):
        parts = msg.split('_', 1)
        if len(parts) == 2:
            device = self.devices.get(parts[0])
            if device is not None and not device['permission']:
                return "access denied"
        Publisher.publish(msg)
        return "Done!"

    @cherrypy.expose
    def auth(self, device=None):
        if device is None:
            status = ""
        elif device in self.devices:
            d = self.devices[device]
            permissions = not d['permission']
            d['permission'] = permissions
            if not permissions:
                self.control(device, 'OFF')
            status = "success"
        else:
            status = "Device not found: %s" % device
        return self.render('index.html', status=status, devices=self.devices, auth=True)


if __name__ == '__main__':
    cherrypy.config.update({
        'server.socket_host': '0.0.0.0',
        'server.socket_port': 8080
    })
    WebSocketPlugin(cherrypy.engine).subscribe()
    cherrypy.tools.websocket = WebSocketTool()
    cherrypy.tree.mount(McuProtoServer(), '/', {
        '/ws': {
            'tools.websocket.on': True,
            'tools.websocket.handler_cls': Publisher,
            },
        '/static': {
            'tools.staticdir.on': True,
            'tools.staticdir.dir': STATIC_DIR,
            },
        '/auth': {
            'tools.auth_digest.on': True,
            'tools.auth_digest.realm': 'localhost',
            'tools.auth_digest.get_ha1': auth_digest.get_ha1_dict_plain(AUTH['server']),
            'tools.auth_digest.key': 'a565c27146791cfb',    
        },
    })
    cherrypy.engine.signals.subscribe()
    cherrypy.engine.start()
    cherrypy.engine.block()    
