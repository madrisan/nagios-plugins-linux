#!/usr/bin/python3

import docker
import websockets

client = docker.Client()
container = client.create_container(
    image = 'centos:latest',
    stdin_open = True,
    tty = True,
    command = '/bin/sh',
)
