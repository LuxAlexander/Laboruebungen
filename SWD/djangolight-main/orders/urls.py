from django.urls import path

from . import views

app_name = "orders"

# Map URLs to views
urlpatterns = [
    path("add_to_basket", views.add_to_basket, name="add_to_basket"),
    path("basket_overview", views.basket_overview, name="basket_overview"),
    path("place_order", views.place_order, name="place_order"),
    path("clear_basket", views.clear_basket, name="clear_basket"),
    path("my", views.order_list, name="order_list"),
]
