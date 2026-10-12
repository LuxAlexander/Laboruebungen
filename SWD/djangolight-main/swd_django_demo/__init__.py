from django.apps import apps as django_apps


def get_container():
    return django_apps.get_app_config("swd_django_demo").container
