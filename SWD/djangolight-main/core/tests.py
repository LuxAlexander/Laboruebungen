from unittest.mock import patch

from django.db import OperationalError, connection
from django.test import TestCase
from django.urls import reverse


class TestSiteRoot(TestCase):
    def test_root_redirects_to_products(self):
        response = self.client.get("/")
        self.assertRedirects(response, reverse("products:index"), fetch_redirect_response=False)


class TestHealthCheck(TestCase):
    def test_reports_ok_when_the_database_answers(self):
        response = self.client.get(reverse("core:health_check"))

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {"status": "ok"})

    def test_reports_unhealthy_when_the_database_is_unreachable(self):
        # the database is the system boundary: simulate it being down
        with (patch.object(connection, "cursor", side_effect=OperationalError("connection refused")),
              self.assertLogs("core.views", level="ERROR") as logs):
            response = self.client.get(reverse("core:health_check"))

        self.assertEqual(response.status_code, 503)
        self.assertEqual(response.json(), {"status": "unhealthy"})
        # the response stays terse, the operator finds the cause in the log
        self.assertIn("connection refused", logs.output[0])
