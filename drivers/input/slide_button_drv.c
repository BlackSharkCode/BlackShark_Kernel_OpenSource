
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/of_irq.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>

#define SLIDE_BUTTON_NAME "slide_button"

enum{
    SLIDE_BUTTON_DISABLE,
    SLIDE_BUTTON_ENABLE,
    SLIDE_BUTTON_MAX
};

struct slide_button_desc {
    int32_t irq_gpio;
    int irq;
    int wakeup;
    //unsigned int pin;
    int level_val;
    int debounce_interval;
    int software_debounce;
    struct delayed_work work;

    struct gpio_desc *gpiod;
}slide_button_desc;

static const struct of_device_id slide_button_of_match[] = {
    { .compatible = "slide_button", },
    { },
};

static struct slide_button_desc slide_button = {0};
static struct input_dev *button_idev;

static int slide_button_get_level_value(void) {
    int ret = -1;
    int value = -1;

    ret = gpio_get_value(slide_button.irq_gpio);
    if(ret == 0) {
        value = SLIDE_BUTTON_ENABLE;
    }
    else {
        value = SLIDE_BUTTON_DISABLE;
    }
    pr_err("%s value:%d\n", __func__, value);

    return value;
}

static void slide_key_handle(struct work_struct *work)
{
    int ret;


    ret = slide_button_get_level_value();
    if(ret != slide_button.level_val) {
        pr_err("%s level_val:%d\n", __func__, ret);

        input_report_switch(button_idev, SW_KEYPAD_SLIDE, ret);
        input_sync(button_idev);

        slide_button.level_val = ret;
    }

}

static irqreturn_t slide_button_irq_trigger_handler(int32_t irq, void *dev_id)
{


	if (slide_button.wakeup)
		pm_wakeup_event(button_idev->dev.parent, 5000);

	mod_delayed_work(system_wq,
			 &slide_button.work,
			 msecs_to_jiffies(slide_button.software_debounce));

    return IRQ_HANDLED;
}



static int slide_button_parse_dt(struct device *dev) {
    int ret = -1;
    struct device_node *np = dev->of_node;

    slide_button.wakeup = of_property_read_bool(np, "slide-button,wakeup");

    slide_button.irq_gpio = of_get_named_gpio_flags(np, "slide-button,gpios", 0, NULL);
    if(gpio_is_valid(slide_button.irq_gpio)) {
        ret = gpio_request(slide_button.irq_gpio, "slide_button-irq");

        slide_button.gpiod = gpio_to_desc(slide_button.irq_gpio);
        if (!(slide_button.gpiod)) {
            pr_err("%s, gpiod request failed\n", __func__);
            return -1;
        }
    }

    if (of_property_read_u32(np, "debounce-interval", &slide_button.debounce_interval)) {
            pr_err("%s, debounce-interval failed\n", __func__);
            slide_button.debounce_interval = 15;
    }


    return ret;
}


#ifdef CONFIG_PM_SLEEP
static int slide_keys_suspend(struct device *dev)
{

	if (device_may_wakeup(dev)) {
			if (slide_button.wakeup)
				enable_irq_wake(slide_button.irq);
	}

	return 0;
}

static int slide_keys_resume(struct device *dev)
{

	if (device_may_wakeup(dev)) {
			if (slide_button.wakeup)
				disable_irq_wake(slide_button.irq);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(slide_keys_pm_ops, slide_keys_suspend, slide_keys_resume);



static int slide_button_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    int ret = -1;
    int wakeup = 0;

    pr_err("%s enter\n", __func__);

    ret = slide_button_parse_dt(dev);
    if(ret < 0) {
        pr_err("%s fail to parse dt \n", __func__);
        return -EIO;
    }

	slide_button.software_debounce = 0;

	if (slide_button.debounce_interval) {
		if (slide_button.gpiod) {
			pr_err("%s set debounce\n", __func__);
			ret = gpiod_set_debounce(slide_button.gpiod,
					slide_button.debounce_interval * 1000);
			if (ret < 0) {
				pr_err("%s, set debounce failed, use soft_debounce %d\n", __func__, ret);
				slide_button.software_debounce =
						slide_button.debounce_interval;
			}
		}
	}



    //input device init
    button_idev = devm_input_allocate_device(dev);
    if(NULL == button_idev) {
        pr_err("%s mfailed to allocate input device\n", __func__);
        return -ENOMEM;
    }

    button_idev->phys = "slide_button/input0";
    button_idev->name = SLIDE_BUTTON_NAME;
    button_idev->id.bustype = BUS_HOST;

    set_bit(EV_KEY , button_idev->evbit);
    set_bit(EV_SW , button_idev->evbit);
    set_bit(SW_KEYPAD_SLIDE, button_idev->swbit);

    //interrupt register and handle
    slide_button.irq = gpio_to_irq(slide_button.irq_gpio);
    if(slide_button.irq < 0) {
        pr_err("%s gpio_to_irq fail\n", __func__);
        ret = slide_button.irq;
        goto err_free_dev;
    }
    pr_err("%s irq:%d\n", __func__, slide_button.irq);

    ret = input_register_device(button_idev);
    if(ret) {
        pr_err("%s fail register input device\n", __func__);
        goto err_free_dev;
    }

    ret = slide_button_get_level_value();
    slide_button.level_val = ret;

    input_report_switch(button_idev, SW_KEYPAD_SLIDE, ret);
    input_sync(button_idev);

    INIT_DELAYED_WORK(&slide_button.work,  slide_key_handle);

    ret = request_irq(slide_button.irq, slide_button_irq_trigger_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, \
                        "slide_button", &slide_button);
    if(ret)
    {
        pr_err("%s request irq fail\n", __func__);
        goto err_free_irq;
    }

    if(slide_button.wakeup)
    {
        wakeup = 1;
    }

    device_init_wakeup(&pdev->dev, wakeup);

    return 0;

err_free_irq:
    free_irq(slide_button.irq, slide_button_irq_trigger_handler);
err_free_dev:
    input_unregister_device(button_idev);
    return ret;
}

static int slide_button_remove(struct platform_device *pdev)
{
    if(slide_button.irq)
    {
        free_irq(slide_button.irq, slide_button_irq_trigger_handler);
    }

    input_unregister_device(button_idev);

    return 0;
}

static struct platform_driver slide_button_device_driver = {
    .probe	    = slide_button_probe,
    .remove	    = slide_button_remove,
	.driver	    = {
        .name   = "slide_button",
	.pm = &slide_keys_pm_ops,
        .of_match_table = of_match_ptr(slide_button_of_match),
    }
};

static int __init slide_button_init(void)
{
    pr_err("%s enter\n", __func__);
    return platform_driver_register(&slide_button_device_driver);
}

static void __exit slide_button_exit(void)
{
    platform_driver_unregister(&slide_button_device_driver);
}

module_init(slide_button_init);
module_exit(slide_button_exit);

MODULE_LICENSE("GPL");
