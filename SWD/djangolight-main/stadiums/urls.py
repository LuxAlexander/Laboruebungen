from django.urls import path

from . import views

app_name = "stadiums"

urlpatterns = [
    path("statistics/", views.statistics, name="statistics"),
]
