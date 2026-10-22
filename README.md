🍽️ Restaurant Order Management System

📌 Overview

An embedded restaurant ordering system using the LPC2129 ARM7 microcontroller, LCD, keypad, and HC-05 Bluetooth module. It allows users to select a table, place food orders, and notify the chef wirelessly.

✨ Features

- Table selection (1–9)
- Food selection: Dosa, Idli, Meal, and Tea
- Quantity management
- Order confirmation using "#"
- Clear order using "*"
- Bluetooth communication with the chef
- LCD display for order status

🛠️ Hardware Requirements

- LPC2129 ARM7
- 16×2 LCD
- 4×4 Matrix Keypad
- HC-05 Bluetooth Module
- Connecting wires and power supply

💻 Technologies Used

Embedded C, UART0, LCD, Keypad, and Bluetooth.

🚀 Working

1. Select a table.
2. Choose food items using the keypad.
3. Press "#" to send the order via Bluetooth.
4. The chef sends "ready" after preparing the order.
5. The LCD displays the order status.

🎯 Applications

- Restaurants and cafés
- Embedded systems projects
- Wireless food ordering prototypes
