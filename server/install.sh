#!/bin/bash
#scp -i mcu_proto.pem server ubuntu@mcu_proto.jetperch.com:/home/ubuntu
#ssh -i mcu_proto.pem ubuntu@mcu_proto.jetperch.com
#chmod -R 770 server
#server/install.sh
#server/server.py
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install python3-pip python3-yaml
sudo pip3 install --upgrade cherrypy ws4py requests Jinja2
# redirect HTTP port 80 to allow Ubuntu user to serve
sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to 8080
