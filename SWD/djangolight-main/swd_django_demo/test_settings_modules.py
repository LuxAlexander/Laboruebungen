import json
import os
import subprocess
import sys
from pathlib import Path
from unittest import TestCase

"""
The settings modules are tested by loading them in a fresh Python process: Django reads its settings once per
process, and the test run itself already uses swd_django_demo.settings.
"""

PROJECT_DIR = Path(__file__).resolve().parent.parent
PROD = "swd_django_demo.settings_prod"
DEV = "swd_django_demo.settings"

PROD_ENV = {"DJANGO_SECRET_KEY": "prod-secret", "DJANGO_DB_PASSWORD": "db-secret"}

_PRINT_SETTINGS = """
import json
from django.conf import settings
print(json.dumps({
    "SECRET_KEY": settings.SECRET_KEY,
    "DEBUG": settings.DEBUG,
    "INSTALLED_APPS": settings.INSTALLED_APPS,
    "MIDDLEWARE": settings.MIDDLEWARE,
    "SESSION_SERIALIZER": getattr(settings, "SESSION_SERIALIZER", None),
    "AUTH_USER_MODEL": settings.AUTH_USER_MODEL,
    "DATABASE": {k: str(v) for k, v in settings.DATABASES["default"].items() if k in ("ENGINE", "NAME", "USER", "PASSWORD", "HOST")},
}))
"""


def _run(module: str, env: dict[str, str]) -> subprocess.CompletedProcess:
    # start from a clean environment, so settings of the developer's shell do not leak into the test
    clean = {k: v for k, v in os.environ.items() if not k.startswith("DJANGO_")}
    return subprocess.run([sys.executable, "-c", _PRINT_SETTINGS], cwd=PROJECT_DIR, capture_output=True, text=True,
                          env={**clean, **env, "DJANGO_SETTINGS_MODULE": module})


def load_settings(module: str, env: dict[str, str]) -> dict:
    result = _run(module, env)
    if result.returncode != 0:
        raise AssertionError(f"loading {module} failed:\n{result.stderr}")
    return json.loads(result.stdout)


class TestProdSettings(TestCase):
    def test_secret_key_comes_from_the_environment(self):
        self.assertEqual(load_settings(PROD, PROD_ENV)["SECRET_KEY"], "prod-secret")

    def test_missing_secret_key_fails_with_a_clear_message(self):
        result = _run(PROD, {"DJANGO_DB_PASSWORD": "db-secret"})

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ImproperlyConfigured: environment variable DJANGO_SECRET_KEY is not set", result.stderr)

    def test_shares_the_application_configuration_with_development(self):
        prod = load_settings(PROD, PROD_ENV)
        dev = load_settings(DEV, {})

        for name in ("INSTALLED_APPS", "MIDDLEWARE", "SESSION_SERIALIZER", "AUTH_USER_MODEL"):
            self.assertEqual(prod[name], dev[name], name)
        self.assertEqual(prod["SESSION_SERIALIZER"], "django.contrib.sessions.serializers.JSONSerializer")

    def test_runs_without_debug_on_mariadb(self):
        prod = load_settings(PROD, PROD_ENV)

        self.assertFalse(prod["DEBUG"])
        self.assertEqual(prod["DATABASE"], {"ENGINE": "django.db.backends.mysql", "NAME": "django-database",
                                            "USER": "root", "PASSWORD": "db-secret", "HOST": "database"})

    def test_missing_database_password_fails_with_a_clear_message(self):
        result = _run(PROD, {"DJANGO_SECRET_KEY": "prod-secret"})

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ImproperlyConfigured: environment variable DJANGO_DB_PASSWORD is not set", result.stderr)


class TestDevSettings(TestCase):
    def test_falls_back_to_the_insecure_development_key(self):
        self.assertTrue(load_settings(DEV, {})["SECRET_KEY"].startswith("django-insecure-"))

    def test_secret_key_can_be_overridden_by_the_environment(self):
        self.assertEqual(load_settings(DEV, {"DJANGO_SECRET_KEY": "local-secret"})["SECRET_KEY"], "local-secret")
