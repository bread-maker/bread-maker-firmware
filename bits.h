#define set(reg,value) reg |= (value)
#define unset(reg,value) reg &= ~(value)
#define set_bit(reg,value) reg |= (_BV(value))
#define unset_bit(reg,value) reg &= ~(_BV(value))
