from collections import Counter
from decimal import Decimal
from typing import Iterable, MutableMapping, Optional

from core.domain import CustomerInfo, OrderSummary, OrderLine, CatalogItem
from core.interfaces import ProductCatalog


class Basket:
    """
    The products a visitor intends to order, held in the session until an order is placed.
    Owns the session key and the pricing of the basket into order positions.
    """

    SESSION_KEY = "product_list"

    def __init__(self, product_ids: Iterable = (), session: Optional[MutableMapping] = None):
        # session data must be JSON serializable, so ids are kept as strings
        self._product_ids: list[str] = [str(product_id) for product_id in product_ids]
        self._session = session

    @classmethod
    def load(cls, session: MutableMapping) -> "Basket":
        return cls(session.get(cls.SESSION_KEY, []), session)

    def __len__(self) -> int:
        return len(self._product_ids)

    def add(self, product_id) -> None:
        self._product_ids.append(str(product_id))
        if self._session is not None:
            self._session[self.SESSION_KEY] = list(self._product_ids)

    def clear(self) -> None:
        self._product_ids.clear()
        if self._session is not None:
            self._session.pop(self.SESSION_KEY, None)

    def lines(self, products: ProductCatalog, customer: Optional[CustomerInfo] = None) -> OrderSummary:
        """
        Price the basket: one position per distinct product, in order of first appearance.
        Unknown product ids are skipped.
        """
        positions: list[OrderLine] = []
        total_price = Decimal("0")
        for product_id, quantity in Counter(self._product_ids).items():
            product: Optional[CatalogItem] = products.get_product(product_id)
            if product is None:
                continue
            positions.append(OrderLine(pos=len(positions) + 1, product=product,
                                              quantity=quantity, price=product.price))
            total_price += product.price * quantity

        return OrderSummary(order_number=0, customer=customer, lines=positions, total_price=total_price)
