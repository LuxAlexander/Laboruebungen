# SWD Demo Shop

The shop domain of the Software-Design lab reference application: a visitor collects products from a
catalog in a basket, and a customer places an order that is paid from prepaid credit. Stock, shipping,
taxes and external payment are deliberately not part of this domain.

## Language

### Catalog

**Catalog**:
The set of products that can be ordered. The shop only reads it; products enter it from outside.
_Avoid_: inventory, assortment, stock (nothing here is counted or reserved)

**Product**:
A single offering in the catalog, identified by its name and carrying a description and a price.
_Avoid_: item, article, good, goods

**Price**:
What one unit of a product costs a customer. The catalog decides it, so the same product may cost
different amounts in two different catalogs.
_Avoid_: cost, amount, value

### Basket

**Visitor**:
Someone using the shop who is not logged in. A visitor may browse and fill a basket but cannot place
an order.
_Avoid_: guest, anonymous user, session

**Basket**:
The products a visitor intends to order, kept until the order is placed or the basket is cleared. It
carries no prices, and adding the same product again raises that product's quantity.
_Avoid_: cart, shopping cart, wishlist

**Order Line**:
One distinct product within a basket or an order, with its quantity, the price of a single unit and
its position in the order.
_Avoid_: position, order position, line item, order item

**Order Summary**:
A basket after the catalog has priced it: the order lines and the total. It is owned by nobody and
recorded nowhere until the order is placed.
_Avoid_: draft order, quote, priced basket, pending order

### Ordering

**Customer**:
A person who can place orders. The same person logs in to the shop; there is no separate notion of a
user or a shop account.
_Avoid_: user, client, buyer, shopper

**Credit**:
The prepaid amount a customer may spend. It is the only means of payment in this shop, and a customer
without enough of it cannot order.
_Avoid_: balance, funds, budget, wallet

**Redeem**:
To take an order's total out of a customer's credit. A redemption happens in full or not at all.
_Avoid_: charge, debit, withdraw, deduct

**Place**:
To turn a basket into an order: price it, redeem the credit, record it. An empty basket cannot be
placed.
_Avoid_: check out, submit, buy, purchase

**Order**:
A basket that a customer has placed and paid for with credit. An order is final; it is never changed
or cancelled.
_Avoid_: purchase, transaction, sale

**Order History**:
The orders one customer has placed. It can only be read, and only by that customer.
_Avoid_: my orders, past orders, order list
