from django.urls import path

from . import views

app_name = "orders"

urlpatterns = [
    path("", views.match_list, name="match_list"),
    path("match/<int:match_id>/", views.match_detail, name="match_detail"),
    path("basket/", views.basket_overview, name="basket_overview"),
    path("basket/add", views.add_seat, name="add_seat"),
    path("basket/remove", views.remove_seat, name="remove_seat"),
    path("basket/clear", views.clear_basket, name="clear_basket"),
    path("basket/buy", views.place_order, name="place_order"),
    path("order/<int:order_id>/", views.confirmation, name="confirmation"),
    path("my/", views.order_list, name="order_list"),
]
