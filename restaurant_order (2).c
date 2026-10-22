/* ------------------------------------------------------------
   Restaurant Order System  -  LPC2129 (ARM7)
   16x2 LCD (8-bit) + 4x4 Keypad + UART0 + HC-05 Bluetooth

   PIN MAP
   UART0  : P0.0 = TXD0 -> HC-05 RXD
            P0.1 = RXD0 <- HC-05 TXD
   LCD    : D0-D7 = P0.16 - P0.23
            RS = P0.8 , E = P0.9 , RW = GND
   KEYPAD : Rows (output)  = P0.10 - P0.13
            Cols (input)   = P0.4  - P0.7  (need 10k pull-ups to 3.3V)

   PCLK assumed = 15 MHz (same as your delay_ms code)
   HC-05 data mode baud = 9600

   FLOW
   1. "Restaurant Management" title
   2. Press 1-9  -> "Table N selected"
   3. Menu: 1=Dosa 2=Idli 3=Meal 4=Tea   (press a key again = qty+1)
            #  = send order      * = clear order
   4. Order shown on LCD + sent to mobile through Bluetooth
   5. Chef types "ready" on mobile -> LCD shows "Order is READY"
   ------------------------------------------------------------ */
#include<LPC21XX.H>

#define LCD_DATA  (0xFF<<16)
#define LCD_RS    (1<<8)
#define LCD_E     (1<<9)
#define ROWS      (0xF<<10)

/* ---------- prototypes ---------- */
void delay_ms(unsigned int ms);
void lcd_cmd(unsigned char c);
void lcd_data(unsigned char d);
void lcd_init(void);
void lcd_clear(void);
void lcd_print(unsigned char line, const char *s);
void uart_init(void);
void uart_tx(char c);
void uart_str(const char *s);
char get_key(void);
char wait_key(void);

/* ---------- data ---------- */
const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

const char *item[4] = {"Dosa","Idli","Meal","Tea"};

/* ---------- delay (Timer0 polling) ---------- */
void delay_ms(unsigned int ms)
{
    T0PR  = 15000-1;
    T0TCR = 0x01;
    while(T0TC < ms);
    T0TCR = 0x03;
    T0TCR = 0x00;
}

/* ---------- LCD ---------- */
void lcd_cmd(unsigned char c)
{
    IOCLR0 = LCD_DATA;
    IOSET0 = ((unsigned int)c << 16);
    IOCLR0 = LCD_RS;
    IOSET0 = LCD_E;
    delay_ms(2);
    IOCLR0 = LCD_E;
}

void lcd_data(unsigned char d)
{
    IOCLR0 = LCD_DATA;
    IOSET0 = ((unsigned int)d << 16);
    IOSET0 = LCD_RS;
    IOSET0 = LCD_E;
    delay_ms(2);
    IOCLR0 = LCD_E;
}

void lcd_init(void)
{
    lcd_cmd(0x38);      /* 8-bit, 2 line, 5x8 */
    lcd_cmd(0x0C);      /* display ON, cursor OFF */
    lcd_cmd(0x06);      /* auto increment */
    lcd_cmd(0x01);      /* clear */
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
}

/* line = 0 -> first row, line = 1 -> second row */
void lcd_print(unsigned char line, const char *s)
{
    lcd_cmd(line == 0 ? 0x80 : 0xC0);
    while(*s)
        lcd_data(*s++);
}

/* ---------- UART0 ---------- */
void uart_init(void)
{
    PINSEL0 = (PINSEL0 & ~0x0F) | 0x05;   /* P0.0 = TXD0, P0.1 = RXD0 */
    U0LCR = 0x83;                          /* 8N1, DLAB = 1 */
    U0DLL = 98;                            /* 15MHz / (16*9600) = 97.6 */
    U0DLM = 0;
    U0LCR = 0x03;                          /* DLAB = 0 */
}

void uart_tx(char c)
{
    while(!(U0LSR & 0x20));                /* wait THR empty */
    U0THR = c;
}

void uart_str(const char *s)
{
    while(*s)
        uart_tx(*s++);
}

