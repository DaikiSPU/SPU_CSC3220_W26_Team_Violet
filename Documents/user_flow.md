1. User opens this app.
1. Check if user is stored in DB.
1. If no user in DB, go to Create Account Page, otherwise go to Login Page.

# Create Account Page
1. User enters username, password, and pin number
1. User clicks "Create".
1. System hashes password and pin number.
1. System stores username, hased password, and hashed pin number in DB.
1. If it's completed, user is redirected to Main Trading Page.
1. If it's failed, an error message is popped up in Create Account Page.

# Login Page
1. User enters username and password.
1. User clicks "Login".
1. System validates credentials.
1. If it's successful, user is redirected to Main Trading Page.
1. If it's failed, an error message is popped up in Login Page.

# Main Trading Page (Dashboard)
1. System loads market data, user account information, and positions.
1. System initializes trading bots and market simulation.
1. User selects a market from the market dropdown menu.
1. System updates the following panels based on the selected market:
    - Order Book (Buy/Sell orders)
    - Price Chart
    - User Position
    - Transaction History

# Viewing Market Information
1. User observes the price chart to understand price movement.
1. User checks the order book to view current buy and sell orders.
1. User checks account summary (cash, equity, P/L).

# Placing an Order
1. User selects Buy or Sell in the Order Entry panel.
1. User selects order type.
1. User enters price and quantity.
1. User clicks Place Order.
1. System validates the order:
    - Price must be valid
    - Quantity must be positive
    - User must have sufficient cash or shares
1. If validation fails, an error message is displayed.
1. If validation succeeds:
    - Order is inserted into the Orders table
    - Matching engine attempts to match orders

# Order Matching
1. If matching orders exist in the order book:
    - A trade is executed.
1. Trade information is stored in the Trades table.
1. The system updates:
    - User positions
    - User cash balance
    - Transaction history