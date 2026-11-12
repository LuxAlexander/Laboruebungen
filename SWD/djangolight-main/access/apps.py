from django.apps import AppConfig


class AccessConfig(AppConfig):
    default_auto_field: str = "django.db.models.BigAutoField"
    name: str = "access"
    verbose_name: str = "Access control"