/* ---------- Keypad ---------- */
/* returns 0 if no key pressed, else the key character */
char get_key(void)
{
    unsigned char r, c, col;

    for(r = 0; r < 4; r++)
    {
        IOSET0 = ROWS;                     /* all rows HIGH */
        IOCLR0 = (1 << (10 + r));          /* pull ONE row LOW */
        delay_ms(1);                       /* let line settle */

        col = (IOPIN0 >> 4) & 0x0F;        /* read P0.4-P0.7 */
        if(col != 0x0F)                    /* some column went LOW */
        {
            for(c = 0; c < 4; c++)
            {
                if(!(col & (1 << c)))
                {
                    delay_ms(20);          /* debounce */
                    while((((IOPIN0 >> 4) & 0x0F)) != 0x0F);  /* wait release */
                    IOSET0 = ROWS;
                    return keymap[r][c];
                }
            }
        }
    }
    IOSET0 = ROWS;
    return 0;
}

char wait_key(void)
{
    char k;
    while((k = get_key()) == 0);
    return k;
}

/* ---------- main ---------- */
int main()
{
    unsigned char qty[4];
    unsigned char i, total, ready;
    char key, table;
    char buf[2];

    IODIR0 = LCD_DATA | LCD_RS | LCD_E | ROWS;   /* columns stay input */
    IOSET0 = ROWS;

    lcd_init();
    uart_init();

    /* 1. title */
    lcd_print(0, "Restaurant");
    lcd_print(1, "Management");
    uart_str("Restaurant Management\r\n");
    delay_ms(2000);

    while(1)
    {
        /* 2. select table */
        lcd_clear();
        lcd_print(0, "Select Table");
        lcd_print(1, "Press 1 - 9");

        do {
            key = wait_key();
        } while(key < '1' || key > '9');
        table = key;

        lcd_clear();
        lcd_print(0, "Table ");
        lcd_data(table);
        lcd_print(1, "selected");

        uart_str("Table ");
        uart_tx(table);
        uart_str(" selected\r\n");
        delay_ms(1500);

        /* 3. menu */
        for(i = 0; i < 4; i++) qty[i] = 0;
        total = 0;

        while(1)
        {
            lcd_clear();
            lcd_print(0, "1:Dosa 2:Idli");
            lcd_print(1, "3:Meal 4:Tea #OK");

            key = wait_key();

            if(key >= '1' && key <= '4')
            {
                i = key - '1';
                if(qty[i] < 9)
                {
                    qty[i]++;
                    total++;
                }
                lcd_clear();
                lcd_print(0, "Added: ");
                {
                    const char *p = item[i];
                    while(*p) lcd_data(*p++);   /* continue on same line */
                }
                lcd_print(1, "Qty: ");
                lcd_data('0' + qty[i]);
                delay_ms(1000);
            }
            else if(key == '*')            /* clear order */
            {
                for(i = 0; i < 4; i++) qty[i] = 0;
                total = 0;
                lcd_clear();
                lcd_print(0, "Order cleared");
                delay_ms(1000);
            }
            else if(key == '#')            /* confirm */
            {
                if(total == 0)
                {
                    lcd_clear();
                    lcd_print(0, "No items!");
                    delay_ms(1000);
                }
                else
                    break;
            }
        }

        /* 4. send order to LCD + mobile */
        lcd_clear();
        lcd_print(0, "Table ");
        lcd_data(table);
        lcd_print(1, "Order sent");

        uart_str("ORDER Table ");
        uart_tx(table);
        uart_str(": ");
        for(i = 0; i < 4; i++)
        {
            if(qty[i] > 0)
            {
                uart_str(item[i]);
                uart_str(" x");
                uart_tx('0' + qty[i]);
                uart_str("  ");
            }
        }
        uart_str("\r\n");
        delay_ms(1500);

        /* 5. wait for chef reply ("ready") */
        lcd_clear();
        lcd_print(0, "Waiting chef...");
        lcd_print(1, "Table ");
        lcd_data(table);

        ready = 0;
        while(!ready)
        {
            if(U0LSR & 0x01)               /* byte received */
            {
                buf[0] = U0RBR;
                if(buf[0] == 'r' || buf[0] == 'R')
                {
                    ready = 1;
                    delay_ms(100);         /* drain rest of "ready" */
                    while(U0LSR & 0x01)
                        buf[1] = U0RBR;
                }
            }
        }

        lcd_clear();
        lcd_print(0, "Table ");
        lcd_data(table);
        lcd_print(1, "Order is READY!");
        uart_str("Order READY for Table ");
        uart_tx(table);
        uart_str("\r\n");
        delay_ms(3000);
    }
}
