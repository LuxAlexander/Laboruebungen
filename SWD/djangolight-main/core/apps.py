from django.apps import AppConfig


class CoreConfig(AppConfig):
    # The default_auto_field attribute specifies the type of the primary key that Django should use for models
    # that do not specify a primary key type explicitly. In this case, we're using a BigAutoField as the default
    # type for the primary key field.
    default_auto_field: str = 'django.db.models.BigAutoField'

    # The name attribute specifies the name of the app that this configuration belongs to.
    name: str = 'core'
