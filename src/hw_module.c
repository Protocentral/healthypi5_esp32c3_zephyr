#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>

#define HW_THREAD_STACKSIZE 4096
#define HW_THREAD_PRIORITY 5

// Boot Control GPIO
static const struct gpio_dt_spec boot_ctrl_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(bootctrl), gpios);

// User Button SW0
static struct gpio_dt_spec button_user = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_user_cb;

#define GPIO_DEBOUNCE_TIME K_MSEC(60)
K_SEM_DEFINE(sem_user_key_pressed, 0, 1);

static void cooldown_expired_isr_user(struct k_work *work)
{
    ARG_UNUSED(work);

    int val = gpio_pin_get_dt(&button_user);
    // enum button_evt evt = val ? BUTTON_EVT_PRESSED : BUTTON_EVT_RELEASED;
    //  m_key_pressed = GPIO_KEYPAD_KEY_RIGHT;
    if (val == 0)
    {
        k_sem_give(&sem_user_key_pressed);
        printk("U\n");
    }
}

static K_WORK_DELAYABLE_DEFINE(cooldown_work_user, cooldown_expired_isr_user);

static void button_isr_user(const struct device *port,
                            struct gpio_callback *cb,
                            uint32_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    k_work_reschedule(&cooldown_work_user, GPIO_DEBOUNCE_TIME);
    printk("User Button Pressed\n");
}

void rp_set_boot_ctrl_on(void)
{
    gpio_pin_set_dt(&boot_ctrl_gpio, 1);
}

void hw_thread(int unused1, int unused2, int unused3)
{
    int ret = 0;

    if (!device_is_ready(boot_ctrl_gpio.port))
    {
        return;
    }

    ret = gpio_pin_configure_dt(&boot_ctrl_gpio, GPIO_OUTPUT);
    gpio_pin_set_dt(&boot_ctrl_gpio, 0);
    
    // Configure sw0 button

    ret = gpio_pin_configure_dt(&button_user, GPIO_INPUT);

    //gpio_init_callback(&button_user_cb, button_isr_user, BIT(button_user.pin));
    //ret = gpio_add_callback(button_user.port, &button_user_cb);
    //ret = gpio_pin_interrupt_configure_dt(&button_user, GPIO_INT_EDGE_TO_ACTIVE);

    printk("HW Thread started!\n");

    for (;;)
    {
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(hw_thread_id, HW_THREAD_STACKSIZE, hw_thread, NULL, NULL, NULL, HW_THREAD_PRIORITY, 0, 0);
