#!/usr/bin/python3
# Copyright 2015 Jetperch LLC

# Requires Python 3.x with packages:
# pip3 install cherrypy ws4py requests Jinja2
#

import cherrypy
import requests
from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.websocket import WebSocket
import yaml
import os
from jinja2 import Environment, PackageLoader


MYDIR = os.path.dirname(os.path.abspath(__file__))
AUTH_FILE = os.path.join(MYDIR, 'auth.yaml')


def load_auth():
    with open(AUTH_FILE, 'r') as f:
        return yaml.load(f)


# See http://www.ralph-heinkel.com/blog/category/web/
class Publisher(WebSocket):
    SUBSCRIBERS = set()

    def __init__(self, *args, **kwargs):
        WebSocket.__init__(self, *args, **kwargs)
        print('WebSocket created')
        self.SUBSCRIBERS.add(self)
    
    def received_message(self, msg):
        print('received_message: %s' % msg)
        self.send(msg.data, msg.is_binary)

    def closed(self, code, reason=None):
        self.SUBSCRIBERS.remove(self)
    
    @staticmethod
    def publish(msg):
        for subscriber in Publisher.SUBSCRIBERS:
            print(msg)
            subscriber.send(msg)


class McuProtoServer(object):
    def __init__(self):
        self.auth = load_auth()
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
    def spark(self, mode='ROTATE_HUE'):
        url = 'https://api.spark.io/v1/devices/%s/mode' % self.auth['spark']['device']
        args = {'access_token': self.auth['spark']['token'],
                'params': mode}
        requests.post(url, data=args)
        return self.render('spark.html', message='')
    
    @cherrypy.expose
    def ws(self):
        pass # delegate to handler
        
    @cherrypy.expose
    def publish(self, msg):
        Publisher.publish(msg)
        return ""
        

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
               }
    })
    cherrypy.engine.signals.subscribe()
    cherrypy.engine.start()
    cherrypy.engine.block()    
