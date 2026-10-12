"""
ASGI config for swd_django_demo project.

It exposes the ASGI callable as a module-level variable named ``application``.

For more information on this file, see
https://docs.djangoproject.com/en/4.1/howto/deployment/asgi/
"""

import os

from django.core.asgi import get_asgi_application

# The DJANGO_SETTINGS_MODULE environment variable is used to tell Django which settings module to use.
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'swd_django_demo.settings')

# get_asgi_application() function returns the ASGI application callable that can be used to run the application.
application = get_asgi_application()
