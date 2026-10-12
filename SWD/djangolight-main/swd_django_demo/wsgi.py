"""
WSGI config for swd_django_demo project.

It exposes the WSGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/4.1/howto/deployment/wsgi/

This is the wsgi.py file for a Django project, which stands for Web Server Gateway Interface.
The WSGI is a specification for a universal interface between web servers and web applications or frameworks for Python.
This file is responsible for exposing the Django application to the web server.
The get_wsgi_application function returns a WSGI callable, which is a Python object that can be invoked by a web server
to handle requests.
The os.environ.setdefault method sets the DJANGO_SETTINGS_MODULE environment variable to swd_django_demo.settings,
which is the name of the settings module for the Django project.
This ensures that Django uses the correct settings module when the application is started.

When this file is executed, the application variable is set to the WSGI callable,
which can be used by the web server to serve the Django application.
"""

import os

from django.core.wsgi import get_wsgi_application

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'swd_django_demo.settings')

application = get_wsgi_application()
