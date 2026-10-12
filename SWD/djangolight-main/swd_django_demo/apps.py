from django.apps import AppConfig
from django.conf import settings


class ProjectConfig(AppConfig):
    """Composition root: builds and wires the dependency-injection container."""

    name: str = "swd_django_demo"

    def ready(self) -> None:
        # ready() runs after every app's models are loaded, so the container
        # can import models from all apps without ordering issues.
        from swd_django_demo.containers import Container

        self.container = Container()
        self.container.config.from_dict(settings.__dict__)
        self.container.wire(modules=["products.views", "orders.views"])
