from django.urls import path

from . import views

app_name = "access"

urlpatterns = [
    path("", views.gate, name="gate"),
    path("alerts/", views.alerts, name="alerts"),
    path("alerts/<int:alert_id>/acknowledge", views.acknowledge, name="acknowledge"),
]
