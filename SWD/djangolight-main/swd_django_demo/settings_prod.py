"""
Production settings for the container deployment (containers/compose.yaml).

Everything is inherited from the development settings in settings.py; this module only overrides what differs
in production. Select it with DJANGO_SETTINGS_MODULE=swd_django_demo.settings_prod (set in the Dockerfile).

Secrets are never checked in, they are passed in by the environment:
- DJANGO_SECRET_KEY   the cryptographic signing key
- DJANGO_DB_PASSWORD  the password of the database user

https://docs.djangoproject.com/en/stable/howto/deployment/checklist/
"""

import os

from django.core.exceptions import ImproperlyConfigured

from .settings import *  # noqa: F401,F403 - the shared configuration


def _required_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise ImproperlyConfigured(f"environment variable {name} is not set")
    return value


SECRET_KEY = _required_env("DJANGO_SECRET_KEY")

DEBUG = False

ALLOWED_HOSTS = ["127.0.0.1", "localhost"]
CSRF_TRUSTED_ORIGINS = ["http://localhost:8000", "http://127.0.0.1:8000"]

DATABASES = {
    "default": {
        "ENGINE": "django.db.backends.mysql",
        "NAME": os.environ.get("DJANGO_DB_NAME", "django-database"),
        "USER": os.environ.get("DJANGO_DB_USER", "root"),
        "PASSWORD": _required_env("DJANGO_DB_PASSWORD"),
        "HOST": os.environ.get("DJANGO_DB_HOST", "database"),
    }
}
