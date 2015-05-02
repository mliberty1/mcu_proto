#!/usr/bin/python3
# Copyright 2015 Jetperch LLC

# Requires Python 3.x with packages:
# pip3 install cherrypy ws4py requests Jinja2
#

import cherrypy
from cherrypy.lib import auth_digest
import requests
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.websocket import WebSocket
import yaml
import json
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


class McuProtoServer(object):
    def __init__(self):
        self.permissions = OrderedDict([
            ('ElectricImp', False),
            ('Spark', False),
            ('CC3200', False),
            ('mbed', False),
        ])
        self.env = Environment(loader=PackageLoader('mcu_server', 'www'),
                               trim_blocks=True,
                               line_statement_prefix='@')
    
    def render(self, page, **kwargs):
        template = self.env.get_template(page)
        return template.render(**kwargs)
    
    @cherrypy.expose
    def index(self):
        return self.render('index.html')

    @cherrypy.expose
    def electricimp(self, mode='OFF'):
        if not self.permissions['ElectricImp']:
            return "access denied"
        url = 'https://agent.electricimp.com/%s' % AUTH['electric_imp']['agent']
        args = {'setMode': mode}
        requests.post(url, data=args)
        return "Done!"
        
    @cherrypy.expose
    def spark(self, mode='ROTATE_HUE'):
        if not self.permissions['Spark']:
            return "access denied"
        url = 'https://api.spark.io/v1/devices/%s/mode' % AUTH['spark']['device']
        args = {'access_token': AUTH['spark']['token'],
                'params': mode}
        requests.post(url, data=args)
        return "Done!"
    
    @cherrypy.expose
    def ws(self):
        pass # delegate to handler
        
    @cherrypy.expose
    def publish(self, msg):
        parts = msg.split('_', 1)
        if len(parts) == 2:
            permission = self.permissions.get(parts[0])
            if not permission:
                return "access denied"
        Publisher.publish(msg)
        return "Done!"

    @cherrypy.expose
    def auth(self, device=None):
        if device is None:
            status = ""
        elif device in self.permissions:
            self.permissions[device] = not self.permissions[device]
            status = "success"
        else:
            status = "Device not found: %s" % device
        return self.render('auth.html', status=status, permissions=self.permissions)


        
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
