# Copyright 2015 Jetperch LLC

# Requires Python 3.x with cherrypy & requests
# pip install cherrypy

import cherrypy
import requests
import yaml
import os


MYDIR = os.path.dirname(os.path.abspath(__file__))
AUTH_FILE = os.path.join(MYDIR, 'auth.yaml')


def load_auth():
    with open(AUTH_FILE, 'r') as f:
        return yaml.load(f)


PAGE = """\
<!DOCTYPE html>
<html>
<head>
    <title>My Form</title>
</head>
<body>
    <h1>Spark demo</h1>
    <form method='GET' target="submit_frame">
        <input type='hidden' name='mode' value='ROTATE_HUE'>
        <input type='submit' value='Rotate Hue'></p>
    </form>
    <form method='GET' target="submit_frame">
        <input type='hidden' name='mode' value='OFF'>
        <input type='submit' value='Off'></p>
    </form>
    <p>{message}</p>
    <iframe name="submit_frame" style="display:none;"></iframe>
</body>
</html>
"""
        
class McuProtoServer(object):
    def __init__(self):
        self.auth = load_auth()

    @cherrypy.expose
    def index(self):
        return "Hello world!"
        
    @cherrypy.expose
    def spark(self, mode='ROTATE_HUE'):
        url = 'https://api.spark.io/v1/devices/%s/mode' % self.auth['spark']['device']
        args = {'access_token': self.auth['spark']['token'],
                'params': mode}
        requests.post(url, data=args)
        return PAGE.format(message='')

if __name__ == '__main__':
   cherrypy.quickstart(McuProtoServer())
