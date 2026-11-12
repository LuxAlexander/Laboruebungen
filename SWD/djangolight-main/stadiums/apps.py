from django.apps import AppConfig


class StadiumsConfig(AppConfig):
    default_auto_field: str = "django.db.models.BigAutoField"
    name: str = "stadiums"
    verbose_name: str = "Stadiums and matches (master data)"
